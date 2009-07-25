#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/time.h>
#include <sys/types.h>

#define	SAMPLE		10
#define	CACHE_MIN	1024
#define	CACHE_MAX	256 * 1024 * 1024 /* 1 GB! */

typedef	u_int8_t	u8;
typedef	u_int32_t	u32;
typedef u_int64_t	u64;

u32*	buffer[CACHE_MAX];

double
timestamp(void)
{
    struct timeval	tv;
    gettimeofday(&tv, 0);
    return (tv.tv_sec + (tv.tv_usec/1000000.0));
}

u32 **
shuffle(u32 **buffer, u32 stride, u32 max)
{
    int i, j, r, n, tmp;
    static int	*indices = NULL;

    if (indices == NULL) {
	indices = calloc(CACHE_MAX, sizeof(int));
	if (indices == NULL) {
	    printf("not enough memory\n");
	    exit(1);
	}
    } else {
	bzero(indices, CACHE_MAX);
    }
    for (i = 0, j = 0; i < max; i+=stride, j++) indices[j] = i;
/* shuffle it */
     n = j;
     while (n > 1) {
	 n--;
	 r = random() % n;
	 tmp = indices[r];
	 indices[r] = indices[n];
	 indices[n] = tmp;
     }
/* build linked list */
     for (i = 0; i < j-1; i++) {
	 buffer[indices[i]] = (u32 *)&buffer[indices[i+1]];
     }
     buffer[indices[i]] = NULL;
     return (&buffer[indices[0]]);
}

int
main(int ac, char **av)
{
    u32 register	i, stride;
    u32	steps, csize;
    double	sec0, sec;
    u32 **start, **p;

    for (csize=CACHE_MIN; csize <= CACHE_MAX; csize*=2) {
	for (stride=1; stride <= csize; stride=stride*2) {
	    sec = 0.0;
	    steps = 0;
	    start = shuffle(buffer, stride, csize);
	    do {
		sec0 = timestamp();
		for (i=SAMPLE*stride; i > 0; i--) {
		    for (p = start; p; p = (u32 **)*p) {}
		}
		steps++;
		sec += timestamp() - sec0;
	    } while (sec < 1.0);

	    printf("%lu\t%lu\t%.1lf\n",
		   stride * sizeof(u32),
		   csize * sizeof(u32),
		   (sec * 1000000000.0) /
		   (SAMPLE * stride * steps * (csize / stride)));
	    fflush(stdout);
	}
	printf("\n");
	fflush(stdout);
    }
    return 0;
}

/*
 * Local Variables: **
 * c-file-style: "cc-mode" **
 * End: **
 */
