# Local Variables: **
# mode: gnuplot **
# End: **
#set term x11
set xtics (2, 8, 32, 128, 512, "2K" 2048, "8K" 8192, "32K" 32768, \
	"128K" 131072, "512K" 524288)
set grid
set log x
set title "Mac OS X - Sequential"
set xlabel "Stride (bytes)"
set ylabel "Time (ns)"
plot "MACOS_SEQ_REBOOT" index 9 using 1:3 with lines title "2M", \
     "MACOS_SEQ_REBOOT" index 10 using 1:3 with lines title "4M", \
     "MACOS_SEQ_REBOOT" index 11 using 1:3 with lines title "8M", \
     "MACOS_SEQ_REBOOT" index 12 using 1:3 with lines title "16M", \
     "MACOS_SEQ_REBOOT" index 13 using 1:3 with lines title "32M", \
     "MACOS_SEQ_REBOOT" index 14 using 1:3 with lines title "64M", \
     "MACOS_SEQ_REBOOT" index 15 using 1:3 with lines title "128M", \
     "MACOS_SEQ_REBOOT" index 16 using 1:3 with lines title "256M", \
     "MACOS_SEQ_REBOOT" index 17 using 1:3 with lines title "512M", \
     "MACOS_SEQ_REBOOT" index 18 using 1:3 with lines title "1G"
