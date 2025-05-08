# Authored by Christopher Tam for Georgia Tech's CS 2200; Modified by Aman Jain for the vmem autograder
TARGET = vm-sim

CC     = gcc
CFLAGS = -Wall -Wextra -Wsign-conversion -Wpointer-arith -Wcast-qual -Wwrite-strings -Wshadow -Wmissing-prototypes -Wpedantic -Wwrite-strings -g -std=gnu99 -lm -O0

LFLAGS =

SRCDIR = *-src
INCDIR = $(SRCDIR)
BINDIR = .

SUBMIT_FILES  = $(SRC) $(INC) Makefile
SUBMISSION_NAME = project3-vm

SRC := $(wildcard $(SRCDIR)/*.c)
INC := $(wildcard $(INCDIR)/*.h)

INCFLAGS := $(patsubst %/,-I%,$(dir $(wildcard $(INCDIR)/.)))

.PHONY: all
all:
	@$(MAKE) release

.PHONY: release
release: CFLAGS += -mtune=native -O0
release: $(BINDIR)/$(TARGET)

.PHONY: clean
clean:
	@rm -f $(BINDIR)/$(TARGET)
	@rm -rf $(BINDIR)/$(TARGET).dSYM


.PHONY: submit
submit:
	@(tar zcfh $(SUBMISSION_NAME).tar.gz $(SUBMIT_FILES) && \
	echo "Created submission archive $$(tput bold)$(SUBMISSION_NAME).tar.gz$$(tput sgr0).")


$(BINDIR)/$(TARGET): $(SRC) $(INC)
	@mkdir -p $(BINDIR)
	@$(CC) $(CFLAGS) $(INCFLAGS) $(SRC) -o $@ $(LFLAGS)
