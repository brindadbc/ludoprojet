/*
 * gui_common.h - Declarations communes de l'interface graphique (raylib).
 */
#ifndef GUI_COMMON_H
#define GUI_COMMON_H

#include "raylib.h"
#include "types.h"

#define LARGEUR 1100
#define HAUTEUR 720
#define CS      44          /* taille d'une cellule du plateau (pixels) */
#define BX      30          /* coin haut-gauche du plateau */
#define BY      30

/* Fichier de sauvegarde. Version web : dossier /data, conserve par le navigateur (IndexedDB). */
#ifdef PLATFORM_WEB
#define FICHIER_SAUVEGARDE "/data/sauvegarde.ludo"
#else
#define FICHIER_SAUVEGARDE "sauvegarde.ludo"
#endif

typedef enum { ECRAN_MENU = 0, ECRAN_JEU = 1 } Ecran;
typedef enum { MENU_RIEN, MENU_JOUER, MENU_CHARGER, MENU_QUITTER } MenuAction;

/* Configuration choisie dans le menu. */
typedef struct {
    int     nb_joueurs;
    char    noms[MAX_JOUEURS][MAX_NOM];
    int     ia[MAX_JOUEURS];           /* 1 = ordinateur */
    ModeFin mode;
} Config;

/* ---- main_gui.c ---- */
void gui_sauvegarde_persistante(void);     /* web : ecrit la sauvegarde dans le navigateur */

/* ---- gui_dessin.c : outils de dessin ---- */
void  gui_charger_police(void);
void  gui_liberer_police(void);
void  gui_texte(const char *s, float x, float y, float taille, Color c);
void  gui_texte_gras(const char *s, float x, float y, float taille, Color c);
float gui_largeur(const char *s, float taille);
void  gui_texte_centre(const char *s, float cx, float y, float taille, Color c);
void  gui_texte_tronque(const char *s, float x, float y, float taille, Color c, float larg_max);
void  gui_nom_affiche(const char *nom, char *sortie, int taille);

Color gui_couleur(int siege);
Color gui_couleur_claire(int siege);
Color gui_couleur_sombre(int siege);
Color gui_contraste(Color fond);

int   gui_bouton(Rectangle r, const char *label, Color fond, float taille, int actif);
void  gui_ombre_carte(Rectangle r, Color fond);

void  gui_centre_cellule(int ligne, int colonne, float *x, float *y);
void  gui_dessiner_plateau(const Partie *p);
void  gui_dessiner_pion(float x, float y, float r, int siege, int numero, float lueur);
void  gui_dessiner_de(float cx, float cy, float taille, int valeur, Color accent);
void  gui_fond_modal(void);
int   gui_modal_regles(void);              /* retourne 1 si on doit fermer */

/* ---- gui_menu.c ---- */
void       gui_config_defaut(Config *c);
int        gui_siege_de_rang(int nb_joueurs, int rang);
MenuAction gui_menu(Config *cfg);
void       gui_menu_message(const char *msg);

/* ---- gui_jeu.c ---- */
void gui_jeu_demarrer(const Config *cfg);
int  gui_jeu_charger(const char *chemin);
int  gui_jeu_frame(void);                  /* retourne 1 pour revenir au menu */
void gui_jeu_vitesse(float facteur);

#endif
