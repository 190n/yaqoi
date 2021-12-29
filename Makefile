CC = clang
CFLAGS = -Wall -Wextra -Werror -Wpedantic -O2 -g -Ivendor/stb
LFLAGS = -lm
OBJS = enqoi.o encoder.o stbi.o container.o
RM = rm -f

all: enqoi

enqoi: $(OBJS)
	$(CC) $(LFLAGS) $(OBJS) -o enqoi

%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	$(RM) enqoi $(OBJS)

scan-build: clean
	scan-build --use-cc=$(CC) make

format:
	clang-format -i -style=file *.[ch]

.PHONY: all clean scan-build format
