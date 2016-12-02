# Cache

## Memory-Hierarchy Understanding Tools

These are a few programs that benchmark different access patterns and
graph the results to understand the cache hierarchy of the computer
they are run on.

I first saw the technique used for generating these latency numbers in
exercise 5.2 on page 476 of
[Computer Architecture: A
Quantitative Approach](http://www.amazon.com/gp/product/0123704901)

The basic idea is that you write a program to walk a contiguous region
of memory using different strides and time how long accessing the
memory takes. The idea for this exercise comes from the
[Ph.D. dissertation](http://portal.acm.org/citation.cfm?id=170337) of
Rafael Héctor Saavedra‑Barrera, where he describes the following
approach:

> Assume that a machine has a cache capable of holding D 4‑byte
> words, a line size of b words, and an associativity
> a. The number of sets in the cache is given by D/ab. We
> also assume that the replacement algorithm is LRU, and that the lowest
> available address bits are used to select the cache set.
> 
> Each of our experiments consists of computing many times a simple
> floating-point function on a subset of elements taken from a
> one-dimensional array of N 4-byte elements. This subset is
> given by the following sequence:
> 
> 1, s+1, 2s+1, ..., N-s+1
> 
> Thus, each experiment is characterized by a particular value of
> N and s. The stride s allows us to change the rate at
> which misses are generated, by controlling the number of consecutive
> accesses to the same cache line, cache set, etc. The magnitude of
> s varies from 1 to N/2 in powers of two.

He goes on to note

> Depending on the magnitudes of N and s in a particular
> experiment, with respect to the size of the cache (D), the line
> size (b), and the associativity (a), there are four
> possible regimes of operations; each of these is characterized by the
> rate at which misses occur in the cache. A summary of the
> characteristics of the four regimes is given in table 5.1.

And table 5.1 helpfully summarizes the regimes.

| Size of Array |    Stride    | Frequency of Misses | Time per Iteration |
|---------------|--------------|---------------------|--------------------|
| 1 ≤ N ≤ D     | 1 ≤ s  ≤ N/2 |      no misses      |        T           |
| D < N         | 1 ≤ s ≤ b    | one miss every b/s elements| T |
| D < N         | b ≤ s < N/a  | one miss every element | T + M |
| D < N         | N/a ≤ s ≤ N/2| no misses | T |

I thought it would be fun to try it, so I wrote a program to do
that.

I ran this program on my machine after rebooting in single-user mode
like Hennessy and Patterson suggest so that virtual addresses track
physical addresses a little closer, and with a little help from
gnuplot, I got images like this:

![alt text][seq]

[seq]: https://github.com/ob/cache/raw/master/images/seq.png "Sequential Access"

![alt text][random]

[random]: https://github.com/ob/cache/raw/master/images/random.png "Random Access"

