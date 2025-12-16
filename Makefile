CC = cc
TARGET = raytracing
SRC = raytracing.c


CFLAGS = -Wall -Wextra -g $(shell sdl2-config --cflags)
LDLIBS = $(shell sdl2-config --libs) -lm


$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)


clean:
	rm -f $(TARGET)
