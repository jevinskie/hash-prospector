CC      = /opt/homebrew/opt/llvm/bin/clang
CFLAGS  = -std=c99 -Wall -Wextra -march=native -O3 -ggdb3 -fopenmp
LDFLAGS =
LDLIBS  = -lm -ldl

ifneq ($(shell uname -s),Darwin)
SO_EXT := so
else
SO_EXT := dylib
LDFLAGS += -flto=full -fvisibility=hidden
endif

compile: prospector genetic hillclimb hp16

prospector: prospector.c
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ prospector.c $(LDLIBS)

genetic: genetic.c
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ genetic.c $(LDLIBS)

hillclimb: hillclimb.c
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ hillclimb.c $(LDLIBS)

hp16: hp16.c
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ hp16.c $(LDLIBS)

tests/degski64.$(SO_EXT): tests/degski64.c
tests/h2hash32.$(SO_EXT): tests/h2hash32.c
tests/hash32shift.$(SO_EXT): tests/hash32shift.c
tests/murmurhash3_finalizer32.$(SO_EXT): tests/murmurhash3_finalizer32.c
tests/splitmix64.$(SO_EXT): tests/splitmix64.c

HASH_LIBS = \
    tests/degski64.$(SO_EXT) \
    tests/h2hash32.$(SO_EXT) \
    tests/hash32shift.$(SO_EXT) \
    tests/murmurhash3_finalizer32.$(SO_EXT) \
    tests/splitmix64.$(SO_EXT)

hashes: $(HASH_LIBS)

check: prospector hashes
	./prospector -E -8 -l tests/degski64.$(SO_EXT)
	./prospector -E -4 -l tests/h2hash32.$(SO_EXT)
	./prospector -E -4 -l tests/hash32shift.$(SO_EXT)
	./prospector -E -4 -l tests/murmurhash3_finalizer32.$(SO_EXT)
	./prospector -E -8 -l tests/splitmix64.$(SO_EXT)

clean:
	rm -f prospector genetic hillclimb hp16 $(HASH_LIBS)
	rm -rf *.dSYM/
	rm -rf tests/*.dSYM

.SUFFIXES: .$(SO_EXT) .c
.c.$(SO_EXT):
	$(CC) -shared $(LDFLAGS) -fPIC $(CFLAGS) -o $@ $<
