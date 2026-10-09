/*
 * LudoProjet - Jeu de Ludo en langage C (console).
 * Projet Algo et Prog - ISESTMA 2026-2027 - Groupe 1.
 */
#include "affichage.h"
#include "file.h"
#include "historique.h"
#include "jeu.h"
#include "joueur.h"
#include "regles.h"
#include "sauvegarde.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void aide_ligne_commande(const char *prog)
{
    printf("Usage : %s [options]\n"
           "  --no-color     désactive les couleurs ANSI\n"
           "  --seed N       fixe la graine du générateur aléatoire (parties reproductibles)\n"
           "  --help         affiche cette aide\n", prog);
}

static void nettoyer_nom(char *nom)
{
    char *c;
    for (c = nom; *c; c++)
        if (*c == ' ' || *c == '\t') *c = '_';
}

/* Saisie interactive de la configuration. Retourne 0 si OK, -1 si abandon. */
static int configurer_partie(Partie *p)
{
    static const int SIEGES_2[2] = { 0, 2 };
    int n, i, choix;

    n = utils_lire_entier("\n  Nombre de joueurs (2 à 4) : ", 2, 4);
    if (n < 0) return -1;
    memset(p, 0, sizeof *p);
    p->nb_joueurs = n;

    for (i = 0; i < n; i++) {
        char invite[96], nom[64], defaut[MAX_NOM];
        int  siege = (n == 2) ? SIEGES_2[i] : i;
        int  type;

        printf("\n  Joueur %d - couleur %s\n", i + 1, affichage_nom_couleur(siege));
        type = utils_lire_entier("    Type (1 = humain, 2 = ordinateur) : ", 1, 2);
        if (type < 0) return -1;
        snprintf(defaut, sizeof defaut, "%s%d", type == 1 ? "Joueur" : "Ordi", i + 1);
        snprintf(invite, sizeof invite, "    Nom [%s] : ", defaut);
        printf("%s", invite);
        fflush(stdout);
        if (!utils_lire_ligne(nom, sizeof nom)) return -1;
        if (nom[0] == '\0') strcpy(nom, defaut);
        nettoyer_nom(nom);
        joueur_init(&p->joueurs[i], nom, type == 1 ? HUMAIN : ORDINATEUR, siege);
    }

    choix = utils_lire_entier("\n  Fin de partie : 1 = premier vainqueur, 2 = classement complet : ", 1, 2);
    if (choix < 0) return -1;
    p->mode = (choix == 1) ? FIN_PREMIER_VAINQUEUR : FIN_CLASSEMENT_COMPLET;
    regles_init_partie(p);
    return 0;
}

static void jouer(Partie *p)
{
    Historique h;
    historique_init(&h);
    jeu_boucle(p, &h);
    historique_liberer(&h);
}

static void menu_charger(void)
{
    Partie p;
    char nom[128];
    int r;
    printf("  Fichier à charger [sauvegarde.ludo] : ");
    fflush(stdout);
    if (!utils_lire_ligne(nom, sizeof nom)) return;
    if (nom[0] == '\0') strcpy(nom, "sauvegarde.ludo");
    r = charger_partie(&p, nom);
    if (r == SAUV_ERREUR_FICHIER)      printf("  Fichier introuvable : %s\n", nom);
    else if (r == SAUV_ERREUR_FORMAT)  printf("  Fichier de sauvegarde invalide.\n");
    else {
        printf("  Partie chargée (%d joueurs, tour %d).\n", p.nb_joueurs, p.nb_tours);
        jouer(&p);
    }
}

int main(int argc, char **argv)
{
    int couleur = 1, i, choix;
    unsigned int graine = utils_graine_horloge();

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--no-color") == 0) couleur = 0;
        else if (strcmp(argv[i], "--seed") == 0 && i + 1 < argc)
            graine = (unsigned int)strtoul(argv[++i], NULL, 10);
        else { aide_ligne_commande(argv[0]); return strcmp(argv[i], "--help") == 0 ? 0 : 1; }
    }
    affichage_init(couleur);
    utils_init_alea(graine);

    affichage_titre();
    for (;;) {
        printf("\n  1. Nouvelle partie\n"
               "  2. Charger une partie\n"
               "  3. Règles du jeu\n"
               "  4. L'équipe\n"
               "  0. Quitter\n");
        choix = utils_lire_entier("  Votre choix : ", 0, 4);
        if (choix <= 0) break;
        switch (choix) {
        case 1: {
            Partie p;
            if (configurer_partie(&p) == 0) jouer(&p);
            break;
        }
        case 2: menu_charger(); break;
        case 3: affichage_regles(); break;
        case 4: affichage_equipe(); break;
        }
    }
    printf("\n  À bientôt !\n");
    return 0;
}
