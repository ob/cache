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
set title "Linux - Sequential"
set xlabel "Stride (bytes)"
set ylabel "Time (ns)"
plot "LINUX_SEQ_REBOOT" index 1 using 1:3 with lines title "8K", \
     "LINUX_SEQ_REBOOT" index 2 using 1:3 with lines title "16K", \
     "LINUX_SEQ_REBOOT" index 3 using 1:3 with lines title "32K", \
     "LINUX_SEQ_REBOOT" index 4 using 1:3 with lines title "64K", \
     "LINUX_SEQ_REBOOT" index 9 using 1:3 with lines title "2M", \
     "LINUX_SEQ_REBOOT" index 10 using 1:3 with lines title "4M", \
     "LINUX_SEQ_REBOOT" index 11 using 1:3 with lines title "8M", \
     "LINUX_SEQ_REBOOT" index 14 using 1:3 with lines title "64M", \
     "LINUX_SEQ_REBOOT" index 15 using 1:3 with lines title "128M", \
     "LINUX_SEQ_REBOOT" index 17 using 1:3 with lines title "512M", \
     "LINUX_SEQ_REBOOT" index 18 using 1:3 with lines title "1GB"
