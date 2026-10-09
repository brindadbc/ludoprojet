#include "sauvegarde.h"
#include "file.h"
#include <stdio.h>
#include <string.h>

#define SIGNATURE "LUDO_SAVE_V1"

int sauvegarder_partie(const Partie *p, const char *chemin)
{
    FILE *f = fopen(chemin, "w");
    int j, k, tete;
    if (f == NULL) return SAUV_ERREUR_FICHIER;
    tete = file_vide(&p->file) ? 0 : file_tete(&p->file);
    fprintf(f, "%s\n%d %d %d %d %d\n", SIGNATURE, p->nb_joueurs, (int)p->mode,
            p->nb_tours, p->nb_classes, tete);
    for (j = 0; j < p->nb_joueurs; j++) {
        const Joueur *pl = &p->joueurs[j];
        fprintf(f, "%s %d %d %d", pl->nom, (int)pl->type, pl->siege, pl->rang);
        for (k = 0; k < NB_PIONS; k++) fprintf(f, " %d", pl->pions[k]);
        fputc('\n', f);
    }
    if (ferror(f)) { fclose(f); return SAUV_ERREUR_FICHIER; }
    return fclose(f) == 0 ? SAUV_OK : SAUV_ERREUR_FICHIER;
}

int charger_partie(Partie *p, const char *chemin)
{
    FILE *f = fopen(chemin, "r");
    Partie t;
    char sig[32];
    int j, k, mode, tete, classes_vus = 0, siege_pris[MAX_JOUEURS] = {0};
    int rang_pris[MAX_JOUEURS + 1] = {0};

    if (f == NULL) return SAUV_ERREUR_FICHIER;
    memset(&t, 0, sizeof t);

    if (fscanf(f, "%31s", sig) != 1 || strcmp(sig, SIGNATURE) != 0) goto erreur;
    if (fscanf(f, "%d %d %d %d %d", &t.nb_joueurs, &mode, &t.nb_tours,
               &t.nb_classes, &tete) != 5) goto erreur;
    if (t.nb_joueurs < 2 || t.nb_joueurs > MAX_JOUEURS) goto erreur;
    if (mode != FIN_PREMIER_VAINQUEUR && mode != FIN_CLASSEMENT_COMPLET) goto erreur;
    if (tete < 0 || tete >= t.nb_joueurs || t.nb_tours < 0) goto erreur;
    t.mode = (ModeFin)mode;

    for (j = 0; j < t.nb_joueurs; j++) {
        Joueur *pl = &t.joueurs[j];
        int type;
        if (fscanf(f, "%20s %d %d %d", pl->nom, &type, &pl->siege, &pl->rang) != 4) goto erreur;
        if (type != HUMAIN && type != ORDINATEUR) goto erreur;
        pl->type = (TypeJoueur)type;
        if (pl->siege < 0 || pl->siege >= MAX_JOUEURS || siege_pris[pl->siege]) goto erreur;
        siege_pris[pl->siege] = 1;
        if (pl->rang < 0 || pl->rang > t.nb_joueurs) goto erreur;
        if (pl->rang > 0) {
            if (rang_pris[pl->rang]) goto erreur;
            rang_pris[pl->rang] = 1;
            classes_vus++;
        }
        for (k = 0; k < NB_PIONS; k++) {
            if (fscanf(f, "%d", &pl->pions[k]) != 1) goto erreur;
            if (pl->pions[k] < POS_BASE || pl->pions[k] > POS_ARRIVEE) goto erreur;
        }
    }
    if (classes_vus != t.nb_classes) goto erreur;

    fclose(f);
    file_reconstruire(&t, tete);
    *p = t;
    return SAUV_OK;

erreur:
    fclose(f);
    return SAUV_ERREUR_FORMAT;
}
