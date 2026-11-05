CC = cc
TARGET = shadowcasting
SRC = shadowcasting.c


# Aggressive optimization flags for maximum performance
CFLAGS = -O3 -march=native -mtune=native -ffast-math -flto -Wall -Wextra $(shell sdl2-config --cflags)
LDLIBS = $(shell sdl2-config --libs) -lm
LDFLAGS = -flto


$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS) $(LDFLAGS)


clean:
	rm -f $(TARGET)
