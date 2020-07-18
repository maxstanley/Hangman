
build: hangman.o
	gcc ./bin/hangman.o -o ./bin/hangman	

hangman.o:
	gcc -c ./src/hangman.c -o ./bin/hangman.o -Wall -Wextra -pedantic

clean:
	rm -f bin/*
