VERSION         = 1.0
APP             = myapp
ICONTEXT        = "My App"
APPID           = strt
RCP             = $(APP).rcp
PRC             = $(APP).prc
DEF             = $(APP).def
SRC             = $(APP).c $(APP)Section2.c $(APP)Section3.c
SECTIONS        = $(APP)-sections

CC              = m68k-palmos-gcc
PILRC           = pilrc
BUILDPRC        = build-prc
MULTIGEN        = multigen

# Uncomment this if you want to build a GDB-debuggable version
#CFLAGS = -O2 -g
CFLAGS = -O2

all: $(PRC)

$(PRC): $(APP) bin.stamp
	$(BUILDPRC) $(PRC) $(ICONTEXT) $(APPID) $(DEF) *.bin
	ls -l *.prc

$(APP): $(SRC:.c=.o) $(SECTIONS).ld
	$(CC) $(CFLAGS) -o $@ $^

bin.stamp: $(RCP)
	$(PILRC) $^ $(BINDIR)
	touch $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@
#               touch $<
# Enable the previous line if you want to compile EVERY time.

$(SECTIONS).o: $(SECTIONS).s
	$(CC) $(CFLAGS) -c $< -o $@

$(SECTIONS).s $(SECTIONS).ld: $(DEF)
	$(MULTIGEN) $(DEF)

depend dep:
	$(CC) -M $(SRC) > .dependencies

clean:
	rm -rf *.o $(APP) *.bin *.stamp *.s *.ld

veryclean: clean
	rm -rf *.prc *.rcp *.bak
