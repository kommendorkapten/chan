CC=gcc
CFLAGS=-m64 -O2 -DMT_SAFE -I/usr/local/include
LFLAGS += -lpthread -L/usr/local/lib -lscut
UNAME=$(shell uname -s)

# Default to c99 on Solaris
ifeq ($(UNAME), SunOS)
CC=c99
CFLAGS += -D_POSIX_C_SOURCE=200112L
endif

# Configure stuff based on compiler
ifeq ($(CC), gcc)
CFLAGS += -W -Wall -pedantic -std=c99
endif

# Configure based on OS
ifeq ($(UNAME), SunOS)
ifeq ($(CC), c99)
CFLAGS += -mt 
endif
endif

# Chose event mechanism (poll and select are supported)
EVENT_M=poll

OBJDIR=obj
BINDIR=bin
RAWOBJS=chan.o chan_$(EVENT_M).o lock.o
OBJS=$(RAWOBJS:%=$(OBJDIR)/%)
DIRS=$(OBJDIR) $(BINDIR)
lint_deps=chan.c test_channel.c lock.c chan_$(EVENT_M).c 
exe=$(BINDIR)/test
bench=$(BINDIR)/bench

.PHONY: dir
.PHONY: all
.PHONY: lint
.PHONY: clean
.PHONY: distclean

all: dir $(exe) $(bench)

dir: $(DIRS)

$(DIRS):
	mkdir $(DIRS)

test: $(exe)
	$(exe)

$(exe): test_channel.c $(OBJS)
	$(CC) $(CFLAGS) -o $@ test_channel.c $(OBJS) $(LFLAGS)

$(bench): bench_channel.c $(OBJS)
	@$(CC) $(CFLAGS) -o $@ bench_channel.c $(OBJS) $(LFLAGS)

lint:
	lint -errfmt=simple -Xc99 -m64 -errchk=%all -Ncheck=%all -Nlevel=3 $(lint_deps)

$(OBJDIR)/%.o: %.c
	@$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJDIR)/*.o $(exe) $(bench)

distclean: 
	rm -r $(DIRS)
