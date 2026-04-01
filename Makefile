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

TEST_LAYER_SRCS = $(filter-out $(SRCDIR)/main.c,$(wildcard $(SRCDIR)/*.c))
TEST_BIN        = ghost-detector-tests.exe

.PHONY: all clean run test

all: $(BUILDDIR) $(TARGET)

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ -ladvapi32

$(BUILDDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

run: all
	./$(TARGET)

test: $(BUILDDIR)
	$(CC) $(CFLAGS) -o $(TEST_BIN) $(TEST_LAYER_SRCS) tests/test_main.c -ladvapi32
	./$(TEST_BIN)

clean:
	rm -rf $(BUILDDIR) $(TARGET) $(TEST_BIN)
