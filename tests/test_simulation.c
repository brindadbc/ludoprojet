/*
 * Simulation : on fait jouer des milliers de parties entre ordinateurs et on
 * verifie les invariants du jeu apres chaque tour.
 */
#include "affichage.h"
#include "file.h"
#include "historique.h"
#include "jeu.h"
#include "joueur.h"
#include "regles.h"
#include "utils.h"
#include <stdio.h>
#include <string.h>

#define TOURS_MAX 200000

/* Retourne NULL si tout va bien, sinon un message d'erreur. */
static const char *verifier(const Partie *p)
{
    int owner[LONGUEUR_PISTE], j, k, a;
    for (a = 0; a < LONGUEUR_PISTE; a++) owner[a] = -1;
    for (j = 0; j < p->nb_joueurs; j++)
        for (k = 0; k < NB_PIONS; k++) {
            int pos = p->joueurs[j].pions[k];
            if (pos < POS_BASE || pos > POS_ARRIVEE) return "position hors limites";
            a = regles_case_absolue(p->joueurs[j].siege, pos);
            if (a >= 0 && !regles_case_sure(a)) {
                if (owner[a] != -1 && owner[a] != j)
                    return "deux joueurs sur une case non sure";
                owner[a] = j;
            }
        }
    return NULL;
}

static int simuler(unsigned int graine, int nb, ModeFin mode)
{
    static const int SIEGES_2[2] = { 0, 2 };
    Partie p; Historique h; int i, tours = 0, rangs[MAX_JOUEURS + 1] = {0};
    const char *err;

    memset(&p, 0, sizeof p);
    p.nb_joueurs = nb;
    p.mode = mode;
    for (i = 0; i < nb; i++)
        joueur_init(&p.joueurs[i], "IA", ORDINATEUR, nb == 2 ? SIEGES_2[i] : i);
    regles_init_partie(&p);
    historique_init(&h);
    utils_init_alea(graine);

    while (!regles_partie_terminee(&p)) {
        if (++tours > TOURS_MAX) { printf("  boucle infinie (graine %u)\n", graine); return 1; }
        if (jeu_jouer_tour(&p, &h) != TOUR_OK) return 1;
        if ((err = verifier(&p)) != NULL) {
            printf("  invariant viole (%s), graine %u, %d joueurs\n", err, graine, nb);
            return 1;
        }
    }
    regles_finaliser_classement(&p);
    if (mode == FIN_CLASSEMENT_COMPLET) {
        for (i = 0; i < nb; i++) {
            int r = p.joueurs[i].rang;
            if (r < 1 || r > nb || rangs[r]) { printf("  classement incoherent (graine %u)\n", graine); return 1; }
            rangs[r] = 1;
        }
    } else if (p.nb_classes != 1) {
        printf("  premier vainqueur incoherent (graine %u)\n", graine);
        return 1;
    }
    historique_liberer(&h);
    return 0;
}

int main(void)
{
    unsigned int g;
    int nb, mode, parties = 0, echecs = 0;
    affichage_silencieux(1);
    for (nb = 2; nb <= 4; nb++)
        for (mode = 0; mode <= 1; mode++)
            for (g = 1; g <= 300; g++) {
                echecs += simuler(g, nb, (ModeFin)mode);
                parties++;
            }
    printf("%d parties simulees, %d echec(s)\n", parties, echecs);
    return echecs == 0 ? 0 : 1;
}
