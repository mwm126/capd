#ifndef COUNTERFUNCTIONS_H
#define COUNTERFUNCTIONS_H

#include "globalDefinitions.h"

/*********************************************/
/*       Counter Manangement Functions       */
/*********************************************/

#define MAX_NO_OF_COUNTER_ENTRIES MAX_NO_OF_CONNECTIONS

int searchCounterFile(FILE *f, int serial);

void updateCounterFileEntry(FILE *f, int serial, int counter);

#endif
