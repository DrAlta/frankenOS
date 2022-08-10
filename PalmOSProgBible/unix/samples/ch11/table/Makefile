APP             = table
ICONTEXT        = "Table Example"
APPID           = LFtb
RCP             = $(APP).rcp
PRC             = $(APP).prc
SRC             = $(APP).c
GRC             = $(APP).grc
BIN             = $(APP).bin

CC              = m68k-palmos-gcc
PILRC           = pilrc
OBJRES          = m68k-palmos-obj-res
BUILDPRC        = build-prc


# uncomment this if you want to build a gdb debuggable version
# -On: n=Optimization-level (0=none), -g: needed for debugging
# CFLAGS = -O0 -g $(DEFINES) $(INCLUDES)
CFLAGS = -O2 $(DEFINES) $(INCLUDES)

all: $(PRC)

$(PRC): grc.stamp bin.stamp;
	$(BUILDPRC) $(PRC) $(ICONTEXT) $(APPID) *.grc *.bin $(LINKFILES) 
	ls -l *.prc

grc.stamp: $(APP) ;
	$(OBJRES) $(APP)
	touch $@

$(APP): $(SRC:.c=.o) ;
	$(CC) $(CFLAGS) $^ -o $@

bin.stamp: $(RCP) ;
	$(PILRC) $^ $(BINDIR)
	touch $@

%.o: %.c ;
	$(CC) $(CFLAGS) -c $< -o $@
#               touch $<
# enable this line if you want to compile EVERY time.

depend dep: ;
	$(CC) -M $(SRC) > .dependencies

clean:
	rm -rf *.o $(APP) *.bin *.grc *.stamp

veryclean: clean
	rm -rf *.prc *.bak *~
