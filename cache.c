#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <getopt.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>

typedef	u_int8_t	u8;
typedef	u_int32_t	u32;
typedef u_int64_t	u64;

#define	SAMPLE		10
#define	CACHE_MIN	(1024 / sizeof(u32*))		      /* 1K of pointers */
#define	CACHE_MAX	((1024 * 1024 * 1024) / sizeof(u32*)) /* 1 GB of pointers */

static struct option longopts[] = {
    { "buffer",		required_argument,	NULL,		'b' },
    { "output",		required_argument,	NULL,		'o' },
    { NULL,		0,			NULL,		0 }
};

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
    u32 i, j, r, n, tmp;
    static int	*indices = NULL;

    if (indices == NULL) {
	indices = calloc(CACHE_MAX, sizeof(int));
	if (indices == NULL) {
	    fprintf(stderr, "not enough memory\n");
	    exit(1);
	}
    } else {
	bzero(indices, CACHE_MAX * sizeof(int));
    }
    for (i = 1, j = 0; i <= max; i+=stride, j++) indices[j] = i;
    n = j;
    while (n > 1) {
	n--;
	r = random() % n;
	tmp = indices[r];
	indices[r] = indices[n];
	indices[n] = tmp;
    }
    for (i = 0; i < j-1; i++) {
	buffer[indices[i]] = (u32 *)&buffer[indices[i+1]];
    }
    buffer[indices[i]] = NULL;
    return (&buffer[indices[0]]);
}

void
random_access(u32 cache_max, FILE *out)
{
    u32 register	i, stride;
    u32	steps, csize, limit;
    double	sec0, sec;
    u32 register	**start, **p;

    for (csize=CACHE_MIN; csize <= cache_max; csize*=2) {
	for (stride=1; stride <= csize/2; stride=stride*2) {
	    sec = 0.0;
	    limit = csize - stride + 1;
	    steps = 0;
	    start = shuffle(buffer, stride, limit);
	    do {
		sec0 = timestamp();
		for (i=SAMPLE*stride; i > 0; i--)
		    for (p = start; p; p = (u32 **)*p) ;
		steps++;
		sec += timestamp() - sec0;
	    } while (sec < 1.0);

	    fprintf(out, "%lu\t%lu\t%.1lf\n",
		    stride * sizeof(u32*),
		    csize * sizeof(u32*),
		    sec*1e9/(steps*SAMPLE*stride*((limit-1)/stride+1)));
	    fflush(out);
	}
	fprintf(out, "\n\n");
	fflush(out);
    }
}

static void
short_hostname(char *buf, size_t n)
{
    char *dot;
    if (gethostname(buf, n) != 0) {
	snprintf(buf, n, "unknown");
	return;
    }
    buf[n - 1] = '\0';
    if ((dot = strchr(buf, '.')) != NULL) *dot = '\0';
}

int
main(int ac, char **av)
{
    int	c;
    u32	cache_size = CACHE_MAX;
    const char	*output_path = NULL;
    char	default_path[512];
    char	hostname[256];
    time_t	now;
    FILE	*out;

    while ((c = getopt_long(ac, av, "b:o:", longopts, NULL)) != -1) {
	switch (c) {
	case 'b':
	    cache_size = strtol(optarg, 0, 10) / sizeof(u32*);
	    break;
	case 'o':
	    output_path = optarg;
	    break;
	default:
	    fprintf(stderr, "usage: %s [--buffer <size>] [--output <path>]\n",
		    av[0]);
	    return (1);
	}
    }
    if (cache_size < CACHE_MIN || cache_size > CACHE_MAX) {
	fprintf(stderr, "%s: buffer must be %ld–%ld bytes\n",
		av[0], CACHE_MIN * sizeof(u32*), CACHE_MAX * sizeof(u32*));
	return (1);
    }

    short_hostname(hostname, sizeof(hostname));
    now = time(NULL);

    if (output_path == NULL) {
	char timestamp[32];
	strftime(timestamp, sizeof(timestamp), "%Y%m%d-%H%M%S", localtime(&now));
	snprintf(default_path, sizeof(default_path),
		 "results/%s-%s", hostname, timestamp);
	output_path = default_path;
    }

    out = fopen(output_path, "w");
    if (out == NULL) {
	fprintf(stderr, "%s: cannot open %s: ", av[0], output_path);
	perror(NULL);
	return (1);
    }

    fprintf(out, "# host: %s\n", hostname);
    fprintf(out, "# date: %.24s\n", ctime(&now));
    fprintf(out, "# doing random access with a max buffer of %ld\n",
	    cache_size * sizeof(u32*));
    fflush(out);

    fprintf(stderr, "writing to %s\n", output_path);
    random_access(cache_size, out);

    fclose(out);
    return (0);
}

/*
 * Local Variables: **
 * c-file-style: "cc-mode" **
 * End: **
 */
