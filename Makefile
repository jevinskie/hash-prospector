CC      := clang
CFLAGS  := -flto=full -std=gnu23 -Wall -Wextra -march=native -O3 -ggdb3 -fopenmp
LDFLAGS :=
ifneq ($(shell uname -s),Darwin)
LDLIBS  := -lm -ldl
else
LDLIBS  :=
endif

ifneq ($(shell uname -s),Darwin)
SO_EXT := so
else
SO_EXT := dylib
LDFLAGS += -flto=full -fvisibility=hidden
endif

HASH_LIBS = \
    tests/degski64.$(SO_EXT) \
    tests/h2hash32.$(SO_EXT) \
    tests/hash32shift.$(SO_EXT) \
    tests/murmurhash3_finalizer32.$(SO_EXT) \
    tests/splitmix64.$(SO_EXT)

TARGETS_EXE := prospector genetic hillclimb hp16
TARGETS_SO := $(HASH_LIBS)
TARGETS := $(TARGETS_EXE) $(TARGETS_SO)

all: $(TARGETS)

compile: $(TARGETS_EXE)

%: %.c
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $^ $(LDLIBS)

tests/%.$(SO_EXT): tests/%.c
	$(CC) -shared $(LDFLAGS) -fPIC $(CFLAGS) -o $@ $^ $(LDLIBS)

hashes: $(HASH_LIBS)

check: prospector hashes
	./prospector -E -8 -l tests/degski64.$(SO_EXT)
	./prospector -E -4 -l tests/h2hash32.$(SO_EXT)
	./prospector -E -4 -l tests/hash32shift.$(SO_EXT)
	./prospector -E -4 -l tests/murmurhash3_finalizer32.$(SO_EXT)
	./prospector -E -8 -l tests/splitmix64.$(SO_EXT)

clean-targets:
	rm -f $(TARGETS)
	rm -rf *.dSYM/
	rm -rf tests/*.dSYM/

clean-compile-commands:
	rm -f compile_commands.json

clean: clean-targets clean-compile-commands

compile_commands.json:
	bear -- $(MAKE) -B -f $(MAKEFILE_LIST) RUNNING_BEAR=1
	$(MAKE) -f $(MAKEFILE_LIST) clean-targets

scan:
	scan-build -V $(MAKE) -B -f $(MAKEFILE_LIST)

.PHONY: clean-targets clean-compile-commands clean compile_commands.json scan
