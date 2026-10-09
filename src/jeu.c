#include "jeu.h"
#include "affichage.h"
#include "file.h"
#include "joueur.h"
#include "regles.h"
#include "sauvegarde.h"
#include "utils.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FICHIER_DEFAUT "sauvegarde.ludo"

/* ------------------------------------------------------------------ */
/* Commandes saisies par un joueur humain                              */
/* ------------------------------------------------------------------ */

int jeu_proposer_sauvegarde(const Partie *p)
{
    char nom[128];
    int  r;
    printf("  Nom du fichier de sauvegarde [%s] : ", FICHIER_DEFAUT);
    fflush(stdout);
    if (!utils_lire_ligne(nom, sizeof nom)) return 0;
    if (nom[0] == '\0') strcpy(nom, FICHIER_DEFAUT);
    r = sauvegarder_partie(p, nom);
    if (r == SAUV_OK) printf("  Partie sauvegardée dans « %s ».\n", nom);
    else              printf("  Erreur : impossible d'écrire « %s ».\n", nom);
    return r == SAUV_OK;
}

/* Traite une commande d'un caractere. Retourne 1 si c'etait une commande. */
static int traiter_commande(char c, Partie *p, Historique *h, int *quitter)
{
    char rep[16];
    switch (tolower((unsigned char)c)) {
    case 'h': affichage_historique(p, h, 10); return 1;
    case 'p': affichage_plateau(p); affichage_etat_joueurs(p); return 1;
    case 'r': affichage_regles(); return 1;
    case '?': affichage_aide(); return 1;
    case 's': jeu_proposer_sauvegarde(p); return 1;
    case 'q':
        printf("  Quitter la partie ? (o/n) : ");
        fflush(stdout);
        if (!utils_lire_ligne(rep, sizeof rep) || !utils_est_oui(rep)) return 1;
        printf("  Sauvegarder avant de quitter ? (o/n) : ");
        fflush(stdout);
        if (utils_lire_ligne(rep, sizeof rep) && utils_est_oui(rep))
            jeu_proposer_sauvegarde(p);
        *quitter = 1;
        return 1;
    default:
        return 0;
    }
}

static int lancer_humain(Partie *p, Historique *h, int j, int *quitter)
{
    char ligne[32];
    const Joueur *pl = &p->joueurs[j];
    for (;;) {
        printf("\n  %s (%s) : Entrée pour lancer le dé [? = aide] ",
               pl->nom, affichage_nom_couleur(pl->siege));
        fflush(stdout);
        if (!utils_lire_ligne(ligne, sizeof ligne)) { *quitter = 1; return 0; }
        if (ligne[0] == '\0') return utils_lancer_de();
        if (ligne[1] == '\0' && traiter_commande(ligne[0], p, h, quitter)) {
            if (*quitter) return 0;
            continue;
        }
        printf("  Commande inconnue. Tapez ? pour l'aide.\n");
    }
}

static int choisir_coup_humain(Partie *p, Historique *h, const Coup *coups, int n,
                               int de, int *quitter)
{
    char ligne[32];
    affichage_coups(p, coups, n, de);
    for (;;) {
        char *fin;
        long  v;
        int   i;
        printf("  Quel pion jouer ? ");
        fflush(stdout);
        if (!utils_lire_ligne(ligne, sizeof ligne)) { *quitter = 1; return -1; }
        if (ligne[0] != '\0' && ligne[1] == '\0' &&
            traiter_commande(ligne[0], p, h, quitter)) {
            if (*quitter) return -1;
            affichage_coups(p, coups, n, de);
            continue;
        }
        v = strtol(ligne, &fin, 10);
        if (ligne[0] != '\0' && *fin == '\0')
            for (i = 0; i < n; i++)
                if (coups[i].pion + 1 == v) return i;
        printf("  Choix invalide : tapez le numéro d'un des pions proposés.\n");
    }
}

/* ------------------------------------------------------------------ */
/* Un tour de jeu                                                      */
/* ------------------------------------------------------------------ */

static void enregistrer(Historique *h, Evenement ev)
{
    ev.suivant = NULL;
    historique_empiler(h, &ev);
}

static void annoncer(const Partie *p, const Evenement *ev)
{
    char buf[220];
    affichage_decrire_evenement(p, ev, buf, sizeof buf, 0);
    affichage_message("  >> %s\n", buf);
}

int jeu_jouer_tour(Partie *p, Historique *h)
{
    int j = file_tete(&p->file);
    Joueur *pl = &p->joueurs[j];
    int humain = (pl->type == HUMAIN);
    int six = 0, quitter = 0, rejouer;

    p->nb_tours++;
    if (humain) {
        affichage_plateau(p);
        affichage_etat_joueurs(p);
    } else {
        affichage_message("\n--- Tour %d : %s (%s, ordinateur) ---\n",
                          p->nb_tours, pl->nom, affichage_nom_couleur(pl->siege));
    }

    do {
        Coup coups[NB_PIONS];
        Evenement ev;
        int de, n, choix;

        rejouer = 0;
        de = humain ? lancer_humain(p, h, j, &quitter) : utils_lancer_de();
        if (quitter) { p->nb_tours--; return TOUR_QUITTER; }

        if (humain) affichage_de(de);
        else        affichage_message("  %s lance le dé : %d\n", pl->nom, de);

        memset(&ev, 0, sizeof ev);
        ev.tour = p->nb_tours; ev.joueur = j; ev.de = de;
        ev.victime_joueur = -1; ev.victime_pion = -1;

        six = (de == VALEUR_SORTIE) ? six + 1 : 0;
        if (six >= MAX_SIX_CONSECUTIFS) {
            ev.type = EV_TROIS_SIX;
            annoncer(p, &ev);
            enregistrer(h, ev);
            break;
        }

        n = regles_coups_possibles(p, j, de, coups);
        if (n == 0) {
            ev.type = EV_PASSE;
            annoncer(p, &ev);
            enregistrer(h, ev);
            rejouer = (de == VALEUR_SORTIE);
            continue;
        }

        if (n == 1) {
            choix = 0;
            if (humain) {
                char buf[160];
                affichage_decrire_coup(p, &coups[0], buf, sizeof buf);
                printf("  Un seul coup possible, joué automatiquement : %s\n", buf);
            }
        } else if (humain) {
            choix = choisir_coup_humain(p, h, coups, n, de, &quitter);
            if (quitter) { p->nb_tours--; return TOUR_QUITTER; }
        } else {
            choix = joueur_choisir_coup_ia(p, j, coups, n);
        }

        {
            const Coup *c = &coups[choix];
            ev.type = c->sortie_base ? EV_SORTIE : EV_DEPLACEMENT;
            ev.pion = c->pion; ev.depart = c->depart; ev.arrivee = c->arrivee;
            ev.victime_joueur = c->victime_joueur; ev.victime_pion = c->victime_pion;
            regles_appliquer_coup(p, j, c);
            annoncer(p, &ev);
            enregistrer(h, ev);

            if (pl->rang > 0) {                       /* le joueur vient de gagner */
                Evenement v = ev;
                v.type = EV_VICTOIRE; v.rang = pl->rang;
                annoncer(p, &v);
                enregistrer(h, v);
                break;
            }
            rejouer = (de == VALEUR_SORTIE) || c->capture || c->arrive_maison;
        }
        if (rejouer) affichage_message("  %s rejoue !\n", pl->nom);
    } while (rejouer);

    file_defiler(&p->file);
    if (pl->rang == 0) file_enfiler(&p->file, j);
    return TOUR_OK;
}

int jeu_boucle(Partie *p, Historique *h)
{
    while (!regles_partie_terminee(p) && !file_vide(&p->file)) {
        if (jeu_jouer_tour(p, h) == TOUR_QUITTER) return 0;
    }
    regles_finaliser_classement(p);
    affichage_plateau(p);
    affichage_classement(p);
    return 1;
}
