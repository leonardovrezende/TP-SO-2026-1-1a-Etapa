CC = gcc
CFLAGS = -Wall -pthread
TARGET = trabSO
SRCS = PCB.c TCB.c fila.c escalonador.c main.c

.PHONY: all monoprocessador multiprocessador clean

all: monoprocessador

monoprocessador: $(SRCS)
	$(CC) $(CFLAGS) -DNUM_CPUS=1 -o $(TARGET) $(SRCS)

multiprocessador: $(SRCS)
	$(CC) $(CFLAGS) -DNUM_CPUS=2 -o $(TARGET) $(SRCS)

clean:
	rm -f $(TARGET)
