piano: piano.o
	gcc -o piano piano.o -lSDL2 -lm 

piano.o: piano.c
	gcc -W -Wall -Wextra -pedantic -g -c piano.c
