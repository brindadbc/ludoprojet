/* jeu.h - Deroulement d'une partie (tours, saisies, boucle principale). */
#ifndef JEU_H
#define JEU_H
#include "types.h"
#include "historique.h"

#define TOUR_OK      0
#define TOUR_QUITTER 1

int jeu_jouer_tour(Partie *p, Historique *h);
int jeu_boucle(Partie *p, Historique *h);  /* 1 = partie terminee, 0 = abandon */
int jeu_proposer_sauvegarde(const Partie *p);

#endif
