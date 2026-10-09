#ifndef UTILS_H
#define UTILS_H
#include <stddef.h>

void         utils_init_alea(unsigned int graine);
unsigned int utils_graine_horloge(void);
int          utils_lancer_de(void);                    /* 1..6 */
void         utils_trim(char *s);
int          utils_lire_ligne(char *buf, size_t taille); /* 0 si fin de fichier */
int          utils_lire_entier(const char *invite, int min, int max); /* -1 si EOF */
int          utils_est_oui(const char *s);

#endif
