/** Player has been successfully registered */
const OK_NAME_REGISTERED = 10000;

/** A player is already registered with the same name */
const ERROR_NAME_REPEATED = 10001;

/** Server is full. No more games are allowed */
const ERROR_SERVER_FULL = 10002;

/** Player not found */
const ERROR_PLAYER_NOT_FOUND = 10003;

/** Code for performing a move */
const TURN_MOVE = 70001;

/** Code for waiting */
const TURN_WAIT = 70002;

/** Code to show that the player wins */
const GAMEOVER_WIN = 50052;

/** Code to show a draw game */
const GAMEOVER_DRAW = 50053;

/** Code to show that the player loses */
const GAMEOVER_LOSE = 50054;

/** Board width (in number of cells) */
const BOARD_WIDTH = 7;

/** Board height (in number of cells) */
const BOARD_HEIGHT = 6;

/** Total number of cells in the board */
const BOARD_CELLS = 42;

/** Character for player 1 chip */
const PLAYER_1_CHIP = 111;

/** Character for player 2 chip */
const PLAYER_2_CHIP = 120;

/** Character for empty cell */
const EMPTY_CELL = 32;

/** Length for tString */
const STRING_LENGTH = 128;

/** Board */
typedef char tBoard [BOARD_CELLS];

/** Type for names, messages and this kind of variables */
typedef char tString [STRING_LENGTH];

/** Players */
enum tPlayer{player1, player2};

/** Result for moves */
enum tMove{OK_move, fullColumn_move};


/** Message used to send the player's name */
struct tMessage{
	tString msg;
};

/** Move performed by the player */
struct tColumn{
	unsigned int column;
	tString player;
};

/** Response from the server */
struct tBlock{
	unsigned int code;
	tString msg;
	tBoard board;
};

program CONECTA4 {
	version CONECTA4VER {
		int registerPlayer (tMessage) = 1; 
		tBlock getGameStatus (tMessage) = 2;
		tBlock insertChipInBoard (tColumn) = 3;		
	} = 1;
} = 99;
