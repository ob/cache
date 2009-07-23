#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>

#define	SAMPLE		10
#define	CACHE_MIN	4 * 1024
#define	CACHE_MAX	512 * 1024 * 1024

typedef u_int8_t	u8;
typedef	u_int32_t	u32;
typedef u_int64_t	u64;

u8			x[CACHE_MAX];

double
timestamp(void)
{
    struct timeval	tv;
    gettimeofday(&tv, 0);
    return (tv.tv_sec + (tv.tv_usec/1000000.0));
}

int
main(int ac, char **av)
{
    u32 register	i, j, k, stride, temp;
    u32	steps, limit, tsteps, csize;
    double	sec0, sec;

    for (csize=CACHE_MIN; csize <= CACHE_MAX; csize*=2) {
	for (stride=1; stride <= csize; stride=stride*2) {
	    sec = 0.0;
	    steps = 0;
	    do {
		sec0 = timestamp();
		for (i=SAMPLE*stride; i > 0; i--) {
		    for (j=0; j < csize; j+=stride) {
			x[j] += 1;
		    }
		}
		steps++;
		sec += timestamp() - sec0;
	    } while (sec < 1.0);

	    tsteps = 0;
	    do {
		sec0 = timestamp();
		for (i=SAMPLE*stride; i > 0; i--) {
		    for (j=0; j < csize; j+=stride) {
			temp += j;
		    }
		}
		tsteps++;
		sec -= timestamp() - sec0;
	    } while (tsteps < steps);
	    printf("%u\t%u\t%.1lf\n",
		   stride,
		   csize,
		   (sec * 1000000000.0) /
		   (SAMPLE * stride * steps * (csize / stride)));
	    fflush(stdout);
	}
	printf("\n");
	fflush(stdout);
    }
}

/*
 * Local Variables: **
 * c-file-style: "cc-mode" **
 * End: **
 */
