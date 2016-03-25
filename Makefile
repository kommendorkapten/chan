CC=gcc
CFLAGS=-m64 -O2
UNAME=$(shell uname -s)


ifeq ($(UNAME), SunOS)
CC=c99
CFLAGS += -D_POSIX_C_SOURCE=200112L -DMT
endif 

ifeq ($(CC), gcc)
CFLAGS += -W -Wall -pedantic -std=c99
else ifeq ($(CC), c99)
CFLAGS += -mt 
endif

LFLAGS += -lpthread

OBJDIR=obj
BINDIR=bin
RAWOBJS=chan.o
OBJS=$(RAWOBJS:%=$(OBJDIR)/%)
DIRS=$(OBJDIR) $(BINDIR)
lint_deps=chan.c test_channel.c
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

$(exe): test_channel.c $(OBJS)
	$(CC) $(CFLAGS) -o $@ test_channel.c $(OBJS) $(LFLAGS)

$(bench): bench_channel.c $(OBJS)
	$(CC) $(CFLAGS) -o $@ bench_channel.c $(OBJS) $(LFLAGS)

lint:
	lint -errfmt=simple -Xc99 -m64 -errchk=%all -Ncheck=%all -Nlevel=3 $(lint_deps)

$(OBJDIR)/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJDIR)/*.o $(exe)

distclean: 
	rm -r $(DIRS)
