APP             = serialchat
ICONTEXT        = "Serial Chat"
APPID           = LFsc
RCP             = $(APP).rcp
PRC             = $(APP).prc
SRC             = $(APP).c

CC              = m68k-palmos-gcc
PILRC           = pilrc
OBJRES          = m68k-palmos-obj-res
BUILDPRC        = build-prc


# Uncomment this if you want to build a GDB-debuggable version
# CFLAGS = -O0 -g
CFLAGS = -O2

all: $(PRC)

$(PRC): grc.stamp bin.stamp;
	$(BUILDPRC) $(PRC) $(ICONTEXT) $(APPID) *.grc *.bin
	ls -l *.prc

grc.stamp: $(APP)
	$(OBJRES) $(APP)
	touch $@

$(APP): $(SRC:.c=.o)
	$(CC) $(CFLAGS) $^ -o $@

bin.stamp: $(RCP)
	$(PILRC) $^ $(BINDIR)
	touch $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@
#               touch $<
# Enable the previous line if you want to compile EVERY time.

depend dep:
	$(CC) -M $(SRC) > .dependencies

clean:
	rm -rf *.o $(APP) *.bin *.grc *.stamp *~

veryclean: clean
	rm -rf *.prc *.bak
