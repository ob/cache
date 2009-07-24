#set term x11
set ticslevel 0
set xrange [] reverse
set xtics (2, 8, 32, 128, 512, "2K" 2048, "8K" 8192, "32K" 32768, "128K" 131072, \
    "512K" 524288, "2M" 2097152, "8M" 8388608, "32M" 33554432, "128M" 134217728, \
    "512M" 536870912)
set ytics (2, 8, 32, 128, 512, "2K" 2048, "8K" 8192, "32K" 32768, "128K" 131072, \
    "512K" 524288, "2M" 2097152, "8M" 8388608, "32M" 33554432, "128M" 134217728, \
    "512M" 536870912)
set grid
unset key
set title "Cache Latency"
set xlabel "Stride (bytes)"
set ylabel "Buffer Size (bytes)"
set zlabel "Time (ns)"
#set view 57,13
set view 70,30
#set view 46, 345
set pm3d at s hidden3d 100
set style line 100 lt 5 lw 0.5
unset surf
set log xy # add z to use log scale for time
#set log z
splot "OUT" with lines

# Local Variables: **
# mode: gnuplot **
# End: **
