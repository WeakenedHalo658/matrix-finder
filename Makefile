CC       = gcc
CFLAGS   = -Wall -Wextra -O2 -Iinclude
# MinGW gcc needs a writable temp dir; set all three variables so that
# make sub-processes inherit the right value regardless of the Windows
# TEMP/TMP registry entry.
export TMPDIR := /tmp
export TEMP   := /tmp
export TMP    := /tmp
TARGET   = ghost-detector.exe
SRCDIR   = src
BUILDDIR = build

SRCS  = $(wildcard $(SRCDIR)/*.c)
OBJS  = $(patsubst $(SRCDIR)/%.c,$(BUILDDIR)/%.o,$(SRCS))

.PHONY: all clean run

all: $(BUILDDIR) $(TARGET)

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ -ladvapi32

$(BUILDDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

run: all
	./$(TARGET)

clean:
	rm -rf $(BUILDDIR) $(TARGET)
