
build: hangman.o
	gcc ./bin/hangman.o -o ./bin/hangman	

hangman.o:
	gcc -c ./src/hangman.c -o ./bin/hangman.o -DWORD_FILE="/tmp/file.txt" -Wall -Wextra -pedantic

clean:
	rm -f bin/*
