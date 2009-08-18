#include <efi.h>
#include <efilib.h>
#include <efiapi.h>

typedef	UINT8	u8;
typedef	UINT32	u32;
typedef UINT64	u64;

#define	SAMPLE		10
#define	CACHE_MIN	1024
#define	CACHE_MAX	256 * 1024 * 1024 /* 1 GB! */

#define	MAX_ARGS	32	/* arbitrary */

#define	ONE_SEC		1000000000 /* 1G ns */

u32*	buffer;

INTN	populate_argv(CHAR16 *buf, UINTN len, CHAR16 **argv);

void	sequential_access(u32 cache_max);

/* Most of the idioms here were cribbed from the osx book */
EFI_STATUS
efi_main(EFI_HANDLE image, EFI_SYSTEM_TABLE *systab)
{
    CHAR16		*av[MAX_ARGS];
    INTN		ac = 0;
    CHAR16		*rawargs = NULL;
    EFI_LOADED_IMAGE	*info;
    EFI_STATUS		status;
    UINTN		cache_size;

    InitializeLib(image, systab);


    cache_size = CACHE_MAX;

    // Prepare to gather command-line arguments. First, retrieve image info.
    status = BS->HandleProtocol(image, &LoadedImageProtocol, (VOID *)&info);
    if (EFI_ERROR(status)) {
	Print(L"%r = BS->HandleProtocol(..., &LoadedImageProtocol, ...)\n",
	      status);
	return status;
    }

    // Allocate memory for processing command-line arguments.
    status = BS->AllocatePool(EfiLoaderData,
			      info->LoadOptionsSize + sizeof(CHAR16),
			      (VOID *)&rawargs);
    if (status != EFI_SUCCESS) {
	Print(L"%r = BS->AllocatePool(args)\n", status);
	return status;
    }

    // Populate C-style ac and av.
    CopyMem(rawargs, info->LoadOptions, info->LoadOptionsSize);
    ac = populate_argv(rawargs, info->LoadOptionsSize, av);


    if (ac != 2) {
	cache_size = CACHE_MAX;
    } else {
	cache_size = Atoi((CHAR16 *)av[1]);
    }

    if (cache_size < CACHE_MIN) {
	Print(L"%s: buffer needs to be at least %d\n",
		av[0], CACHE_MIN * sizeof(u32));
	status = EFI_INVALID_PARAMETER;
	goto out;
    }
    if (cache_size > CACHE_MAX) {
	Print(L"%s: buffer needs to be less than %d\n",
		av[0], CACHE_MAX * sizeof(u32));
	status = EFI_INVALID_PARAMETER;
	goto out;
    }
    Print(L"# doing sequential access with a max buffer of %d\n",
	       cache_size * sizeof(u32));

    IPrint(ST->StdErr, L"# doing sequential access with a max buffer of %d\n",
	       cache_size * sizeof(u32));

    status = BS->AllocatePool(EfiLoaderData,
			      cache_size * sizeof(u32), (void **)&buffer);
    if (status != EFI_SUCCESS) {
	Print(L"%r = BS->AllocatePool(buffer)\n", status);
	return status;
    }
    sequential_access(cache_size);
    status = EFI_SUCCESS;
out:
    return status;
}


u64
nanoseconds(void)
{
    EFI_TIME_CAPABILITIES	cap;
    EFI_TIME			time;
    EFI_STATUS			status;

    status = RT->GetTime(&time, &cap);
    if (status != EFI_SUCCESS) {
	Print(L"%r = GetTime()\n", status);
	return 0;
    }
    /* This is kind of a hack since we only expect to spend at most a
     * couple of sec. DO NOT RUN THIS CODE AT ANY O'CLOCK TIME. */
    return (time.Minute * 60 * 1000000000 +
	    time.Second * 1000000000 + time.Nanosecond);
    /* time.Nanosecons is normally useless (0), but I include it anyway */
}

void
sequential_access(UINTN cache_max)
{
    u32 register	i, j, stride;
    u32 steps, csize, limit;
    u32	nsec0, nsec;

    for (csize=CACHE_MIN; csize <= cache_max; csize*=2) {
	Print(L"csize = %d\n", csize);
	for (stride=1; stride <= csize/2; stride=stride*2) {
	    nsec = 0;
	    limit = csize - stride + 1;
	    steps = 0;
	    do {
		nsec0 = nanoseconds();
		for (i=SAMPLE*stride; i > 0; i--) {
		    for (j=1; j <= limit; j+=stride) {
			buffer[j] = buffer[j] + 1;
		    }
		}
		steps++;
		nsec += nanoseconds() - nsec0;
	    } while (nsec < ONE_SEC); /* gather 2 sec */

	    IPrint(ST->StdErr, L"%d\t%d\t%d\n",
		   stride * sizeof(u32),
		   csize * sizeof(u32),
/* no floatng point */
		  nsec / (steps*SAMPLE*stride*((limit-1)/stride+1)));
	    Print(L"%d\t%d\t%d\n",
		   stride * sizeof(u32),
		   csize * sizeof(u32),
/* no floatng point */
		  nsec / (steps*SAMPLE*stride*((limit-1)/stride+1)));
	}
	IPrint(ST->StdErr, L"\n\n");
	Print(L"\n\n");
    }
}


// the following function is stolen/adapted from elilo's argify()
//
// Copyright (C) 2001-2003 Hewlett-Packard Co.
//     Contributed by Stephane Eranian <eranian@hpl.hp.com>
//  Copyright (C) 2001 Silicon Graphics, Inc.
//      Contributed by Brent Casavant <bcasavan@sgi.com>

INTN
populate_argv(CHAR16 *buf, UINTN len, CHAR16 **argv)
{
    UINTN   i = 0;
    UINTN   j = 0;
    CHAR16 *p = buf;
#define CHAR_SPACE L' '

    if (buf == 0) {
	argv[0] = NULL;
	return 0;
    }

// len is the number of bytes (and not the number of CHAR16's)
    len = len >> 1;

// Here we use CHAR_NULL as the terminator rather than the length because
// the EFI shell returns rather bogus values for it. Apparently, we are
// guaranteed to find '\0' in the buffer where the real input arguments
// stop, so we use it instead.
//
    for(;;) {
	while (buf[i] == CHAR_SPACE && buf[i] != CHAR_NULL && i < len)
	    i++;

	if (buf[i] == CHAR_NULL || i == len)
	    goto end;

	p = buf+i;
	i++;

	while (buf[i] != CHAR_SPACE && buf[i] != CHAR_NULL && i < len)
	    i++;

	argv[j++] = p;

	if (buf[i] == CHAR_NULL)
	    goto end;

	buf[i] = CHAR_NULL;

	if (i == len)
	    goto end;

	i++;

	if (j == (MAX_ARGS - 1)) {
	    Print(L"%s: too many arguments (%d), truncating", argv[0], j);
	    goto end;
	}
    }

  end:
    argv[j] = NULL;
    return j;
}

/*
 * Local Variables: **
 * c-file-style: "cc-mode" **
 * End: **
 */
