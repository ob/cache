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
plot "KNOPPIX_SEQ_DIRAC" index 1 using 1:3 with lines title "8K", \
     "KNOPPIX_SEQ_DIRAC" index 2 using 1:3 with lines title "16K", \
     "KNOPPIX_SEQ_DIRAC" index 3 using 1:3 with lines title "32K", \
     "KNOPPIX_SEQ_DIRAC" index 4 using 1:3 with lines title "64K", \
     "KNOPPIX_SEQ_DIRAC" index 9 using 1:3 with lines title "2M", \
     "KNOPPIX_SEQ_DIRAC" index 10 using 1:3 with lines title "4M", \
     "KNOPPIX_SEQ_DIRAC" index 11 using 1:3 with lines title "8M", \
     "KNOPPIX_SEQ_DIRAC" index 14 using 1:3 with lines title "64M", \
     "KNOPPIX_SEQ_DIRAC" index 15 using 1:3 with lines title "128M", \
     "KNOPPIX_SEQ_DIRAC" index 17 using 1:3 with lines title "512M", \
     "KNOPPIX_SEQ_DIRAC" index 18 using 1:3 with lines title "1GB"
