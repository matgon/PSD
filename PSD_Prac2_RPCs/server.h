#include "conecta4.h"

/** Maximum number of active games in the server */
#define MAX_GAMES 5

/** Type for game status */
typedef enum {gameEmpty, gameWaitingPlayer, gameReady} tGameState;

/**
 * Struct that contains a game for 2 players
 */
typedef struct game{

	tBoard board;						/** Board of the game */
	tPlayer currentPlayer;				/** Current player */
	tString player1Name;				/** Name of player 1 */
	tString player2Name;				/** Name of player 2 */
	int endOfGame;						/** Flag to control the end of the game*/
	tGameState status;					/** Flag to indicate the status of this game */

}tGame;


/**
 * Initialize server structures
 */
void initServerStructures ();

/**
 * Switch the current player
 *
 * @param currentPlayer Current player
 * @return Player that is currently awaiting.
 */
tPlayer switchPlayer (tPlayer currentPlayer);


/**
 * Search for a game.
 *
 * This function returns a number between [0-MAX_GAMES] if there
 * exists an empty game, or ERROR_SERVER_FULL in other case.
 */
int searchEmptyGame ();


/**
 * Locate the game where a given player is active.
 *
 * @param player - Player name
 * @return If the player is found, then the index of the game is
 * returned [0-MAX_GAMES]. In other case, this function returns ERROR_PLAYER_NOT_FOUND.
 */
int locateGameForPlayer (tString player);


/**
 * Init the structure of a terminated game
 * @param index - Index of the game structure in the array
 */
void initGameByIndex (int index);

