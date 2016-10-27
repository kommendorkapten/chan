# Chose event mechanism (poll and select are supported)
EVENT_POLL=1

CC=gcc
CFLAGS=-m64 -DMT_SAFE -I/usr/local/include -DEVENT_POLL=$(EVENT_POLL)
LFLAGS += -lpthread 
UNAME=$(shell uname -s)

# Default to c99 on Solaris
ifeq ($(UNAME), SunOS)
CC=c99
CFLAGS += -D_POSIX_C_SOURCE=200112L
endif

# Configure stuff based on compiler
ifeq ($(CC), gcc)
CFLAGS += -W -Wall -pedantic -std=c99 -O2
else ifeq ($(CC), c99)
CFLAGS += -v -xO2
endif


# Configure based on OS
ifeq ($(UNAME), SunOS)
ifeq ($(CC), c99)
CFLAGS += -mt -v
endif
endif

OBJDIR=obj
BINDIR=bin
RAWOBJS=chan.o chan_poll.o chan_select.o lock.o
OBJS=$(RAWOBJS:%=$(OBJDIR)/%)
DIRS=$(OBJDIR) $(BINDIR)
lint_deps=chan.c lock.c chan_poll.c chan_select.c
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
	$(CC) $(CFLAGS) -o $@ test_channel.c $(OBJS) $(LFLAGS) -L/usr/local/lib -lscut

$(bench): bench_channel.c $(OBJS)
	$(CC) $(CFLAGS) -o $@ bench_channel.c $(OBJS) $(LFLAGS)

lint:
	lint -errtags=yes -Xc99 -m64 -errchk=%all -Ncheck=%all -u -m -Nlevel=3 $(lint_deps) -erroff=E_FUNC_RET_ALWAYS_IGNOR,E_SIGN_EXTENSION_PSBL,E_CAST_INT_TO_SMALL_INT,E_FUNC_DECL_VAR_ARG,E_ASGN_RESET

$(OBJDIR)/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJDIR)/*.o $(exe) $(bench)

distclean: 
	rm -r $(DIRS)
