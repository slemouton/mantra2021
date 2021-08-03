current: linux

# ----------------------- NT -----------------------

msw: debounce.dll scofo.dll
	cd ../patch/manoury-pluton/lib && nmake msw
	cd ../patch/dodge-song/lib && nmake msw

.SUFFIXES: .dll

PDNTCFLAGS = /W3 /WX /DNT /DPD /nologo
VC="\Program Files\Microsoft Visual Studio\Vc98"
PD="..\..\pd"

PDNTINCLUDE = /I.  /I$(VC)\include /I$(PD)\src

PDNTLDIR = $(VC)\lib
PDNTLIB = $(PDNTLDIR)\libc.lib \
	$(PDNTLDIR)\oldnames.lib \
	$(PDNTLDIR)\kernel32.lib \
	$(PD)\bin\pd.lib 

.c.dll:
	cl $(PDNTCFLAGS) $(PDNTINCLUDE) /c $*.c
	link /dll /export:$*_setup $*.obj $(PDNTLIB)
	del $*.obj $*.lib $*.exp

# ----------------------- LINUX ia64 -----------------------

linux:  debounce.l_ia64 debounce.l_i386 scofo.l_ia64 scofo.l_i386
	make -C ../patch/manoury-pluton/lib linux
	make -C ../patch/dodge-song/lib linux
	
.SUFFIXES: .l_ia64 .l_i386

LINUXCFLAGS = -DPD -O2 -funroll-loops -fomit-frame-pointer \
    -Wall -W -Wshadow -Wstrict-prototypes -Werror \
    -Wno-unused -Wno-parentheses -Wno-switch

LINUXINCLUDE = -I../../pd/src -I../../../pd/src -I../../../../pd/src \
  -I../../../../../pd/src -I../../../../../../pd/src \
  -I../../../../../../../pd/src

.c.l_i386:
	cc $(LINUXCFLAGS) $(LINUXINCLUDE) -m32 -o $*.o -c $*.c
	cc -m32  -export_dynamic -shared -o $*.l_i386 $*.o -lc -lm
	strip --strip-unneeded $*.l_i386

.c.l_ia64:
	cc $(LINUXCFLAGS) $(LINUXINCLUDE) -fPIC -o $*.o -c $*.c
	cc -export_dynamic -shared -o $*.l_ia64 $*.o -lc -lm
	strip --strip-unneeded $*.l_ia64
	rm $*.o


# ----------------------- Mac OSX -----------------------

mac: debounce.d_ppc debounce.d_fat scofo.d_ppc scofo.d_fat
	make -C ../patch/manoury-pluton/lib mac
	make -C ../patch/dodge-song/lib mac

.SUFFIXES: .d_ppc .d_fat

DINC=Pd.app/Contents/Resources/src/
DARWININCLUDE = -I../../$(DINC) -I../../../$(DINC) -I../../../../$(DINC) \
  -I../../../../../$(DINC) -I../../../../../../$(DINC) \
  -I../../../../../../../$(DINC)
DARWINCFLAGS = -DPD -O2 -Wall -W -Wshadow -Wstrict-prototypes \
    -Wno-unused -Wno-parentheses -Wno-switch

.c.d_ppc:
	cc -arch ppc $(DARWINCFLAGS) $(DARWININCLUDE) -o $*.o -c $*.c
	cc -arch ppc -bundle -undefined suppress -flat_namespace -o $*.d_ppc $*.o 
	rm -f $*.o
.c.d_fat:
	$(CC) -arch i386 -arch ppc $(DARWINCFLAGS) $(DARWININCLUDE) -o $*.o -c $*.c
	$(CC) -arch i386 -arch ppc -bundle -undefined suppress -flat_namespace \
	    -o $*.d_fat $*.o 
	rm -f $*.o
