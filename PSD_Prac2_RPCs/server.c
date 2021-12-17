#include "server.h"

/** Flag to enable debugging */
#define DEBUG_SERVER 1


/** Array with games */
tGame games[MAX_GAMES];


void initServerStructures (){

	int i;

		if (DEBUG_SERVER)
			printf ("\nServer is on!!!Initializing...\n");

		// Init seed
		srand (time(NULL));

		// Init each game
		for (i=0; i<MAX_GAMES; i++){

			initBoard (games[i].board);

			if ((rand()%2)==0)
				games[i].currentPlayer = player1;
			else
				games[i].currentPlayer = player2;

			memset (games[i].player1Name, 0, STRING_LENGTH);
			memset (games[i].player2Name, 0, STRING_LENGTH);
			games[i].endOfGame = FALSE;
			games[i].status = gameEmpty;
		}
}


tPlayer switchPlayer (tPlayer currentPlayer){

	tPlayer nextPlayer;

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


int locateGameForPlayer (tString name){

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


void initGameByIndex (int index){

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


int *registerplayer_1_svc(tMessage *argMsg, struct svc_req *rqstp){

	static int  result;
	int gameIndex;

	// Init...
	gameIndex = -1;

	// Looks if the name is already registered
	if(locateGameForPlayer(argMsg->msg) != ERROR_PLAYER_NOT_FOUND){
		result = ERROR_NAME_REPEATED;
	}

	// Search fon an empty game
	gameIndex = searchEmptyGame();
	if(gameIndex == ERROR_SERVER_FULL)
		result = ERROR_SERVER_FULL;
	else if(result != ERROR_NAME_REPEATED){
		// If the game is empty fill player 1 and change game status
		if(games[gameIndex].status == gameEmpty){

			strcpy(games[gameIndex].player1Name,  argMsg->msg);
			games[gameIndex].status = gameWaitingPlayer;
			result = OK_NAME_REGISTERED;

		// If the game is waiting fill player 2 and change game status
		}else if(games[gameIndex].status == gameWaitingPlayer){

			strcpy(games[gameIndex].player2Name, argMsg->msg);
			games[gameIndex].status = gameReady;
			result = OK_NAME_REGISTERED;

		// If the server is full
		}
	}

	return &result;
}


tBlock *getgamestatus_1_svc(tMessage *argMsg, struct svc_req *rqstp){
	
	tBlock *response;
	int gameIndex;
	// Init...
	
	response = (tBlock*) malloc (sizeof (tBlock));
	// Locate the game
	gameIndex = locateGameForPlayer(argMsg->msg);

	if(gameIndex == ERROR_PLAYER_NOT_FOUND){
			
		response->code = ERROR_PLAYER_NOT_FOUND;

	}else if(games[gameIndex].status != gameWaitingPlayer){

		tPlayer p;

		if(strcmp(argMsg->msg, games[gameIndex].player1Name))
			p = player1;
		else
			p = player2;
		
		if(checkWinner(games[gameIndex].board, switchPlayer(games[gameIndex].currentPlayer))){
			
			response->code = GAMEOVER_LOSE;
			strcpy(response->msg, "YOU LOSE.");
			strcpy(response->board, games[gameIndex].board);
			games[gameIndex].endOfGame = TRUE;
			
		}else if(isBoardFull(games[gameIndex].board)){

			games[gameIndex].endOfGame = TRUE;
			response->code = GAMEOVER_DRAW;
			strcpy(response->msg, "ITS A DRAW.");

		}else {

			if(games[gameIndex].currentPlayer == p){

				response->code = TURN_MOVE;
				strcpy(response->msg, "ITS YOUR TURN");

			}else{

				response->code = TURN_WAIT;
				strcpy(response->msg, "YOU ARE WAITING");

			}

		}

	}else{//if the game is not ready yet

		response->code = gameWaitingPlayer;
		strcpy(response->msg, "THERE IS NO PLAYER 2");

	}
	if(games[gameIndex].endOfGame == FALSE)
		strcpy(response->board, games[gameIndex].board);
	else
		initGameByIndex(gameIndex);

	return response;
}


tBlock * insertchipinboard_1_svc(tColumn *argCol, struct svc_req *rqstp){

	tMove moveResult;
	tBlock *response;
	int gameIndex;

	// Init...
	response = (tBlock*) malloc (sizeof (tBlock));

	// Locate the game
	gameIndex = locateGameForPlayer (argCol->player);
	
	if(checkMove(games[gameIndex].board, argCol->column) == OK_move){

		insertChip(games[gameIndex].board, games[gameIndex].currentPlayer, argCol->column);
		
		// fill the response field
		if(checkWinner(games[gameIndex].board, games[gameIndex].currentPlayer)){

			response->code = GAMEOVER_WIN;
			strcpy(response->msg, "YOU WIN!!!");
			
		}else{

			response->code = TURN_WAIT;
			strcpy(response->msg, "");
		}


		games[gameIndex].currentPlayer = switchPlayer(games[gameIndex].currentPlayer);

	}else{

		response->code = TURN_MOVE;
		strcpy(response->msg, "Movimiento erroneo, introduzcalo de nuevo: ");
	
	}
	strcpy(response->board, games[gameIndex].board);

	return response;
}
