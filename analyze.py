#!/usr/bin/env python3
"""Infer cache hierarchy parameters from a cache.c data file.

Reads the gnuplot-format TSV (stride, buffer, ns) emitted by cache.c
and prints the cache levels it can detect, their latencies, line size,
and L1 associativity.
"""
import argparse
import sys
from collections import defaultdict, Counter


def parse(path):
    """Return (meta, {buffer_size: {stride: ns}}) sorted by buffer size.

    meta is a dict of header fields: host, date, mode, kind."""
    data = defaultdict(dict)
    meta = {}
    with open(path) as f:
        for line in f:
            s = line.strip()
            if not s:
                continue
            if s.startswith("#"):
                content = s.lstrip("# ").rstrip()
                if "random access" in content:
                    meta["kind"] = "random"
                    meta.setdefault("mode", content)
                elif "sequential access" in content:
                    meta["kind"] = "sequential"
                    meta.setdefault("mode", content)
                elif ":" in content:
                    k, v = content.split(":", 1)
                    meta[k.strip()] = v.strip()
                continue
            parts = s.split()
            if len(parts) < 3:
                continue
            try:
                stride, buf, t = int(parts[0]), int(parts[1]), float(parts[2])
            except ValueError:
                continue
            data[buf][stride] = t
    meta.setdefault("kind", "unknown")
    return meta, dict(sorted(data.items()))


def humanize(n):
    for unit in ("B", "KB", "MB", "GB"):
        if n < 1024:
            return f"{n} {unit}" if isinstance(n, int) and n == int(n) else f"{n:.1f} {unit}"
        n //= 1024 if isinstance(n, int) else 1
        if not isinstance(n, int):
            n /= 1024
    return f"{n} TB"


def row_peak(rows):
    """Peak latency in a row, looking only at the rising portion (clips the
    right-edge collapse-to-L1 from Saavedra's regime 4)."""
    strides = sorted(rows)
    lats = [rows[s] for s in strides]
    peak_i = max(range(len(lats)), key=lats.__getitem__)
    return max(lats[: peak_i + 1])


def level_signal(rows, kind):
    """The per-buffer scalar used to detect cache levels. Random walks make
    the small-stride floor a clean signal. Sequential walks are streamed by
    the hardware prefetcher at small stride, so we use the row's peak (worst-
    case access cost) instead."""
    if kind == "random":
        return rows[min(rows)]
    return row_peak(rows)


def find_levels(data, kind, jump=1.4):
    """Group consecutive buffer sizes; a >jump× rise ends a group. Each group
    corresponds to a cache level (or DRAM)."""
    series = [(b, level_signal(rows, kind)) for b, rows in data.items()]
    groups = [[series[0]]]
    for prev, cur in zip(series, series[1:]):
        if cur[1] > prev[1] * jump:
            groups.append([cur])
        else:
            groups[-1].append(cur)
    return groups


def line_size(data, l1_max_buf, l2_max_buf):
    """For each row that lives in L2 (above L1 but within L2), find the
    smallest stride whose latency reaches 95% of the row's max. That stride
    is the cache line size (one miss per access from there on). Median across
    qualifying rows."""
    estimates = []
    for buf, rows in data.items():
        if buf <= 2 * l1_max_buf or buf > l2_max_buf:
            continue
        strides = sorted(rows)
        lats = [rows[s] for s in strides]
        if max(lats) / min(lats) < 1.15:
            continue  # too flat to find a knee
        peak = max(range(len(lats)), key=lats.__getitem__)
        rising_s, rising_l = strides[: peak + 1], lats[: peak + 1]
        thresh = 0.95 * max(rising_l)
        for s, l in zip(rising_s, rising_l):
            if l >= thresh:
                estimates.append(s)
                break
    if not estimates:
        return None, 0
    estimates.sort()
    return estimates[len(estimates) // 2], len(estimates)


def l1_associativity(data, l1_lat, l1_max_buf):
    """For rows above L1, the right-edge collapse to L1 latency happens at the
    stride where the walked set fits in one way. a = buf / collapse_stride."""
    estimates = []
    for buf, rows in data.items():
        if buf <= l1_max_buf:
            continue
        strides = sorted(rows)
        for s in strides:
            if rows[s] <= l1_lat * 1.5:
                a = buf // s
                if 2 <= a <= 32 and (a & (a - 1)) == 0:
                    estimates.append(a)
                break
    if not estimates:
        return None, 0
    a, n = Counter(estimates).most_common(1)[0]
    return a, n


def level_names(n_groups):
    if n_groups <= 1:
        return ["memory"]
    base = ["L1", "L2", "L3", "L4"]
    return base[: n_groups - 1] + ["memory"]


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("file")
    ap.add_argument("--ghz", type=float,
                    help="CPU clock in GHz; adds a cycle-count column")
    args = ap.parse_args()

    meta, data = parse(args.file)
    if not data:
        sys.exit(f"{args.file}: no data parsed")

    kind = meta["kind"]
    groups = find_levels(data, kind)
    names = level_names(len(groups))

    print(f"{args.file}")
    bits = []
    if "host" in meta:
        bits.append(f"host: {meta['host']}")
    if "date" in meta:
        bits.append(meta["date"])
    if meta.get("mode"):
        bits.append(meta["mode"])
    if bits:
        print("  " + " · ".join(bits))
    print()

    cyc_col = f"  {'cycles':>7s}" if args.ghz else ""
    print(f"  {'level':6s}  {'fits ≤':>10s}  {'latency':>10s}{cyc_col}")
    print(f"  {'-'*6}  {'-'*10}  {'-'*10}" + ("  " + "-"*7 if args.ghz else ""))

    l1_max = l1_lat = l2_max = None
    for name, group in zip(names, groups):
        lat = min(l for _, l in group)
        max_buf = max(b for b, _ in group)
        size_str = humanize(max_buf) if name != "memory" else "—"
        cyc_str = f"  {lat * args.ghz:>7.1f}" if args.ghz else ""
        print(f"  {name:6s}  {size_str:>10s}  {lat:>7.1f} ns{cyc_str}")
        if name == "L1":
            l1_max, l1_lat = max_buf, lat
        elif name == "L2":
            l2_max = max_buf

    print()
    if kind == "random" and l1_max and l2_max:
        ls, n = line_size(data, l1_max, l2_max)
        if ls:
            print(f"  line size          {ls} B  (consensus across {n} rows)")
    if kind == "random" and l1_max and l1_lat:
        a, n = l1_associativity(data, l1_lat, l1_max)
        if a:
            print(f"  L1 associativity   {a}-way  (consensus across {n} rows)")
    if kind != "random":
        print("  (line size & associativity need random-access data;")
        print("   the hardware prefetcher hides them in sequential walks)")


if __name__ == "__main__":
    main()
