#include "joueur.h"
#include "regles.h"
#include <string.h>

void joueur_init(Joueur *pl, const char *nom, TypeJoueur type, int siege)
{
    int k;
    strncpy(pl->nom, nom, MAX_NOM - 1);
    pl->nom[MAX_NOM - 1] = '\0';
    pl->type = type;
    pl->siege = siege;
    pl->rang = 0;
    for (k = 0; k < NB_PIONS; k++) pl->pions[k] = POS_BASE;
}

/* Nombre d'adversaires pouvant atteindre la case `absolue` avec un de 1..6. */
static int menace(const Partie *p, int j, int absolue)
{
    int i, k, n = 0;
    for (i = 0; i < p->nb_joueurs; i++) {
        if (i == j) continue;
        for (k = 0; k < NB_PIONS; k++) {
            int pos = p->joueurs[i].pions[k];
            int a = regles_case_absolue(p->joueurs[i].siege, pos);
            int dist;
            if (a < 0) continue;
            dist = (absolue - a + LONGUEUR_PISTE) % LONGUEUR_PISTE;
            if (dist >= 1 && dist <= 6 && pos + dist <= POS_FIN_PISTE) n++;
        }
    }
    return n;
}

static int evaluer(const Partie *p, int j, const Coup *c)
{
    int siege = p->joueurs[j].siege;
    int s = 0, a_arr, a_dep;

    if (c->arrive_maison) s += 100;
    if (c->capture)       s += 80;
    if (c->sortie_base)   s += 50;
    if (c->depart <= POS_FIN_PISTE && c->arrivee >= POS_DEBUT_COULOIR) s += 40;

    a_arr = regles_case_absolue(siege, c->arrivee);
    if (a_arr >= 0) {
        if (regles_case_sure(a_arr)) s += 20;
        else                         s -= 15 * menace(p, j, a_arr);
    }
    a_dep = regles_case_absolue(siege, c->depart);
    if (a_dep >= 0 && !regles_case_sure(a_dep))
        s += 12 * menace(p, j, a_dep);          /* fuir une case dangereuse */

    s += c->arrivee / 4;                        /* privilegier l'avancement */
    return s;
}

int joueur_choisir_coup_ia(const Partie *p, int j, const Coup *coups, int n)
{
    int i, meilleur = 0, best = evaluer(p, j, &coups[0]);
    for (i = 1; i < n; i++) {
        int s = evaluer(p, j, &coups[i]);
        if (s > best) { best = s; meilleur = i; }
    }
    return meilleur;
}
