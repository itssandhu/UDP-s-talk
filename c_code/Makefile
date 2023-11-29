CC = gcc
CFLAGS = -Wall -Wextra -pthread -g
LDFLAGS = -pthread
OBJ = list/list.o
EXECUTABLE = s-talk

all: $(EXECUTABLE)

$(EXECUTABLE): main.c $(OBJ)
	$(CC) $(CFLAGS) main.c $(OBJ) -o $(EXECUTABLE) $(LDFLAGS)

clean:
	rm -f $(EXECUTABLE)
