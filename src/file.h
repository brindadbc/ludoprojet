/* file.h - File circulaire (FIFO) pour l'ordre de passage des joueurs. */
#ifndef FILE_H
#define FILE_H
#include "types.h"

void file_init(FileTours *f);
int  file_vide(const FileTours *f);
int  file_enfiler(FileTours *f, int joueur);   /* 0 ok, -1 si pleine */
int  file_defiler(FileTours *f);               /* -1 si vide */
int  file_tete(const FileTours *f);            /* -1 si vide */
void file_reconstruire(Partie *p, int premier);

#endif
