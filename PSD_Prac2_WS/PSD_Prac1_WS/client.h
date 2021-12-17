#include "soapH.h"
#include "conecta4.nsmap"
#include "game.h"
#include <signal.h>




/** Timer interval */
#define TIMER_SEC 3


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
