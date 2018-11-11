#include "savetime_switch.h"
#include <time.h>

#define SAVE_NAME_LEN 256

typedef struct FileTime
{
    int time;
    char name[SAVE_NAME_LEN];
} filetime_t;

typedef struct FILETIMES {
    filetime_t *data;
    struct FILETIMES *next;
} filetimes_t;

struct FILETIMES *filetimes;

void SaveTime_Init( void )
{
    file_t	*f;

    f = FS_Open( "savetimes", "r", true );
	if( !f ) {
        return;
    }

    int len = FS_FileLength( f ) / sizeof(struct FileTime) ;

    for(int i = 0; i < len; ++i)
    {
        struct FileTime* filetime = malloc(sizeof(struct FileTime));
        FS_Seek( f, i * sizeof(struct FileTime), SEEK_SET );
        FS_Read( f, filetime, sizeof(struct FileTime));

        if (filetimes == NULL) {
            filetimes = malloc(sizeof(struct FILETIMES));
            filetimes->data = filetime;
            filetimes->next = NULL;
        } else {
            filetimes_t *current = filetimes;

            while (current->next != NULL) {
                current = current->next;
            }

            current->next = malloc(sizeof(struct FILETIMES));
            current->next->data = filetime;
            current->next->next = NULL;
        }
    }

    filetimes_t *current = filetimes;

    while (current->next != NULL) {
        current = current->next;
    }

    FS_Close( f );
}

void SaveTime_Shutdown( void )
{
    filetimes_t *current = filetimes;
    filetimes_t *next;

    free(current->data);

    while (current->next != NULL) {
        next = current->next;
        free(current->data);
        free(current);
        current = next;
    }

    free(filetimes);
}

void SaveTime_Save( void )
{
    file_t	*f;

    if (filetimes == NULL) {
        printf("Savetime is NULL\n");
        return;
    }

    f = FS_Open( "savetimes", "w", true );
	if( !f ) {
        printf("Unable to create savetimes file\n");
        return;
    }

    filetimes_t *current = filetimes;

    while (current->next != NULL) {
        FS_Write( f, current->data, sizeof(struct FileTime) );
        current = current->next;
    }

    FS_Write( f, current->data, sizeof(struct FileTime) );

    FS_Close( f );
}

void SaveTime_SetSaveFiletime( const char *name )
{
    struct FileTime* filetime = malloc(sizeof(struct FileTime));
    filetime->time = (int)time(NULL);
    strcpy(filetime->name, name);

    if (filetimes == NULL) {
        filetimes = malloc(sizeof(struct FILETIMES));
        filetimes->data = filetime;
        filetimes->next = NULL;
    } else {
        filetimes_t *current = filetimes;

        if (strcmp( name, current->data->name ) == 0) {
            free(current->data);
            current->data = filetime;
            SaveTime_Save();
            return;
        }

        while (current->next != NULL) {
            current = current->next;

            if (strcmp( name, current->data->name ) == 0) {
                free(current->data);
                current->data = filetime;
                SaveTime_Save();
                return;
            }
        }

        current->next = malloc(sizeof(struct FILETIMES));
        current->next->data = filetime;
        current->next->next = NULL;
    }

    SaveTime_Save();
}

int SaveTime_GetSaveFiletime( const char *name )
{
    if (filetimes != NULL) {
        filetimes_t *filetime = filetimes;

        while (filetime != NULL) {
            if (strcmp( name, filetime->data->name ) == 0) {
                return filetime->data->time;
            }

            filetime = filetime->next;
        }
    }

    return 0;
}