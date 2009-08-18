# Local Variables: **
# mode: gnuplot **
# End: **
#set term x11
set xtics (2, 8, 32, 128, 512, "2K" 2048, "8K" 8192, "32K" 32768, \
	"128K" 131072, "512K" 524288)
set grid
set log x
set title "Mac OS X - Random"
set xlabel "Stride (bytes)"
set ylabel "Time (ns)"
plot "MACOS_RAND_REBOOT" index 0 using 1:3 with lines title "4K", \
     "MACOS_RAND_REBOOT" index 1 using 1:3 with lines title "8K", \
     "MACOS_RAND_REBOOT" index 2 using 1:3 with lines title "16K", \
     "MACOS_RAND_REBOOT" index 3 using 1:3 with lines title "32K", \
     "MACOS_RAND_REBOOT" index 4 using 1:3 with lines title "64K", \
     "MACOS_RAND_REBOOT" index 5 using 1:3 with lines title "128K", \
     "MACOS_RAND_REBOOT" index 6 using 1:3 with lines title "256K", \
     "MACOS_RAND_REBOOT" index 7 using 1:3 with lines title "512K", \
     "MACOS_RAND_REBOOT" index 8 using 1:3 with lines title "1M"
