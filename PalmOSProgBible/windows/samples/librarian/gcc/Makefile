VERSION         = 1.0.0gcc
APP             = librarian
ICONTEXT        = "Librarian"
APPID           = LFlb
RCPIN           = $(APP).rcp.in
RCP             = $(APP).rcp
PRC             = $(APP).prc
SRC             = $(APP).c $(APP)DB.c $(APP)Transfer.c

CC              = m68k-palmos-gcc
PILRC           = pilrc
OBJRES          = m68k-palmos-obj-res
BUILDPRC        = build-prc

# Uncomment this if you want to build a GDB-debuggable version
#CFLAGS = -O2 -g
CFLAGS = -O2

all: $(PRC)

$(PRC): grc.stamp bin.stamp
	$(BUILDPRC) $(PRC) $(ICONTEXT) $(APPID) *.grc *.bin
	ls -l *.prc

grc.stamp: $(APP)
	$(OBJRES) $(APP)
	touch $@

$(APP): $(SRC:.c=.o)
	$(CC) $(CFLAGS) -o $@ $^

bin.stamp: $(RCP)
	$(PILRC) $^ $(BINDIR)
	touch $@

$(RCP): $(RCPIN)
	sed -e 's/VERSION_STAMP/$(VERSION)/g' $(RCPIN) > $(RCP)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@
#               touch $<
# Enable the previous line if you want to compile EVERY time.

depend dep:
	$(CC) -M $(SRC) > .dependencies

clean:
	rm -rf *.o $(APP) *.bin *.grc *.stamp

veryclean: clean
	rm -rf *.prc *.rcp *.bak
