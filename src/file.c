#include "file.h"

void file_init(FileTours *f)
{
    f->tete = 0;
    f->taille = 0;
}

int file_vide(const FileTours *f) { return f->taille == 0; }

int file_enfiler(FileTours *f, int joueur)
{
    if (f->taille >= MAX_JOUEURS) return -1;
    f->elements[(f->tete + f->taille) % MAX_JOUEURS] = joueur;
    f->taille++;
    return 0;
}

int file_defiler(FileTours *f)
{
    int v;
    if (f->taille == 0) return -1;
    v = f->elements[f->tete];
    f->tete = (f->tete + 1) % MAX_JOUEURS;
    f->taille--;
    return v;
}

int file_tete(const FileTours *f)
{
    return f->taille == 0 ? -1 : f->elements[f->tete];
}

/* Reconstruit la file a partir de l'etat de la partie : on part de `premier`
 * et on parcourt les joueurs de facon circulaire, en sautant les joueurs
 * deja classes. */
void file_reconstruire(Partie *p, int premier)
{
    int i;
    file_init(&p->file);
    for (i = 0; i < p->nb_joueurs; i++) {
        int j = (premier + i) % p->nb_joueurs;
        if (p->joueurs[j].rang == 0) file_enfiler(&p->file, j);
    }
}
