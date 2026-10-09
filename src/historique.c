#include "historique.h"
#include <stdlib.h>

void historique_init(Historique *h)
{
    h->sommet = NULL;
    h->taille = 0;
}

int historique_empiler(Historique *h, const Evenement *ev)
{
    Evenement *n = malloc(sizeof *n);
    if (n == NULL) return -1;
    *n = *ev;
    n->suivant = h->sommet;
    h->sommet = n;
    h->taille++;
    return 0;
}

int historique_depiler(Historique *h, Evenement *out)
{
    Evenement *n = h->sommet;
    if (n == NULL) return -1;
    if (out != NULL) {
        *out = *n;
        out->suivant = NULL;
    }
    h->sommet = n->suivant;
    h->taille--;
    free(n);
    return 0;
}

void historique_liberer(Historique *h)
{
    while (historique_depiler(h, NULL) == 0) { }
}
