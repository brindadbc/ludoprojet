/* sauvegarde.h - Sauvegarde et chargement d'une partie (fichier texte). */
#ifndef SAUVEGARDE_H
#define SAUVEGARDE_H
#include "types.h"

#define SAUV_OK               0
#define SAUV_ERREUR_FICHIER   1
#define SAUV_ERREUR_FORMAT    2

int sauvegarder_partie(const Partie *p, const char *chemin);
int charger_partie(Partie *p, const char *chemin);

#endif
