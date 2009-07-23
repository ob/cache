#include <stdio.h>
#include <sys/times.h>
#include <sys/types.h>
#include <time.h>

#define	CACHE_MIN	(1024)
#define	CACHE_MAX	(4*1024*1024)
#define	SAMPLE		10

#ifndef	CLK_TCK
#define	CLK_TCK	60
#endif

int	x[CACHE_MAX];

double
get_seconds()
{
    struct	tms rusage;
    times(&rusage);
    return (double)(rusage.tms_utime)/CLK_TCK;
}

int
main()
{
    int	register	i, j, stride, limit, temp;
    int	steps, tsteps, csize;
    double	sec0, sec;

    for(csize=CACHE_MIN; csize <= CACHE_MAX; csize*=2) {
	for (stride=1; stride <= csize/2; stride*=2) {
	    sec = 0;
	    limit = csize-stride+1;
	    steps = 0;
	    do {
		sec0 = 0;
		for (i=SAMPLE*stride; i != 0; i--) {
		    for(j=1;j <= limit; j+=stride) {
			x[j] = x[j] + 1;
		    }
		}
		steps++;
		sec += get_seconds() - sec0;
	    } while (sec < 1.0);

	    tsteps = 0;
	    do {
		sec0 = get_seconds();
		for (i=SAMPLE*stride; i != 0; i--) {
		    for(j=1; j <= limit; j+=stride) {
			temp = temp + 1;
		    }
		}
		tsteps++;
		sec -= get_seconds() - sec0;
	    } while (tsteps < steps);
	    printf("%7d\t%7d\t%14.0f\n",
		   stride*sizeof(int),
		   csize*sizeof(int),
		   sec*1e9/(steps*SAMPLE*stride*((limit-1)/stride+1)));
	}
	printf("\n");
    }
}

/*
 * Local Variables: **
 * c-file-style: "cc-mode" **
 * End: **
 */
