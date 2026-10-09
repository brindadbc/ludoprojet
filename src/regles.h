/* regles.h - Logique pure du jeu (aucun affichage, aucune saisie). */
#ifndef REGLES_H
#define REGLES_H
#include "types.h"

void regles_init_partie(Partie *p);          /* pions en base, file reconstruite */
int  regles_case_absolue(int siege, int pos);/* 0..51 sur la piste, -1 sinon */
int  regles_case_sure(int absolue);
int  regles_coups_possibles(const Partie *p, int j, int de, Coup coups[NB_PIONS]);
void regles_appliquer_coup(Partie *p, int j, const Coup *c);
int  regles_a_gagne(const Joueur *pl);
int  regles_partie_terminee(const Partie *p);
void regles_finaliser_classement(Partie *p);
void regles_classement(const Partie *p, int ordre[MAX_JOUEURS]);
int  regles_score_avancement(const Joueur *pl);
void regles_compter(const Joueur *pl, int *base, int *piste, int *couloir, int *arrives);

#endif
