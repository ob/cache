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
set title "Linux 2.6 - Sequential (>L2)"
set xlabel "Stride (bytes)"
set ylabel "Time (ns)"
plot "KNOPPIX_RAND_DIRAC" index 11 using 1:3 with lines title "8M", \
     "KNOPPIX_RAND_DIRAC" index 12 using 1:3 with lines title "16M", \
     "KNOPPIX_RAND_DIRAC" index 13 using 1:3 with lines title "32M", \
     "KNOPPIX_RAND_DIRAC" index 14 using 1:3 with lines title "64M", \
     "KNOPPIX_RAND_DIRAC" index 15 using 1:3 with lines title "128M", \
     "KNOPPIX_RAND_DIRAC" index 16 using 1:3 with lines title "256M", \
     "KNOPPIX_RAND_DIRAC" index 17 using 1:3 with lines title "512M", \
     "KNOPPIX_RAND_DIRAC" index 18 using 1:3 with lines title "1GB"
