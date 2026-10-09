#include "plateau.h"
#include "types.h"

/* Les 52 cases de la piste commune, dans le sens horaire, a partir de la case
 * de depart du Rouge (ligne 6, colonne 1). {ligne, colonne}. */
static const int PISTE[LONGUEUR_PISTE][2] = {
    {6,1},{6,2},{6,3},{6,4},{6,5},
    {5,6},{4,6},{3,6},{2,6},{1,6},{0,6},
    {0,7},{0,8},
    {1,8},{2,8},{3,8},{4,8},{5,8},
    {6,9},{6,10},{6,11},{6,12},{6,13},{6,14},
    {7,14},{8,14},
    {8,13},{8,12},{8,11},{8,10},{8,9},
    {9,8},{10,8},{11,8},{12,8},{13,8},{14,8},
    {14,7},{14,6},
    {13,6},{12,6},{11,6},{10,6},{9,6},
    {8,5},{8,4},{8,3},{8,2},{8,1},{8,0},
    {7,0},{6,0}
};

/* Emplacements des 4 pions dans chaque base (Rouge, Vert, Jaune, Bleu). */
static const int BASES[4][4][2] = {
    {{2,2},{2,3},{3,2},{3,3}},
    {{2,11},{2,12},{3,11},{3,12}},
    {{11,11},{11,12},{12,11},{12,12}},
    {{11,2},{11,3},{12,2},{12,3}}
};

static CaseInfo grille[TAILLE_GRILLE][TAILLE_GRILLE];
static int      initialise = 0;

static void couloir_coord(int siege, int k, int *l, int *c)
{
    switch (siege) {
    case 0:  *l = 7;      *c = 1 + k;  break;
    case 1:  *l = 1 + k;  *c = 7;      break;
    case 2:  *l = 7;      *c = 13 - k; break;
    default: *l = 13 - k; *c = 7;      break;
    }
}

static void init_grille(void)
{
    int l, c, i, s, k;
    for (l = 0; l < TAILLE_GRILLE; l++)
        for (c = 0; c < TAILLE_GRILLE; c++) {
            CaseInfo *ci = &grille[l][c];
            ci->type = C_VIDE; ci->siege = -1; ci->absolue = -1;
            ci->depart_de = -1; ci->sure = 0; ci->slot = -1;
            if (l <= 5 && c <= 5)        { ci->type = C_BASE; ci->siege = 0; }
            else if (l <= 5 && c >= 9)   { ci->type = C_BASE; ci->siege = 1; }
            else if (l >= 9 && c >= 9)   { ci->type = C_BASE; ci->siege = 2; }
            else if (l >= 9 && c <= 5)   { ci->type = C_BASE; ci->siege = 3; }
            else if (l >= 6 && l <= 8 && c >= 6 && c <= 8) ci->type = C_CENTRE;
        }
    grille[7][7].slot = 0;

    for (s = 0; s < 4; s++)
        for (k = 0; k < 4; k++)
            grille[BASES[s][k][0]][BASES[s][k][1]].slot = k;

    for (i = 0; i < LONGUEUR_PISTE; i++) {
        CaseInfo *ci = &grille[PISTE[i][0]][PISTE[i][1]];
        ci->type = C_PISTE;
        ci->absolue = i;
        ci->sure = (i % ECART_DEPART == 0) || (i % ECART_DEPART == 8);
        if (i % ECART_DEPART == 0) ci->depart_de = i / ECART_DEPART;
    }
    for (s = 0; s < 4; s++)
        for (k = 0; k < 5; k++) {
            couloir_coord(s, k, &l, &c);
            grille[l][c].type = C_COULOIR;
            grille[l][c].siege = s;
        }
    initialise = 1;
}

const CaseInfo *plateau_case(int ligne, int colonne)
{
    if (!initialise) init_grille();
    return &grille[ligne][colonne];
}

void plateau_coordonnees(int siege, int pos, int pion, int *ligne, int *colonne)
{
    if (pos == POS_BASE) {
        *ligne = BASES[siege][pion][0];
        *colonne = BASES[siege][pion][1];
    } else if (pos <= POS_FIN_PISTE) {
        int a = (siege * ECART_DEPART + pos) % LONGUEUR_PISTE;
        *ligne = PISTE[a][0];
        *colonne = PISTE[a][1];
    } else if (pos < POS_ARRIVEE) {
        couloir_coord(siege, pos - POS_DEBUT_COULOIR, ligne, colonne);
    } else {
        *ligne = 7;
        *colonne = 7;
    }
}
