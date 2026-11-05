CC = cc
TARGET = shadowcasting
SRC = shadowcasting.c


# Aggressive optimization flags for maximum performance
CFLAGS = -O3 -march=native -mtune=native -ffast-math -flto -g -Wall -Wextra $(shell sdl2-config --cflags)
LDLIBS = $(shell sdl2-config --libs) -lm
LDFLAGS = -flto


$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS) $(LDFLAGS)


debug: CFLAGS = -O0 -g -Wall -Wextra -fsanitize=address,undefined $(shell sdl2-config --cflags)
debug: LDFLAGS = -fsanitize=address,undefined
debug: clean $(TARGET)

.PHONY: clean debug
clean:
	rm -f $(TARGET)
