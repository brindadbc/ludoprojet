/*
 * types.h - Constantes et structures de donnees communes a tout le projet.
 *
 * Modele de position d'un pion (progression relative au joueur) :
 *   -1        : le pion est dans sa base
 *    0 .. 50  : le pion est sur la piste commune (0 = case de depart)
 *   51 .. 55  : le pion est dans son couloir d'arrivee (5 cases protegees)
 *   56        : le pion est arrive a la maison
 */
#ifndef TYPES_H
#define TYPES_H

#define MAX_JOUEURS          4
#define NB_PIONS             4
#define MAX_NOM              21
#define LONGUEUR_PISTE       52
#define ECART_DEPART         13
#define POS_BASE             (-1)
#define POS_DEPART           0
#define POS_FIN_PISTE        50
#define POS_DEBUT_COULOIR    51
#define POS_ARRIVEE          56
#define VALEUR_SORTIE        6
#define MAX_SIX_CONSECUTIFS  3

typedef enum { HUMAIN = 0, ORDINATEUR = 1 } TypeJoueur;
typedef enum { FIN_PREMIER_VAINQUEUR = 0, FIN_CLASSEMENT_COMPLET = 1 } ModeFin;

/* Un joueur : nom, type, siege (couleur 0..3) et position de ses 4 pions. */
typedef struct {
    char       nom[MAX_NOM];
    TypeJoueur type;
    int        siege;               /* 0 Rouge, 1 Vert, 2 Jaune, 3 Bleu */
    int        pions[NB_PIONS];
    int        rang;                /* 0 = pas encore classe, sinon 1,2,3... */
} Joueur;

/* Un deplacement possible pour un pion, avec ses consequences. */
typedef struct {
    int pion;
    int depart;
    int arrivee;
    int sortie_base;
    int capture;
    int victime_joueur;             /* -1 si pas de capture */
    int victime_pion;
    int arrive_maison;
} Coup;

/* File circulaire (FIFO) de l'ordre de passage des joueurs. */
typedef struct {
    int elements[MAX_JOUEURS];
    int tete;
    int taille;
} FileTours;

/* L'etat complet d'une partie. */
typedef struct {
    Joueur    joueurs[MAX_JOUEURS];
    int       nb_joueurs;
    int       nb_classes;
    int       nb_tours;
    ModeFin   mode;
    FileTours file;                 /* file.elements[file.tete] = joueur courant */
} Partie;

#endif
