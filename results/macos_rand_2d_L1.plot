# Local Variables: **
# mode: gnuplot **
# End: **
#set term x11
set xtics (2, 8, 32, 128, 512, "2K" 2048, "8K" 8192, "32K" 32768, \
	"128K" 131072, "512K" 524288, "2M" 2097152, "8M" 8388608, \
        "32M" 33554432, "128M" 134217728, \
        "512M" 536870912)
# set yrange [0:140]
set grid
set log x
set title "Mac OS 10.5.6 - Random (L1)"
set xlabel "Stride (bytes)"
set ylabel "Time (ns)"
plot "MACOS_RAND_REBOOT" index 0 using 1:3 with lines title "4K", \
     "MACOS_RAND_REBOOT" index 1 using 1:3 with lines title "8K", \
     "MACOS_RAND_REBOOT" index 2 using 1:3 with lines title "16K", \
     "MACOS_RAND_REBOOT" index 3 using 1:3 with lines title "32K", \
     "MACOS_RAND_REBOOT" index 4 using 1:3 with lines title "64K"
