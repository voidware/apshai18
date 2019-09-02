#
# Build Apshai18!
# 

SDCCDIR = i:/sdcc
CC = sdcc
AS = sdasz80
LD = sdldz80
DEBUG = 
OPT2 = --max-allocs-per-node 100000 
OPT = --all-callee-saves --opt-code-size
#OPT = --max-allocs-per-node 100000 
CFLAGS = -mz80 --std-sdcc11 --fsigned-char $(OPT) $(DEBUG)
ASFLAGS = -l

## set up your path to where the SDCC Z80 lib is
LIBS = -l $(SDCCDIR)/lib/z80/z80.lib

#LDFLAGS = -mjwx -b _CODE=0x4349 $(LIBS)

# DOS machine
LDFLAGS = -mjwx -b _CODE=0x5200 $(LIBS)

OBJS = \
	crt0.rel \
	apshai18.rel \
	plot.rel \
	os.rel \
	utils.rel \
	sound.rel \
	libc.rel \
	libcbase.rel \
	soundbit.rel \
	dungeon.rel 

%.rel: %.c
	$(CC) $(CFLAGS) -c $< 

%.rel: %.s
	$(AS) $(ASFLAGS) -o $@ $<

all: apshai18.dsk

# increase optimisation on plot functions
plot.rel: plot.c
	$(CC) $(CFLAGS) $(OPT2) -c $< 

apshai18.cas: apshai18.ihx
	../tools/mksys/mksys apshai18.ihx apshai18.cas


apshai18.ihx : $(OBJS) Makefile
	$(LD) $(LDFLAGS) -i apshai18.ihx $(OBJS)

apshai18.cmd: apshai18.cas
	../tools/trld/trld apshai18.cas apshai18.cmd

apshai18.dsk: apshai18.cmd
	rm -f apshai18.dsk
	cp ../emu/blank.dsk apshai18.dsk
	../tools/trswrite -o apshai18.dsk apshai18.cmd

apshai18.zip: 
	(cd ..; zip -r apshai18.zip readme.md emu doc src tools -x \*win32\* -x \*TAGS\*)

.PHONY:	 clean cleanall tags

clean:
	rm -f *.rel
	rm -f *.lk
	rm -f *.lst
	rm -f *~
	rm -f *.noi
	rm -f *.ihx
	rm -f *.map
	rm -f *.asm
	rm -f *.sym
	rm -f *.pdb
	rm -f *.ilk
	rm -f *.obj

cleanall: clean
	rm -f *.exe
	rm -f *.cmd
	rm -f *.cas
	rm -f *.dsk
	rm -f *.t8c


tags:
	ctags -e *.h *.c



