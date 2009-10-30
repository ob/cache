#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <getopt.h>
#include <sys/time.h>
#include <sys/types.h>

typedef	u_int8_t	u8;
typedef	u_int32_t	u32;
typedef u_int64_t	u64;

#define	SAMPLE		10
#define	CACHE_MIN	(1024 / sizeof(u32*))		      /* 1K */
#define	CACHE_MAX	((1024 * 1024 * 1024) / sizeof(u32*)) /* 1 GB! */

/* Getopt */
static struct option longopts[] = {
    { "random",		no_argument,		NULL,		'r' },
    { "sequential",	no_argument,		NULL,		's' },
    { "buffer",		required_argument,	NULL,		'b' },
    { NULL,		0,			NULL,		0 }
};

typedef enum {rand_access, seq_access} access_kind;

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
    for (i = 1, j = 0; i <= max; i+=stride, j++) indices[j] = i;
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

void
random_access(u32 cache_max)
{
    u32 register	i, stride;
    u32	steps, csize, limit;
    double	sec0, sec;
    u32 **start, **p;

    for (csize=CACHE_MIN; csize <= cache_max; csize*=2) {
	for (stride=1; stride <= csize/2; stride=stride*2) {
	    sec = 0.0;
	    limit = csize - stride + 1;
	    steps = 0;
	    start = shuffle(buffer, stride, limit);
	    do {
		sec0 = timestamp();
		for (i=SAMPLE*stride; i > 0; i--) {
		    for (p = start; p; p = (u32 **)*p) {}
		}
		steps++;
		sec += timestamp() - sec0;
	    } while (sec < 1.0);

	    printf("%lu\t%lu\t%.1lf\n",
		   stride * sizeof(u32*),
		   csize * sizeof(u32*),
		   sec*1e9/(steps*SAMPLE*stride*((limit-1)/stride+1)));
	    fflush(stdout);
	}
	printf("\n\n");
	fflush(stdout);
    }
}

void
sequential_access(u32 cache_max)
{
    u32 register	i, j, stride;
    u32 steps, csize, limit;
    double	sec0, sec;

    for (csize=CACHE_MIN; csize <= cache_max; csize*=2) {
	for (stride=1; stride <= csize/2; stride=stride*2) {
	    sec = 0.0;
	    limit = csize - stride + 1;
	    steps = 0;
	    do {
		sec0 = timestamp();
		for (i=SAMPLE*stride; i > 0; i--) {
		    for (j=1; j <= limit; j+=stride) {
			buffer[j] = buffer[j] + 1;
		    }
		}
		steps++;
		sec += timestamp() - sec0;
	    } while (sec < 1.0);

	    printf("%lu\t%lu\t%.1lf\n",
		   stride * sizeof(u32*),
		   csize * sizeof(u32*),
		   sec*1e9/(steps*SAMPLE*stride*((limit-1)/stride+1)));
	    fflush(stdout);
	}
	printf("\n\n");
	fflush(stdout);
    }
}

int
main(int ac, char **av)
{
    int	c;
    u32	cache_size;
    access_kind	kind;

    cache_size = CACHE_MAX;
    while ((c = getopt_long(ac, av, "rsc:", longopts, NULL)) != -1) {
	switch (c) {
	case 'r':
	    kind = rand_access;
	    break;
	case 's':
	    kind = seq_access;
	    break;
	case 'b':
	    cache_size = strtol(optarg, 0, 10) / sizeof(u32*);
	    break;
	default:
usage:	    fprintf(stderr, "usage: %s (--random|--sequential) "
		    "[--buffer <size>]\n", av[0]);
	    return (1);
	    break;
	}
    }
    if (cache_size < CACHE_MIN) {
	fprintf(stderr, "%s: buffer needs to be at least %ld\n",
		av[0], CACHE_MIN * sizeof(u32*));
	return (1);
    }
    if (cache_size > CACHE_MAX) {
	fprintf(stderr, "%s: buffer needs to be less than %ld\n",
		av[0], CACHE_MAX * sizeof(u32*));
	return (1);
    }
    switch (kind) {
    case rand_access:
	printf("# doing random access with a max buffer of %ld\n",
	       cache_size * sizeof(u32*));
	random_access(cache_size);
	break;
    case seq_access:
	printf("# doing sequential access with a max buffer of %ld\n",
	       cache_size * sizeof(u32*));
	sequential_access(cache_size);
	break;
    default:
	goto usage;
	break;
    }
    return (0);
}

/*
 * Local Variables: **
 * c-file-style: "cc-mode" **
 * End: **
 */
