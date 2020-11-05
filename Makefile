##############################################################################
##  Makefile with automatic dependencies generation
##############################################################################


##############################################################################
##  Source files with paths
##############################################################################

C_SOURCES+= \
	src/main.c			\
	src/library.c		\
	mujs.git/one.c

##############################################################################



##############################################################################
##  Directories and file names
##############################################################################
# Output file name
BIN=jsh
# Path for binary files
BINDIR=bin
# Path for object files
OBJDIR=.obj
# Path for dependencies information
DEPDIR=.dep
##############################################################################



##############################################################################
##  Compiler configuration
##############################################################################
CC?=$(CROSS_COMPILE)gcc
INCLUDES=-Isrc -Imujs.git
DEFINES=
CFLAGS=-Wall -Wstrict-prototypes -Wshadow -O3 -fmerge-all-constants -g $(INCLUDES) $(DEFINES)
LIBS=-lm
LDFLAGS=$(LIBS) -Wl,--relax,--gc-sections
##############################################################################



# Target ALL
all: $(BINDIR)/$(BIN)

# Target for linker
$(BINDIR)/$(BIN): $(C_SOURCES:%.c=$(OBJDIR)/%.c.o)
	@set -e; \
	echo "Linking..."; \
	mkdir -p $(BINDIR); \
	$(CC) $(LDFLAGS) -o $@ $+

# Target for clean
clean:
	rm -rf $(DEPDIR)
	rm -rf $(OBJDIR)
	rm -f src/js_init.h

# Target for distclean
distclean:
	rm -f $(BINDIR)/$(BIN)
	rm -rf $(DEPDIR)
	rm -rf $(OBJDIR)
	rm -f src/js_init.h


# PHONY
.PHONY: all


# Rule for making include from all .js files
src/library.c: src/js_init.h
src/js_init.h: $(wildcard src/js_init/*.js)
	@echo "Creating $@"; \
	cat $+ | xxd -i >$@

# Rule for compiling & deps generation
$(OBJDIR)/%.c.o: %.c
	@set -e; \
	echo "Compiling $<"; \
	mkdir -p $(dir $(DEPDIR)/$<) $(dir $(OBJDIR)/$<); \
	$(CC) $(CFLAGS) -MM -MF $(DEPDIR)/$<.d -MT $(OBJDIR)/$<.o -MP $< || rm -f $(DEPDIR)/$<.d; \
	$(CC) $(CFLAGS) -c -o $(OBJDIR)/$<.o $<


# Including dependencies infomation
-include $(C_SOURCES:%.c=$(DEPDIR)/%.c.d)
