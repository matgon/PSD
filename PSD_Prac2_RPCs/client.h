#include "conecta4.h"
#include "game.h"
#include <signal.h>

/**
 * Set the timer
 */
void setTimer ();

/**
 * Stop the timer
 */
void stopTimer ();

/**
 * Function to handle the alarm signal
 */
void *alarmHandler (void);

/**
 * Read a player move
 * @return A number between [0-6]
 */
unsigned int readMove ();

/**
 * Starts a game in the client-side
 * @param host - Server address
 */
void startGame(char *host);
