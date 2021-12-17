//Mateo González de Miguel y Jorge san Frutos Iglesias
#include "clientGame.h"

void sendMessageToServer (int socketServer, char* message){

	size_t tam = strlen(message);
	 int messageLength = send(socketServer, &tam, sizeof(size_t), 0);

	 // Check bytes sent
	 if (messageLength < 0)
		 showError("ERROR while writing to socket");

	 messageLength = send(socketServer, message, tam, 0);

	 // Check bytes sent
	 if (messageLength < 0)
		 showError("ERROR while writing to socket");

}

void receiveMessageFromServer (int socketServer, char* message){
	size_t tam;
	int messageLength = recv(socketServer, &tam, sizeof(size_t), 0);
	
	 // Check read bytes
	 if (messageLength < 0)
		 showError("ERROR while reading from socket");

	 messageLength = recv(socketServer, message, tam, 0);

	 // Check read bytes
	 if (messageLength < 0)
		 showError("ERROR while reading from socket");

}

void receiveBoard (int socketServer, tBoard board){

	size_t tam;
	int messageLength = recv(socketServer, &tam, sizeof(unsigned int), 0);
	
	 // Check read bytes
	 if (messageLength < 0)
		 showError("ERROR while reading from socket");

	 messageLength = recv(socketServer, board, tam, 0);

	 // Check read bytes
	 if (messageLength < 0)
		 showError("ERROR while reading from socket");
}

unsigned int receiveCode (int socketServer){
	unsigned int code;
	int messageLength = recv(socketServer, &code, sizeof(unsigned int), 0);

	 // Check read bytes
	 if (messageLength < 0)
		 showError("ERROR while reading from socket");
	return code;
}

unsigned int readMove (){

	tString enteredMove;
	unsigned int move;
	unsigned int isRightMove;


		// Init...
		isRightMove = FALSE;
		move = STRING_LENGTH;

		while (!isRightMove){

			printf ("Enter a move [0-6]:");

			// Read move
			fgets (enteredMove, STRING_LENGTH-1, stdin);

			// Remove new-line char
			enteredMove[strlen(enteredMove)-1] = 0;

			// Length of entered move is not correct
			if (strlen(enteredMove) != 1){
				printf ("Entered move is not correct. It must be a number between [0-6]\n");
			}

			// Check if entered move is a number
			else if (isdigit(enteredMove[0])){

				// Convert move to an int
				move =  enteredMove[0] - '0';

				if (move > 6)
					printf ("Entered move is not correct. It must be a number between [0-6]\n");
				else
					isRightMove = TRUE;
			}

			// Entered move is not a number
			else
				printf ("Entered move is not correct. It must be a number between [0-6]\n");
		}

	return move;
}

void sendMoveToServer (int socketServer, unsigned int move){

    int messageLength = send(socketServer, &move, sizeof(unsigned int), 0);

	 // Check bytes sent
	if (messageLength < 0)
	    showError("ERROR while writing to socket");
}



int main(int argc, char *argv[]){

	int socketfd;						/** Socket descriptor */
	unsigned int port;					/** Port number (server) */
	struct sockaddr_in server_address;	/** Server address structure */
	char* serverIP;						/** Server IP */

	tBoard board;						/** Board to be displayed */
	tString playerName;					/** Name of the player */
	tString rivalName;					/** Name of the rival */
	tString message;					/** Message received from server */
	unsigned int column;				/** Selected column */
	unsigned int code;					/** Code sent/receive to/from server */
	unsigned int endOfGame;				/** Flag to control the end of the game */

		// Check arguments!
		if (argc != 3){
			fprintf(stderr,"ERROR wrong number of arguments\n");
			fprintf(stderr,"Usage:\n$>%s serverIP port\n", argv[0]);
			exit(0);
		}

		// Get the server address
		serverIP = argv[1];

		// Get the port
		port = atoi(argv[2]);

		// Create socket
		socketfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

		// Check if the socket has been successfully created
		if (socketfd < 0)
			showError("ERROR while creating the socket");

		// Fill server address structure
		memset(&server_address, 0, sizeof(server_address));
		server_address.sin_family = AF_INET;
		server_address.sin_addr.s_addr = inet_addr(serverIP);
		server_address.sin_port = htons(port);

		// Connect with server
		if (connect(socketfd, (struct sockaddr *) &server_address, sizeof(server_address)) < 0)
			showError("ERROR while establishing connection");
		printf ("Connection established with server!\n");

		// Init player's name
		do{
			memset(playerName, 0, STRING_LENGTH);
			printf ("Enter player name: ");
			fgets(playerName, STRING_LENGTH-1, stdin);
			
			// Remove '\n'
			playerName[strlen(playerName)-1] = 0;

		}while (strlen(playerName) <= 2);
		
		// Send player's name to the server
		sendMessageToServer (socketfd, playerName);

		// Receive rival's name
		memset(rivalName, 0, STRING_LENGTH);
		receiveMessageFromServer (socketfd, rivalName);

		printf ("You are playing against %s\n", rivalName);

		// Init
		endOfGame = FALSE;

		// Game starts
		printf ("Game starts!\n\n");

		// While game continues...

        while(!endOfGame){
			//receive the code, message and board from the server
            code = receiveCode(socketfd);
			memset(message, 0, STRING_LENGTH);
            receiveMessageFromServer(socketfd, message);
			memset(board, 0, STRING_LENGTH);
            receiveBoard(socketfd, board);
			printBoard(board, message);

			//if the code is move/wait the player moves/waits, else the endOfGame is true
			if(code == TURN_MOVE || code == TURN_WAIT){
				if(code == TURN_MOVE){
					code = fullColumn_move;
					//if the code is OK it goes on
					while(code == fullColumn_move){
						//read the move and send it to the server
						column = readMove();
						printf("%d\n", column);
						sendMoveToServer(socketfd, column);
						code = receiveCode(socketfd);
						if(code == fullColumn_move){
							printf("You introduced a wrong column, its full.\n");
						}
					}
				}
			}else{
				endOfGame = TRUE;
			}
        }

	// Close socket
	close(socketfd);
}
