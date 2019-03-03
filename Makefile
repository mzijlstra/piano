piano: piano.o
	gcc -o piano piano.o -lm -lSDL2 -lSDL2_image

piano.o: piano.c
	gcc -W -Wall -Wextra -pedantic -g -c piano.c
