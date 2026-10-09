/* joueur.h - Creation des joueurs et intelligence artificielle. */
#ifndef JOUEUR_H
#define JOUEUR_H
#include "types.h"

void joueur_init(Joueur *pl, const char *nom, TypeJoueur type, int siege);
/* Retourne l'indice (dans coups[]) du coup choisi par l'ordinateur. */
int  joueur_choisir_coup_ia(const Partie *p, int j, const Coup *coups, int n);

#endif
