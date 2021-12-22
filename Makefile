CC = clang
CFLAGS = -Wall -Wextra -Werror -Wpedantic -O2 -g
LFLAGS =
ENQOI_OBJS = enqoi.o encoder.o
DEQOI_OBJS = deqoi.o decoder.o
RM = rm -f

all: enqoi deqoi

enqoi: $(ENQOI_OBJS)
	$(CC) $(LFLAGS) $(ENQOI_OBJS) -o enqoi

deqoi: $(DEQOI_OBJS)
	$(CC) $(LFLAGS) $(DEQOI_OBJS) -o deqoi

%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	$(RM) enqoi deqoi $(ENQOI_OBJS) $(DEQOI_OBJS)

scan-build: clean
	scan-build --use-cc=$(CC) make

format:
	clang-format -i -style=file *.[ch]

.PHONY: all clean scan-build format
