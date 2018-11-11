#ifndef SAVETIME_SWITCH_H
#define SAVETIME_SWITCH_H

#include "common.h"

void SaveTime_Init( void );
void SaveTime_Shutdown( void );
void SaveTime_SetSaveFiletime( const char *name );
int SaveTime_GetSaveFiletime( const char *name );

#endif
