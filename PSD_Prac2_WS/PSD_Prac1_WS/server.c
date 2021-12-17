#include "server.h"
#include <pthread.h>

/** Flag to enable debugging */
#define DEBUG_SERVER 1


/** Array with games */
tGame games[MAX_GAMES];
pthread_mutex_t mutexGameArray[MAX_GAMES];

void initServerStructures (){

	int i;

	if (DEBUG_SERVER)
		printf ("Initializing...\n");

	// Init seed
	srand (time(NULL));

	// Init each game
	for (i=0; i<MAX_GAMES; i++){

		// Allocate and init board
		games[i].board = (xsd__string) malloc (BOARD_WIDTH*BOARD_HEIGHT);
		initBoard (games[i].board);

		// Calculate the first player to play
		if ((rand()%2)==0)
			games[i].currentPlayer = player1;
		else
			games[i].currentPlayer = player2;

		// Allocate and init player names
		games[i].player1Name = (xsd__string) malloc (STRING_LENGTH);
		games[i].player2Name = (xsd__string) malloc (STRING_LENGTH);
		memset (games[i].player1Name, 0, STRING_LENGTH);
		memset (games[i].player2Name, 0, STRING_LENGTH);

		// Game status
		games[i].endOfGame = FALSE;
		games[i].status = gameEmpty;
	}
}

conecta4ns__tPlayer switchPlayer (conecta4ns__tPlayer currentPlayer){

	conecta4ns__tPlayer nextPlayer;

		if (currentPlayer == player1)
			nextPlayer = player2;
		else
			nextPlayer = player1;

	return nextPlayer;
}

int searchEmptyGame (){

	int i, found, result;

		// Init...
		found = FALSE;
		i = 0;
		result = -1;

		// Search for a non-active game
		while ((!found) && (i<MAX_GAMES)){

			if ((games[i].status == gameEmpty) ||
				(games[i].status == gameWaitingPlayer)){
				found = TRUE;
			}
			else
				i++;
		}

		if (found){
			result = i;
		}
		else{
			result = ERROR_SERVER_FULL;
		}

	return result;
}

int locateGameForPlayer (xsd__string name){
	
	int i = 0, found = FALSE, result;
	
	while ((!found) && (i<MAX_GAMES)){
		if (strcmp(games[i].player1Name, name) == 0 || strcmp(games[i].player2Name, name) == 0)
			found = TRUE;
	else
			i++;
	}

	if(found){
		result = i;
	}
	else{
		result = ERROR_PLAYER_NOT_FOUND;
	}

	return result;

}

void freeGameByIndex (int index){

	initBoard (games[index].board);

	if ((rand()%2)==0)
		games[index].currentPlayer = player1;
	else
		games[index].currentPlayer = player2;

	memset (games[index].player1Name, 0, STRING_LENGTH);
	memset (games[index].player2Name, 0, STRING_LENGTH);
	games[index].endOfGame = FALSE;
	games[index].status = gameEmpty;
}


int conecta4ns__register (struct soap *soap, conecta4ns__tMessage playerName, int *code){

	static int  result;
	int gameIndex;

	// Init...
	gameIndex = -1;

	// Looks if the name is already registered
	if(locateGameForPlayer(playerName.msg) != ERROR_PLAYER_NOT_FOUND)
		*code = ERROR_NAME_REPEATED;

	// Search fon an empty game
	gameIndex = searchEmptyGame();

	if(gameIndex == ERROR_SERVER_FULL)
		*code = ERROR_SERVER_FULL;
	else if(*code != ERROR_NAME_REPEATED){
		// If the game is empty fill player 1 and change game status
		if(games[gameIndex].status == gameEmpty){

			strcpy(games[gameIndex].player1Name,  playerName.msg);
			games[gameIndex].status = gameWaitingPlayer;

		// If the game is waiting fill player 2 and change game status
		}else if(games[gameIndex].status == gameWaitingPlayer){

			strcpy(games[gameIndex].player2Name, playerName.msg);
			games[gameIndex].status = gameReady;

		// If the server is full
		}

		*code = OK_NAME_REGISTERED;
	}

  return SOAP_OK;
}

int conecta4ns__getStatus (struct soap *soap, conecta4ns__tMessage playerName, conecta4ns__tBlock* status){

	int gameIndex;
	char messageToPlayer [STRING_LENGTH];

		// Set \0 at the end of the string
		playerName.msg[playerName.__size] = 0;

		// Allocate memory for the status structure
		(status->msgStruct).msg = (xsd__string) malloc (STRING_LENGTH);
		memset ((status->msgStruct).msg, 0, STRING_LENGTH);
		status->board = (xsd__string) malloc (BOARD_WIDTH*BOARD_HEIGHT);
		memset (status->board, 0, BOARD_WIDTH*BOARD_HEIGHT);
		status->__size = BOARD_WIDTH*BOARD_HEIGHT;


		if (DEBUG_SERVER)
			printf ("Receiving getStatus() request from -> %s [%d]\n", playerName.msg, playerName.__size);


		//Get the game index for the player
		gameIndex = locateGameForPlayer(playerName.msg);

	if(gameIndex == ERROR_PLAYER_NOT_FOUND){	//player not found
			
		status->code = ERROR_PLAYER_NOT_FOUND;

	}else if(games[gameIndex].status != gameWaitingPlayer){ //if the game is ready

		unsigned int p;

		if(strcmp(playerName.msg, games[gameIndex].player1Name))
			p = player1;
		else
			p = player2;
		
		if(checkWinner(games[gameIndex].board, switchPlayer(games[gameIndex].currentPlayer))){ //if the other player has won
			
			status->code = GAMEOVER_LOSE;
			strcpy(status->msgStruct.msg, "YOU LOSE.");
			strcpy(status->board, games[gameIndex].board);
			games[gameIndex].endOfGame = TRUE;
		
		}else if(isBoardFull(games[gameIndex].board)){

			games[gameIndex].endOfGame = TRUE;
			status->code = GAMEOVER_DRAW;
			strcpy(status->msgStruct.msg, "ITS A DRAW.");

		}else { //if the game continuous

			if(games[gameIndex].currentPlayer == p){

				status->code = TURN_MOVE;
				strcpy(status->msgStruct.msg, "ITS YOUR TURN");
				//locks the thread
				pthread_mutex_lock(&mutexGameArray[gameIndex]);

			}else{

				status->code = TURN_WAIT;
				strcpy(status->msgStruct.msg, "YOU ARE WAITING");
				//waits for the thread to be unlocked
				pthread_mutex_lock(&mutexGameArray[gameIndex]);
				pthread_mutex_unlock(&mutexGameArray[gameIndex]);

			}

		}

	}else{ //if the game is not ready yet

		status->code = gameWaitingPlayer;
		strcpy(status->msgStruct.msg, "THERE IS NO PLAYER 2");

	}
	if(games[gameIndex].endOfGame == FALSE)
		strcpy(status->board, games[gameIndex].board);
	else
		freeGameByIndex(gameIndex);
	status->__size = strlen(status->board);
	status->msgStruct.__size = strlen(status->msgStruct.msg);

	return SOAP_OK;
}

int conecta4ns__insertChip (struct soap *soap, conecta4ns__tMessage playerName, int playerMove, conecta4ns__tBlock* status){

	int gameIndex;
	conecta4ns__tMove moveResult;

		// Set \0 at the end of the string
		playerName.msg[playerName.__size] = 0;

		// Allocate memory for the status structure
		(status->msgStruct).msg = (xsd__string) malloc (STRING_LENGTH);
		memset ((status->msgStruct).msg, 0, STRING_LENGTH);
		status->board = (xsd__string) malloc (BOARD_WIDTH*BOARD_HEIGHT);
		memset (status->board, 0, BOARD_WIDTH*BOARD_HEIGHT);

		// Locate the game
		gameIndex = locateGameForPlayer (playerName.msg);
		
		if(checkMove(games[gameIndex].board, playerMove) == OK_move){

			insertChip(games[gameIndex].board, games[gameIndex].currentPlayer, playerMove);

			// the response field
			if(checkWinner(games[gameIndex].board, games[gameIndex].currentPlayer)){

				status->code = GAMEOVER_WIN;
				strcpy(status->msgStruct.msg, "YOU WIN!!!");

			}else if(isBoardFull(games[gameIndex].board)){

				status->code = GAMEOVER_DRAW;
				strcpy(status->msgStruct.msg, "ITS A DRAW");
				
			}else {

				status->code = TURN_WAIT;
				strcpy(status->msgStruct.msg, "YOU ARE WAITING.");

			}
			//unlocks the thread when the chip is inserted
			pthread_mutex_unlock(&mutexGameArray[gameIndex]);

			games[gameIndex].currentPlayer = switchPlayer(games[gameIndex].currentPlayer);

		}else{

			status->code = TURN_MOVE;
			strcpy(status->msgStruct.msg, "Movimiento erroneo, introduzcalo de nuevo: ");
		
		}
		strcpy(status->board, games[gameIndex].board);
		status->__size = strlen(status->board);
		status->msgStruct.__size = strlen(status->msgStruct.msg); 
		

	return SOAP_OK;
}

void *processRequest(void *soap){

	pthread_detach(pthread_self());

	if (DEBUG_SERVER)
		printf ("Processing a new request...");

	soap_serve((struct soap*)soap);
	soap_destroy((struct soap*)soap);
	soap_end((struct soap*)soap);
	soap_done((struct soap*)soap);
	free(soap);

	return NULL;
}

int main(int argc, char **argv){ 

	struct soap soap;
	struct soap *tsoap;
	pthread_t tid;
	int port;
	SOAP_SOCKET m, s;

		// Init soap environment
		soap_init(&soap);

		// Configure timeouts
		soap.send_timeout = 60; // 60 seconds
		soap.recv_timeout = 60; // 60 seconds
		soap.accept_timeout = 3600; // server stops after 1 hour of inactivity
		soap.max_keep_alive = 100; // max keep-alive sequence

		initServerStructures();
		pthread_mutex_init(&mutexGameArray, NULL);

		// Get listening port
		port = atoi(argv[1]);

		// Bind
		m = soap_bind(&soap, NULL, port, 100);

		if (!soap_valid_socket(m)){
			exit(1);
		}

		printf("Server is ON!\n", m);

		while (TRUE){

			// Accept a new connection
			s = soap_accept(&soap);

			// Socket is not valid :(
			if (!soap_valid_socket(s)){

				if (soap.errnum){
					soap_print_fault(&soap, stderr);
					exit(1);
				}

				fprintf(stderr, "Time out!\n");
				break;
			}

			// Copy the SOAP environment
			tsoap = soap_copy(&soap);

			if (!tsoap){
				printf ("SOAP copy error!\n");
				break;
			}

			// Create a new thread to process the request
			pthread_create(&tid, NULL, (void*(*)(void*))processRequest, (void*)tsoap);
		}

	// Detach SOAP environment
	soap_done(&soap);
	return 0;
}

