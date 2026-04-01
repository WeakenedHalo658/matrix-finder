CC       = gcc
CFLAGS   = -Wall -Wextra -O2 -Iinclude
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
	$(CC) $(CFLAGS) -o $@ $^

$(BUILDDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

run: all
	./$(TARGET)

clean:
	rm -rf $(BUILDDIR) $(TARGET)
