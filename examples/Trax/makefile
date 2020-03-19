# config
FILES = 
OPT = -Ofast
SDL = -lmingw32 -lSDL2main -lSDL2 -lSDL2_mixer -I../..
WARN = -Wall -Wextra -pedantic -Winline
F = -std=c18 -lm $(WARN) $(SDL) $(OPT)
NAME = a
CC = gcc -o$(NAME).exe $(F)
CO = gcc -c $(F)

.PHONY: all clean target
all: target

# build

FILES += main.c
main.o: main.c
	$(CO) main.c $(F)

target: $(FILES)
	$(CC) $(FILES) ../../SDL_Context.o $(F)

# utils
re:
	make clean && make && make run

run:
	./$(NAME).exe

clean: 
	del *.o
	del $(NAME).exe
