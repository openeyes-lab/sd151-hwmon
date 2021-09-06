#
# Makefile
#
CC = gcc
CFLAGS = -Wall
CFLAGS += -O3 -g3 -I./
BIN = sd151upgrade
LIBS = -lrt -lwiringPi

MAINSRC = updi.c
CSRCS += link.c
CSRCS += phy.c
CSRCS += nvm.c
CSRCS += app.c
CSRCS += ihex.c
CSRCS += progress.c

OBJEXT ?= .o

AOBJS = $(ASRCS:.S=$(OBJEXT))
COBJS = $(CSRCS:.c=$(OBJEXT))

MAINOBJ = $(MAINSRC:.c=$(OBJEXT))

SRCS = $(ASRCS) $(CSRCS) $(MAINSRC)
OBJS = $(AOBJS) $(COBJS)

PCOBJS = $(patsubst %,$(OBJDIR)/%,$(COBJ))
PMOBJS = $(patsubst %,$(OBJDIR)/%,$(MAINOBJ))
PAOBJS = $(patsubst %,$(OBJDIR)/%,$(AOBJS))

## MAINOBJ -> OBJFILES

all: default

%.o: %.c
	@$(CC) $(CFLAGS) -c $< -o $@
	@echo "CC $<"

default: $(AOBJS) $(COBJS) $(MAINOBJ)
	$(CC) -o $(BIN) $(MAINOBJ) $(AOBJS) $(COBJS) $(LDFLAGS) $(LIBS)

clean:
	rm -f $(BIN) $(AOBJS) $(COBJS) $(MAINOBJ)
