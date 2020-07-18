#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/socket.h>

#define BUFFER_SIZE 1024
#define LIVES 11
#define PORT 3001
#define QUEUE_LENGTH 10 // Number of connections that can queue

extern time_t time();

void play_hangman(int in, FILE * fp);
int line_count(FILE * fp, int line);

int main(int argc, char * argv[])
{

	if (argc != 2)
	{
		printf("Invalid arguments\n");
		printf("Expecting dictionary file path\n");
		return -1;
	}

//	char *filename = "/usr/share/dict/british-english";
	FILE * fp = fopen(argv[1], "r");

	if (fp == NULL)
	{
		printf("File %s not accessible\n", argv[1]);
		return -1;
	}
	
	struct sockaddr_in server;

	// Prevents Zombies
	signal(SIGCHLD, SIG_IGN);

	int sock = socket(PF_INET, SOCK_STREAM, 0);
	
	if (sock < 0)
	{
		printf("Error Creating Stram Socket\n");
		return 1;
	}

	server.sin_family      = AF_INET;
	server.sin_addr.s_addr = htonl(INADDR_ANY);
	server.sin_port        = htons(PORT);
	int server_size = sizeof(server);

	if (bind(sock, (struct sockaddr *)&server, server_size) < 0)
	{
		printf("Error Binding to socket\n");
		return 2;
	}

	listen(sock, QUEUE_LENGTH);
	printf("Listening on port: %d\n", PORT);

	struct sockaddr_in client;
	socklen_t client_length = sizeof(struct sockaddr_in);

	// Count the number of lines
	int lines = line_count(fp, RAND_MAX);

	while (1)
	{
		int connection = accept(sock, (struct sockaddr *)&client, &client_length);

		if (connection < 0)
		{
			printf("Error accepting connection\n");
			return 3;
		}

		if (fork() == 0)
		{
			close(sock);
			srand((int)time((long *)0));

			// Go to random line
			line_count(fp, rand() % lines);

			play_hangman(connection, fp);
			exit(0);
		}
		else
		{
			close(connection);
		}
	}

	pclose(fp);
	close(sock);
	return 0;
}

int line_count(FILE * file, int line_limit)
{
	int count = 0;
	rewind(file);

	int c;

	while((c = fgetc(file)) != EOF && line_limit > 0)
	{
		// If the character is not a new line, we don't really care
		if (c != '\n')
			continue;
		
		// Go to the character beind the current \n
		fseek(file, -2L, SEEK_CUR);

		// If the previous character wasn't a \n count this one
		if (fgetc(file) != '\n')
		{
			--line_limit;
			++count;
		}

		// Go forward to go to the next character
		fseek(file, 1L, SEEK_CUR);
	}
	
	// Go to the start of the line if not at the end of the file
	if (c != EOF)
		fseek(file, -1, SEEK_CUR);
	return count;
}

void get_current_line(FILE * file, char ** word)
{
	char buffer[50] = { 0 }	;
	int c;
	int i = 0;

	while ((c = fgetc(file)) != EOF)
	{
		if (c == '\n')
			break;
		buffer[i] = (buffer[i] < 91 && buffer[i] > 64) ? (c - 32) : c;
		++i;
	}

	*word = (char *)calloc(i + 1, sizeof(buffer[0]));
	strcpy(*word, buffer);
}

void play_hangman(int connection, FILE * fp)
{
	char buffer[BUFFER_SIZE];

	int lives = LIVES;
	char guesses[LIVES] = { 0 };

	char * word;
	get_current_line(fp, &word);
	int word_length = strlen(word);

	int MAX_LENGTH = 20;
	char guessed_word[MAX_LENGTH];
	memset(guessed_word, '_', MAX_LENGTH);
	guessed_word[word_length] = 0;

	sprintf(buffer, "Welcome to hangman\nYou have to guess a %d letter word. Good luck!\n", word_length);
	write(connection, buffer, strlen(buffer));

	while (strcmp(guessed_word, word) && lives > 0)
	{
		sprintf(buffer, "%s\tLives: %d\nGuessed: %s\n", guessed_word, lives, guesses);
		write(connection, buffer, strlen(buffer));

		char guess[BUFFER_SIZE];
		read(connection, guess, BUFFER_SIZE);

		int correct = 0;
		char c;
		for (int i = 0; (c = word[i]) != 0; ++i)
		{
			if (guess[0] == c)
			{
				guessed_word[i] = c;
				correct = 1;
			}
		}

		if (!correct)
		{
			guesses[LIVES - lives] = guess[0];
			--lives;
		}
	}

	if (strcmp(word, guessed_word) == 0)
	{
		sprintf(buffer, "Well done! You correctly guessed the word %s\n", word);
		write(connection, buffer, strlen(buffer));
	}
	else
	{
		sprintf(buffer, "I'm Sorry! The word was %s\n", word);
		write(connection, buffer, strlen(buffer));
	}

	free(word);
}

