//Mateo Gonzalez de Miguel y Jorge san Frutos Iglesias
#include "serverGame.h"

#define MAX_THREADS 3

//args of the thread (socket of the players)
typedef struct threadData{ 
	int socketPlayer1;
	int socketPlayer2; 
} t_threadData; 

void sendMessageToPlayer (int socketClient, char* message){
	size_t tam = strlen(message);
	 int messageLength = send(socketClient, &tam, sizeof(size_t), 0);

	 // Check bytes sent
	 if (messageLength < 0)
		 showError("ERROR while writing to socket");

	 messageLength = send(socketClient, message, tam, 0);

	 // Check bytes sent
	 if (messageLength < 0)
		 showError("ERROR while writing to socket");

}

void receiveMessageFromPlayer (int socketClient, char* message){
	size_t tam = 0;
	int messageLength = recv(socketClient, &tam, sizeof(size_t), 0);
	 // Check read bytes
	 if (messageLength < 0)
		 showError("ERROR while reading from socket");

	 messageLength = recv(socketClient, message, tam, 0);

	 // Check read bytes
	 if (messageLength < 0)
		 showError("ERROR while reading from socket");

}

void sendCodeToClient (int socketClient, unsigned int code){

	 int messageLength = send(socketClient, &code, sizeof(unsigned int), 0);

	 // Check bytes sent
	 if (messageLength < 0)
		 showError("ERROR while writing to socket");

}

void sendBoardToClient (int socketClient, tBoard board){
	unsigned int tam = sizeof(tBoard);
	 int messageLength = send(socketClient, &tam, sizeof(unsigned int), 0);

	 // Check bytes sent
	 if (messageLength < 0)
		 showError("ERROR while writing to socket");

	 messageLength = send(socketClient, board, tam, 0);

	 // Check bytes sent
	 if (messageLength < 0)
		 showError("ERROR while writing to socket");
}

unsigned int receiveMoveFromPlayer (int socketClient){
    unsigned int move;
	int messageLength = recv(socketClient, &move, sizeof(unsigned int), 0);

	 // Check read bytes
	 if (messageLength < 0)
		 showError("ERROR while reading from socket");
	return move;
}

int getSocketPlayer (tPlayer player, int player1socket, int player2socket){

	int socket;

		if (player == player1)
			socket = player1socket;
		else
			socket = player2socket;

	return socket;
}

tPlayer switchPlayer (tPlayer currentPlayer){

	tPlayer nextPlayer;

		if (currentPlayer == player1)
			nextPlayer = player2;
		else
			nextPlayer = player1;

	return nextPlayer;
}

void *threadTask(void *args){


	tBoard board;						/** Board of the game */
	tPlayer currentPlayer, waitingPlayer;				/** Current player */
	tMove moveResult;					/** Result of player's move */
	tString player1Name;				/** Name of player 1 */
	tString player2Name;				/** Name of player 2 */
	int endOfGame;						/** Flag to control the end of the game*/
	unsigned int column;				/** Selected column to insert the chip */
	tString message;					/** Message sent to the players */
	int socketPlayer1, socketPlayer2, numThread;
	int socketCurrentPlayer, socketWaitingPlayer, aux;	/**Sockets para identificar al jugador**/
	t_threadData *data;

	data = (t_threadData*) args;
	socketPlayer1 = data->socketPlayer1;
	socketPlayer2 = data->socketPlayer2;
		// Receive player 1 info
		memset(player1Name, 0, STRING_LENGTH);
		receiveMessageFromPlayer(socketPlayer1, player1Name);
		printf ("Name of player 1 received: %s\n", player1Name);
		
		// Receive player 2 info
		memset(player2Name, 0, STRING_LENGTH);
		receiveMessageFromPlayer(socketPlayer2, player2Name);
		printf ("Name of player 2 received: %s\n", player2Name);

		// Send rival name to player 1
		sendMessageToPlayer (socketPlayer1, player2Name);

		// Send rival name to player 2
		sendMessageToPlayer (socketPlayer2, player1Name);

		// Init board
		initBoard (board);
		endOfGame = FALSE;

		// Calculates who is moving first and send the corresponding code
	    if ((rand() % 2) == 0){
	    	currentPlayer = player1;		
	    }
	    else{
            currentPlayer = player2;
	    }

		while(!endOfGame){
			//get the socket of the players
			socketCurrentPlayer = getSocketPlayer(currentPlayer, socketPlayer1, socketPlayer2);
			socketWaitingPlayer = getSocketPlayer(switchPlayer(currentPlayer), socketPlayer1, socketPlayer2);
			//send the corresponding code, message and board to the players
			sendCodeToClient(socketCurrentPlayer, TURN_MOVE);
			sendCodeToClient(socketWaitingPlayer, TURN_WAIT);
			sendMessageToPlayer(socketCurrentPlayer, "Its your turn. You play with:o");
			sendMessageToPlayer(socketWaitingPlayer, "Your rival is thinking... please, wait! You play with:x");
			sendBoardToClient(socketCurrentPlayer, board);
			sendBoardToClient(socketWaitingPlayer, board);

			moveResult = fullColumn_move;
			//while the move result of the column isnt a correct one (full column)
			while(moveResult == fullColumn_move){
				column = receiveMoveFromPlayer(socketCurrentPlayer);
				moveResult = checkMove(board, column);
				if(moveResult == fullColumn_move){
					sendCodeToClient(socketCurrentPlayer, fullColumn_move);
				}else{
					sendCodeToClient(socketCurrentPlayer, OK_move);
				}
			}

			insertChip(board, currentPlayer, column);


			//look if the player who inserted the chip wins or the table is full to end the game
			if(checkWinner(board, currentPlayer)){
				sendCodeToClient(socketPlayer1, GAMEOVER_WIN);
				sendCodeToClient(socketPlayer2, GAMEOVER_LOSE);
				sendMessageToPlayer(socketPlayer1, "YOU WIN!!!");
				sendMessageToPlayer(socketPlayer2, "You lose :(");
				sendBoardToClient(socketPlayer1, board);
				sendBoardToClient(socketPlayer2, board);
				endOfGame = TRUE;
			}else if(isBoardFull(board)){
				sendCodeToClient(socketPlayer1, GAMEOVER_DRAW);
				sendCodeToClient(socketPlayer2, GAMEOVER_DRAW);
				sendMessageToPlayer(socketPlayer1, "Its a draw.");
				sendMessageToPlayer(socketPlayer2, "Its a draw.");
				sendBoardToClient(socketPlayer1, board);
				sendBoardToClient(socketPlayer2, board);
				endOfGame = TRUE;
			}

			//switch the player
			currentPlayer = switchPlayer(currentPlayer);
		}
		//close the sockets
		close(socketPlayer1);
		close(socketPlayer2);

}

int main(int argc, char *argv[]){

	int socketfd;						/** Socket descriptor */
	struct sockaddr_in serverAddress;	/** Server address structure */
	unsigned int port;					/** Listening port */

	struct sockaddr_in player1Address;	/** Client address structure for player 1 */
	struct sockaddr_in player2Address;	/** Client address structure for player 2 */
	int socketPlayer1, socketPlayer2;	/** Socket descriptor for each player */
	unsigned int clientLength;			/** Length of client structure */

	// Check arguments
	if (argc != 2) {
		fprintf(stderr,"ERROR wrong number of arguments\n");
		fprintf(stderr,"Usage:\n$>%s port\n", argv[0]);
		exit(1);
	}

	// Init seed
	srand(time(NULL));

	// Create the socket
	socketfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	// Check
	if (socketfd < 0)
		showError("ERROR while opening socket");

	// Init server structure
	memset(&serverAddress, 0, sizeof(serverAddress));

	// Get listening port
	port = atoi(argv[1]);

	// Fill server structure
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
	serverAddress.sin_port = htons(port);

	// Bind
	if (bind(socketfd, (struct sockaddr *) &serverAddress, sizeof(serverAddress)) < 0)
		showError("ERROR while binding");

	// Listen
	listen(socketfd, 10);

//---------------------------------------------------------------------------
	int numThreads = 0;
	pthread_t threadVector[MAX_THREADS]; 
	t_threadData *threadData;
	threadData = (t_threadData*) malloc(sizeof(t_threadData)*3);

	//while there are less of 3 current games
    while(numThreads < 3){

		// Get length of client structure
		clientLength = sizeof(player1Address);

		// Accept player 1 connection!!!
		socketPlayer1 = accept(socketfd, (struct sockaddr *) &player1Address, &clientLength);

		// Check accept result
		if (socketPlayer1 < 0)
			showError("ERROR while accepting connection for player 1");

		printf ("Player 1 is connected!\n");

		clientLength = sizeof(player2Address);

		// Accept player 2 connection!!!
		socketPlayer2 = accept(socketfd, (struct sockaddr *) &player2Address, &clientLength);

		// Check accept result
		if (socketPlayer2 < 0)
			showError("ERROR while accepting connection for player 2");

		printf ("Player 2 is connected!\n");

		//fill the thread args field in the position it corresponds
		threadData[sizeof(t_threadData)*numThreads].socketPlayer1 = socketPlayer1;
		threadData[sizeof(t_threadData)*numThreads].socketPlayer2 = socketPlayer2;

		printf ("Creating thread number: %d\n",numThreads); 
	
		// create the threads 
	
		if (pthread_create(&threadVector[numThreads], NULL, threadTask, &threadData[sizeof(t_threadData)*numThreads]) == -1){ 
			printf("Error creating thread number:%d\n", numThreads); 
			exit(1);
		}
		
		numThreads++;
	}
	
		// Wait for threads execution (joinable) 
	for(int i=0; i<MAX_THREADS; i++){ 
			pthread_join (threadVector[i], NULL); 
			exit(0); 
    }
	//close the socket and free the data
	close(socketfd);
	free(threadData);
}