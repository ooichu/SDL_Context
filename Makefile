# options
RELEASE = -static-libgcc -mx32
LUA = #-llua53 -DSDL_CONTEXT_LUA
SDL = -lmingw32 -lSDL2_mixer -lSDL2main -lSDL2
DBG = #-g03
OPT = -O3
ERR = -Wall -Wextra -pedantic -Winline
DEFS = -DSDL_CONTEXT_DEBUG
F = -lm $(ERR) $(SDL) $(OPT) $(DBG) $(LUA) 
NAME = a


.PHONY: all target clean
all: target

# files
FILES =

FILES += SDL_Context.o
SDL_Context.o: SDL_Context.c
	gcc -c SDL_Context.c -std=c99 $(F)

FILES += main.o
main.o: main.c
	gcc -c main.c -std=gnu99 $(F)

# build
target: $(FILES)
	gcc $(FILES) $(RELEASE) -std=gnu99 $(F)

# utils
re:
	make clean && make && make run

run:
	./$(NAME).exe

clean:
	del *.o
	del $(NAME).exe

