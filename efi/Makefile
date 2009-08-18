# EFI Build Environment for Mac OS X
# www.osxbook.com
#
# Makefile for EFI applications
#

# Defaults
#
ARCH     = ia32
EFIROOT  = /usr/local
HDRROOT  = $(EFIROOT)/include/efi
INCLUDES = -I. -I$(HDRROOT) -I$(HDRROOT)/$(ARCH) -I$(HDRROOT)/protocol

CRTOBJS  = $(EFIROOT)/lib/crt0-efi-$(ARCH).o
CFLAGS   = -O2 -fpic -Wall -fshort-wchar -fno-strict-aliasing \
           -fno-merge-constants
CPPFLAGS = -DCONFIG_$(ARCH)
FORMAT   = efi-app-$(ARCH)
INSTALL  = install
LDFLAGS  = -nostdlib
LDSCRIPT = $(EFIROOT)/lib/elf_$(ARCH)_efi.lds
LDFLAGS += -T $(LDSCRIPT) -shared -Bsymbolic -L$(EFIROOT)/lib $(CRTOBJS)
LOADLIBS = -lefi -lgnuefi $(shell $(CC) -print-libgcc-file-name)

# Toolchain prefix
#
prefix      = i686-osxbook-linux-gnu-
CC          = $(prefix)gcc
AS          = $(prefix)as
LD          = $(prefix)ld
AR          = $(prefix)ar
RANLIB      = $(prefix)ranlib
OBJCOPY     = $(prefix)objcopy

# Rules
#
%.efi: %.so
	$(OBJCOPY) -j .text -j .sdata -j .data -j .dynamic -j .dynsym -j .rel \
                   -j .rela -j .reloc --target=$(FORMAT) $*.so $@

%.so: %.o
	$(LD) $(LDFLAGS) $^ -o $@ $(LOADLIBS)
	$(RM) $<

%.o: %.c Makefile
	$(CC) $(INCLUDES) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

# Targets
#
TARGETS = cache.efi hello.efi

all: $(TARGETS)

clean:
	rm -f $(TARGETS)
