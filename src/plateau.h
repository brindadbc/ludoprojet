/* plateau.h - Geometrie du plateau : grille 15x15 et chemin des pions. */
#ifndef PLATEAU_H
#define PLATEAU_H

#define TAILLE_GRILLE 15

typedef enum { C_VIDE, C_PISTE, C_COULOIR, C_BASE, C_CENTRE } TypeCase;

typedef struct {
    TypeCase type;
    int      siege;      /* proprietaire (base / couloir), sinon -1 */
    int      absolue;    /* index 0..51 sur la piste, sinon -1 */
    int      depart_de;  /* siege dont c'est la case de depart, sinon -1 */
    int      sure;       /* case sure (depart ou etoile) */
    int      slot;       /* emplacement de pion en base (0..3) ; centre : 0 = case principale */
} CaseInfo;

const CaseInfo *plateau_case(int ligne, int colonne);
void plateau_coordonnees(int siege, int pos, int pion, int *ligne, int *colonne);

#endif
