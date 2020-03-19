.PHONY: all target clean
all: target

# decl's
RELEASE = -static-libgcc
SDL = -lmingw32 -lSDL2main -lSDL2
OPT = #-Ofast
WARN = -Wall -Wextra -Winline -pedantic
DEBUG = -g03
F = $(SDL) $(DEBUG) $(OPT) $(WARN) $(RELEASE) -I../../
NAME = Asteroids
CO = g++ -std=c++17 -c $(F)
CC = g++ $(F) -o$(NAME).exe

# add files
FILES = ../../SDL_Context.o

FILES += Asteroids.o
Asteroids.o: Asteroids.cpp
	$(CO) Asteroids.cpp

# build
target: $(FILES)
	$(CC) $(FILES) $(RELEASE) $(F)

# utils
run:
	$(NAME).exe

clean:
	del *.o
	del $(NAME).exe

