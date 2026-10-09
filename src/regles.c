#include "regles.h"
#include "file.h"

int regles_case_absolue(int siege, int pos)
{
    if (pos < POS_DEPART || pos > POS_FIN_PISTE) return -1;
    return (siege * ECART_DEPART + pos) % LONGUEUR_PISTE;
}

/* Cases sures : les 4 cases de depart (0,13,26,39) et les 4 etoiles (8,21,34,47). */
int regles_case_sure(int absolue)
{
    int m;
    if (absolue < 0) return 0;
    m = absolue % ECART_DEPART;
    return m == 0 || m == 8;
}

void regles_init_partie(Partie *p)
{
    int j, k;
    for (j = 0; j < p->nb_joueurs; j++) {
        for (k = 0; k < NB_PIONS; k++) p->joueurs[j].pions[k] = POS_BASE;
        p->joueurs[j].rang = 0;
    }
    p->nb_classes = 0;
    p->nb_tours = 0;
    file_reconstruire(p, 0);
}

/* Nombre de pions de chaque adversaire (par rapport a j) sur une case de piste. */
static void adversaires_sur_case(const Partie *p, int j, int absolue,
                                 int par_joueur[MAX_JOUEURS])
{
    int i, k;
    for (i = 0; i < MAX_JOUEURS; i++) par_joueur[i] = 0;
    for (i = 0; i < p->nb_joueurs; i++) {
        if (i == j) continue;
        for (k = 0; k < NB_PIONS; k++)
            if (regles_case_absolue(p->joueurs[i].siege, p->joueurs[i].pions[k]) == absolue)
                par_joueur[i]++;
    }
}

static int premier_pion_sur_case(const Joueur *pl, int absolue)
{
    int k;
    for (k = 0; k < NB_PIONS; k++)
        if (regles_case_absolue(pl->siege, pl->pions[k]) == absolue) return k;
    return -1;
}

/*
 * Calcule les coups legaux du joueur j pour un de donne.
 *  - sortie de base uniquement avec un 6 ;
 *  - arrivee a la maison avec un nombre exact ;
 *  - une case non sure occupee par 2 pions (ou plus) d'un meme adversaire est
 *    une "tour" : on ne peut pas s'y poser ;
 *  - un pion seul sur une case non sure est capture (renvoye en base) ;
 *  - les coups equivalents (pions de meme position) ne sont listes qu'une fois.
 */
int regles_coups_possibles(const Partie *p, int j, int de, Coup coups[NB_PIONS])
{
    const Joueur *pl = &p->joueurs[j];
    int n = 0, k, i;

    for (k = 0; k < NB_PIONS; k++) {
        int  pos = pl->pions[k];
        int  dest, sortie = 0, absl, doublon = 0;
        Coup c;

        if (pos == POS_ARRIVEE) continue;
        for (i = 0; i < k; i++)
            if (pl->pions[i] == pos) doublon = 1;
        if (doublon) continue;

        if (pos == POS_BASE) {
            if (de != VALEUR_SORTIE) continue;
            dest = POS_DEPART;
            sortie = 1;
        } else {
            dest = pos + de;
            if (dest > POS_ARRIVEE) continue;
        }

        c.pion = k;
        c.depart = pos;
        c.arrivee = dest;
        c.sortie_base = sortie;
        c.capture = 0;
        c.victime_joueur = -1;
        c.victime_pion = -1;
        c.arrive_maison = (dest == POS_ARRIVEE);

        absl = regles_case_absolue(pl->siege, dest);
        if (absl >= 0 && !regles_case_sure(absl)) {
            int cnt[MAX_JOUEURS], bloque = 0;
            adversaires_sur_case(p, j, absl, cnt);
            for (i = 0; i < p->nb_joueurs; i++)
                if (cnt[i] >= 2) bloque = 1;
            if (bloque) continue;
            for (i = 0; i < p->nb_joueurs; i++) {
                if (cnt[i] == 1) {
                    c.capture = 1;
                    c.victime_joueur = i;
                    c.victime_pion = premier_pion_sur_case(&p->joueurs[i], absl);
                }
            }
        }
        coups[n++] = c;
    }
    return n;
}

int regles_a_gagne(const Joueur *pl)
{
    int k;
    for (k = 0; k < NB_PIONS; k++)
        if (pl->pions[k] != POS_ARRIVEE) return 0;
    return 1;
}

void regles_appliquer_coup(Partie *p, int j, const Coup *c)
{
    p->joueurs[j].pions[c->pion] = c->arrivee;
    if (c->capture)
        p->joueurs[c->victime_joueur].pions[c->victime_pion] = POS_BASE;
    if (regles_a_gagne(&p->joueurs[j]) && p->joueurs[j].rang == 0) {
        p->nb_classes++;
        p->joueurs[j].rang = p->nb_classes;
    }
}

int regles_partie_terminee(const Partie *p)
{
    if (p->mode == FIN_PREMIER_VAINQUEUR) return p->nb_classes >= 1;
    return p->nb_classes >= p->nb_joueurs - 1;
}

void regles_finaliser_classement(Partie *p)
{
    int j;
    if (p->mode == FIN_CLASSEMENT_COMPLET && p->nb_classes == p->nb_joueurs - 1)
        for (j = 0; j < p->nb_joueurs; j++)
            if (p->joueurs[j].rang == 0) p->joueurs[j].rang = ++p->nb_classes;
}

int regles_score_avancement(const Joueur *pl)
{
    int k, s = 0;
    for (k = 0; k < NB_PIONS; k++) s += pl->pions[k] + 1;
    return s;
}

/* Vrai si le joueur a doit etre classe avant le joueur b. */
static int classe_avant(const Partie *p, int a, int b)
{
    int ra = p->joueurs[a].rang, rb = p->joueurs[b].rang;
    if (ra > 0 && rb > 0) return ra < rb;
    if (ra > 0) return 1;
    if (rb > 0) return 0;
    return regles_score_avancement(&p->joueurs[a]) > regles_score_avancement(&p->joueurs[b]);
}

void regles_classement(const Partie *p, int ordre[MAX_JOUEURS])
{
    int i, j;
    for (i = 0; i < p->nb_joueurs; i++) ordre[i] = i;
    for (i = 1; i < p->nb_joueurs; i++) {       /* tri par insertion (stable) */
        int v = ordre[i];
        j = i - 1;
        while (j >= 0 && classe_avant(p, v, ordre[j])) {
            ordre[j + 1] = ordre[j];
            j--;
        }
        ordre[j + 1] = v;
    }
}

void regles_compter(const Joueur *pl, int *base, int *piste, int *couloir, int *arrives)
{
    int k;
    *base = *piste = *couloir = *arrives = 0;
    for (k = 0; k < NB_PIONS; k++) {
        int pos = pl->pions[k];
        if (pos == POS_BASE) (*base)++;
        else if (pos <= POS_FIN_PISTE) (*piste)++;
        else if (pos < POS_ARRIVEE) (*couloir)++;
        else (*arrives)++;
    }
}
