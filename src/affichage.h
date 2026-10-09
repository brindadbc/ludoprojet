/* affichage.h - Tout ce qui s'affiche dans le terminal. */
#ifndef AFFICHAGE_H
#define AFFICHAGE_H
#include <stddef.h>
#include "types.h"
#include "historique.h"

void        affichage_init(int couleur);
void        affichage_silencieux(int actif);
void        affichage_message(const char *fmt, ...);

const char *affichage_nom_couleur(int siege);
char        affichage_lettre(int siege);
void        affichage_decrire_position(int pos, char *buf, size_t n);
void        affichage_decrire_coup(const Partie *p, const Coup *c, char *buf, size_t n);
void        affichage_decrire_evenement(const Partie *p, const Evenement *ev,
                                        char *buf, size_t n, int avec_tour);

void        affichage_titre(void);
void        affichage_plateau(const Partie *p);
void        affichage_etat_joueurs(const Partie *p);
void        affichage_de(int valeur);
void        affichage_coups(const Partie *p, const Coup *coups, int n, int de);
void        affichage_historique(const Partie *p, const Historique *h, int max);
void        affichage_classement(const Partie *p);
void        affichage_regles(void);
void        affichage_aide(void);
void        affichage_equipe(void);

#endif
