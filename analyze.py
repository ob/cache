#!/usr/bin/env python3
"""Infer cache hierarchy parameters from a cache.c data file.

Reads the gnuplot-format TSV (stride, buffer, ns) emitted by cache.c
and prints inferred cache levels (size + latency range), line size,
L1 associativity, and TLB miss penalty. Reports ranges and confidence
instead of single-number summaries, because the underlying measurements
have real spread.
"""
import argparse
import sys
from collections import defaultdict, Counter


def parse(path):
    """Return (meta, {buffer_size: {stride: ns}}) sorted by buffer size.

    meta carries header fields (host, date, mode) and an inferred `kind`."""
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
    for unit in ("B", "KB", "MB", "GB", "TB"):
        if n < 1024:
            return f"{n:g} {unit}"
        n /= 1024
    return f"{n:g} PB"


def row_peak(rows):
    """Peak latency in a row, clipped to the rising portion (drops the
    right-edge collapse-to-L1 that Saavedra's regime 4 produces)."""
    strides = sorted(rows)
    lats = [rows[s] for s in strides]
    peak_i = max(range(len(lats)), key=lats.__getitem__)
    return max(lats[: peak_i + 1])


def row_floor(rows):
    return rows[min(rows)]


def level_signal(rows, kind):
    """The per-buffer scalar used to detect cache-level boundaries. Random
    walks expose the structure at the small-stride floor; sequential walks
    are streamed by the prefetcher at small stride, so we use the row peak."""
    return row_floor(rows) if kind == "random" else row_peak(rows)


def find_levels(data, kind, jump=1.4):
    """Group consecutive buffer sizes; a >jump× rise in the signal ends a
    group. Each group is a cache level (or DRAM)."""
    series = [(b, level_signal(rows, kind)) for b, rows in data.items()]
    groups = [[series[0]]]
    for prev, cur in zip(series, series[1:]):
        if cur[1] > prev[1] * jump:
            groups.append([cur])
        else:
            groups[-1].append(cur)
    return groups


def group_stats(group, data, kind):
    """Buffer range and latency range for one level group."""
    buffers = [b for b, _ in group]
    sigs = [level_signal(data[b], kind) for b in buffers]
    return {
        "min_buf": min(buffers),
        "max_buf": max(buffers),
        "lat_lo": min(sigs),
        "lat_hi": max(sigs),
        "n": len(buffers),
    }


def level_names(n_groups):
    if n_groups <= 1:
        return ["memory"]
    base = ["L1", "L2", "L3", "L4"]
    return base[: n_groups - 1] + ["memory"]


def line_size(data, l1_max_buf, l2_max_buf):
    """For rows in L2 (above L1 but within L2), the smallest stride whose
    latency reaches 95% of the row's peak marks the line size (one miss per
    access from there on). Median across qualifying rows."""
    estimates = []
    for buf, rows in data.items():
        if buf <= 2 * l1_max_buf or buf > l2_max_buf:
            continue
        strides = sorted(rows)
        lats = [rows[s] for s in strides]
        if max(lats) / min(lats) < 1.15:
            continue
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
    """For rows above L1, the right-edge collapse to ≈L1 latency happens at
    the stride where the walked set fits in one way. a = buf / stride."""
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


def tlb_penalty(data, dram_min_buf):
    """For the smallest buffer in DRAM region: row peak − row floor estimates
    the per-access cost of a TLB miss. Smaller buffers in DRAM expose this
    most cleanly (TLB pressure rises quickly with buffer; the L2-miss cost
    floor is roughly constant across the region)."""
    if dram_min_buf is None or dram_min_buf not in data:
        return None
    rows = data[dram_min_buf]
    return row_peak(rows) - row_floor(rows)


def is_suspect(prev_stats, this_stats, dram_stats):
    """A middle level is suspect if it has very few rows and its latency is
    much closer to a cache hit than to DRAM — likely TLB pressure or a noisy
    boundary, not a real cache level."""
    if this_stats["n"] >= 3:
        return False
    if dram_stats is None:
        return False
    gap_below = this_stats["lat_lo"] / max(prev_stats["lat_hi"], 1e-9)
    gap_to_dram = dram_stats["lat_lo"] / max(this_stats["lat_hi"], 1e-9)
    return gap_below < 4 and gap_to_dram > 2


def report(args, meta, data):
    kind = meta["kind"]
    groups = find_levels(data, kind)
    names = level_names(len(groups))
    stats = [group_stats(g, data, kind) for g in groups]

    print(args.file)
    bits = [meta[k] for k in ("host", "date", "mode") if k in meta]
    if bits:
        print("  " + "  ·  ".join(bits))
    print()

    has_dram = names[-1] == "memory"
    dram_stats = stats[-1] if has_dram else None

    # Flag suspect middle levels
    flags = [""] * len(stats)
    for i in range(1, len(stats) - 1):  # skip L1 (first) and memory (last)
        if is_suspect(stats[i - 1], stats[i], dram_stats):
            flags[i] = "?"

    cyc_hdr = f"  {'cycles':>13s}" if args.ghz else ""
    print(f"  {'level':6s}  {'buffer range':>16s}  {'latency (ns)':>14s}{cyc_hdr}")
    print(f"  {'-'*6}  {'-'*16}  {'-'*14}" + ("  " + "-" * 13 if args.ghz else ""))

    for name, st, flag in zip(names, stats, flags):
        lbl = name + flag
        buf_str = (humanize(st["min_buf"]) if st["min_buf"] == st["max_buf"]
                   else f"{humanize(st['min_buf'])} – {humanize(st['max_buf'])}")
        lat_str = (f"{st['lat_lo']:.1f}" if st["lat_lo"] == st["lat_hi"]
                   else f"{st['lat_lo']:.1f} – {st['lat_hi']:.1f}")
        cyc_str = ""
        if args.ghz:
            lo = st["lat_lo"] * args.ghz
            hi = st["lat_hi"] * args.ghz
            val = f"{lo:.0f}" if lo == hi else f"{lo:.0f} – {hi:.0f}"
            cyc_str = f"  {val:>13s}"
        print(f"  {lbl:6s}  {buf_str:>16s}  {lat_str:>14s}{cyc_str}")

    print()
    print("  derived")

    l1_max = stats[0]["max_buf"] if names[0] == "L1" else None
    l1_lat = stats[0]["lat_lo"] if names[0] == "L1" else None
    l2_max = next((st["max_buf"] for n, st in zip(names, stats) if n == "L2"), None)
    dram_min = dram_stats["min_buf"] if dram_stats and len(stats) > 1 else None

    nothing_derived = True

    def plural(n, word):
        return f"{n} {word}" if n == 1 else f"{n} {word}s"

    if kind == "random" and l1_max and l2_max:
        ls, n = line_size(data, l1_max, l2_max)
        if ls and n >= 3:
            print(f"    line size           {ls} B            (consensus: {plural(n, 'row')})")
            nothing_derived = False

    if kind == "random" and l1_max and l1_lat:
        a, n = l1_associativity(data, l1_lat, l1_max)
        if a and n >= 4:
            print(f"    L1 associativity    {a}-way           (consensus: {plural(n, 'row')})")
            nothing_derived = False
        elif a:
            print(f"    L1 associativity    {a}-way  (low confidence: only {plural(n, 'row')})")
            nothing_derived = False

    if kind == "random" and dram_min:
        pen = tlb_penalty(data, dram_min)
        if pen and pen > 5:
            print(f"    TLB miss penalty   ~{pen:.0f} ns          (at {humanize(dram_min)} buffer)")
            nothing_derived = False

    if nothing_derived:
        if kind == "random":
            print("    (not enough resolution in this dataset)")
        else:
            print("    (line size, associativity, TLB need random-access data;")
            print("     the hardware prefetcher hides them in sequential walks)")

    # Notes
    notes = []
    if any(f == "?" for f in flags):
        notes.append("levels marked '?' are weak signals — possibly TLB pressure,")
        notes.append("noisy plateaus, or boundary effects, not separate caches")
    # Boundary contamination note for inner levels
    for name, st in zip(names, stats):
        if name in ("L2", "L3") and st["lat_hi"] / st["lat_lo"] > 1.5:
            notes.append(f"{name} latency floor ({st['lat_lo']:.1f} ns) is at the previous-level boundary;")
            notes.append(f"  typical {name} hit cost in mid-region is closer to ~{(st['lat_lo']+st['lat_hi'])/2:.0f} ns")
            break
    if notes:
        print()
        print("  notes")
        for n in notes:
            print(f"    {n}")


def plot_title(file_path, meta):
    bits = []
    if "host" in meta:
        bits.append(meta["host"])
    if meta.get("kind") in ("random", "sequential"):
        bits.append(meta["kind"])
    return " · ".join(bits) if bits else file_path


def plot_2d(data, meta, file_path, output, n_lines=10):
    """One line per buffer size: stride vs latency."""
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    import numpy as np

    buffers = sorted(data)
    if len(buffers) > n_lines:
        idx = np.linspace(0, len(buffers) - 1, n_lines).round().astype(int)
        buffers = [buffers[i] for i in idx]

    fig, ax = plt.subplots(figsize=(10, 6))
    cmap = plt.get_cmap("plasma")
    for i, buf in enumerate(buffers):
        rows = data[buf]
        strides = sorted(rows)
        lats = [rows[s] for s in strides]
        color = cmap(i / max(1, len(buffers) - 1))
        ax.plot(strides, lats, marker=".", linewidth=1.3,
                color=color, label=humanize(buf))

    ax.set_xscale("log", base=2)
    ax.set_xlabel("Stride (bytes)")
    ax.set_ylabel("Latency (ns)")
    ax.grid(True, which="both", alpha=0.3)
    ax.legend(loc="best", title="Buffer size", fontsize=8, ncol=2)
    ax.set_title(f"Cache latency — {plot_title(file_path, meta)}")
    fig.tight_layout()
    fig.savefig(output, dpi=110)
    print(f"wrote {output}")


def plot_3d(data, meta, file_path, output):
    """Surface plot: stride × buffer × latency."""
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    import numpy as np

    buffers = sorted(data)
    all_strides = sorted({s for rows in data.values() for s in rows})

    Z = np.full((len(buffers), len(all_strides)), np.nan)
    for i, buf in enumerate(buffers):
        for j, stride in enumerate(all_strides):
            if stride in data[buf]:
                Z[i, j] = data[buf][stride]

    X, Y = np.meshgrid(all_strides, buffers)

    fig = plt.figure(figsize=(11, 7.5))
    ax = fig.add_subplot(111, projection="3d")
    surf = ax.plot_surface(np.log2(X), np.log2(Y), Z, cmap="plasma",
                           edgecolor="white", linewidth=0.25, alpha=0.95,
                           antialiased=True)

    def log_ticks(values, label_fn):
        return [np.log2(v) for v in values], [label_fn(v) for v in values]

    smax = max(all_strides)
    stride_marks = [v for v in (8, 128, 2048, 32768, 524288, 8388608,
                                134217728, 1073741824) if v <= smax]
    sx, sxl = log_ticks(stride_marks, humanize)
    ax.set_xticks(sx)
    ax.set_xticklabels(sxl, fontsize=8)

    bmax = max(buffers)
    buf_marks = [v for v in (8192, 131072, 2097152, 33554432, 536870912)
                 if v <= bmax]
    by, byl = log_ticks(buf_marks, humanize)
    ax.set_yticks(by)
    ax.set_yticklabels(byl, fontsize=8)

    ax.set_xlabel("Stride", labelpad=8)
    ax.set_ylabel("Buffer size", labelpad=8)
    ax.set_zlabel("Latency (ns)", labelpad=4)
    ax.set_title(f"Cache latency — {plot_title(file_path, meta)}")
    ax.view_init(elev=22, azim=-115)

    fig.colorbar(surf, shrink=0.5, aspect=12, pad=0.08, label="ns")
    fig.tight_layout()
    fig.savefig(output, dpi=110)
    print(f"wrote {output}")


def default_output(input_path, plot_kind):
    import os
    base = os.path.splitext(os.path.basename(input_path))[0]
    return f"{base}.{plot_kind}.png"


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("file")
    ap.add_argument("--ghz", type=float,
                    help="CPU clock in GHz; adds a cycle-count column")
    ap.add_argument("--plot", choices=["2d", "3d"],
                    help="produce a plot instead of (in addition to) the report")
    ap.add_argument("-o", "--output",
                    help="plot output file (default: <input>.<plot>.png)")
    args = ap.parse_args()

    meta, data = parse(args.file)
    if not data:
        sys.exit(f"{args.file}: no data parsed")

    if args.plot:
        out = args.output or default_output(args.file, args.plot)
        if args.plot == "2d":
            plot_2d(data, meta, args.file, out)
        else:
            plot_3d(data, meta, args.file, out)
    else:
        report(args, meta, data)


if __name__ == "__main__":
    main()
