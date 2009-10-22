# Local Variables: **
# mode: gnuplot **
# End: **
#set term x11
set xtics (2, 8, 32, 128, 512, "2K" 2048, "8K" 8192, "32K" 32768, \
	"128K" 131072, "512K" 524288, "2M" 2097152, "8M" 8388608, \
        "32M" 33554432, "128M" 134217728, \
        "512M" 536870912)
set grid
set log x
set title "Linux 2.6 vs Mac OS 10.5.6 - Sequential (256M)"
set xlabel "Stride (bytes)"
set ylabel "Time (ns)"
plot "KNOPPIX_SEQ_DIRAC" index 16 using 1:3 with lines title "Linux (256M)", \
     "MACOS_SEQ_REBOOT" index 16 using 1:3 with lines title "Mac OS (256M)", \
     "KNOPPIX_SEQ_DIRAC" index 15 using 1:3 with lines title "Linux (128M)", \
     "MACOS_SEQ_REBOOT" index 15 using 1:3 with lines title "Mac OS (128M)"

