LIBS  =
CFLAGS = -Wall

# Should be equivalent to your list of C files, if you don't build selectively
SRC=$(wildcard *.c)

test: $(SRC)
	gcc -o $@ $^ $(CFLAGS) -I../include/ -I. -DLIBGIT2_NO_FEATURES_H -DGIT_THREADS $(LIBS)
