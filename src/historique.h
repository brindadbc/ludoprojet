/* historique.h - Pile (liste chainee) des evenements de la partie. */
#ifndef HISTORIQUE_H
#define HISTORIQUE_H

typedef enum {
    EV_SORTIE, EV_DEPLACEMENT, EV_PASSE, EV_TROIS_SIX, EV_VICTOIRE
} TypeEvenement;

typedef struct Evenement {
    TypeEvenement     type;
    int               tour;
    int               joueur;
    int               de;
    int               pion;
    int               depart;
    int               arrivee;
    int               victime_joueur;   /* -1 si aucune */
    int               victime_pion;
    int               rang;             /* pour EV_VICTOIRE */
    struct Evenement *suivant;
} Evenement;

typedef struct {
    Evenement *sommet;
    int        taille;
} Historique;

void historique_init(Historique *h);
int  historique_empiler(Historique *h, const Evenement *ev);  /* 0 ok, -1 erreur */
int  historique_depiler(Historique *h, Evenement *out);       /* 0 ok, -1 vide */
void historique_liberer(Historique *h);

#endif
