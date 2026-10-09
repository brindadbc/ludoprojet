#include "utils.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

void utils_init_alea(unsigned int graine) { srand(graine); }

unsigned int utils_graine_horloge(void) { return (unsigned int)time(NULL); }

int utils_lancer_de(void)
{
    return (int)(rand() / (RAND_MAX + 1.0) * 6.0) + 1;
}

void utils_trim(char *s)
{
    size_t debut = 0, fin = strlen(s);
    while (s[debut] != '\0' && isspace((unsigned char)s[debut])) debut++;
    while (fin > debut && isspace((unsigned char)s[fin - 1])) fin--;
    if (debut > 0) memmove(s, s + debut, fin - debut);
    s[fin - debut] = '\0';
}

int utils_lire_ligne(char *buf, size_t taille)
{
    size_t n;
    if (fgets(buf, (int)taille, stdin) == NULL) {
        buf[0] = '\0';
        return 0;
    }
    n = strlen(buf);
    if (n > 0 && buf[n - 1] == '\n') {
        buf[--n] = '\0';
    } else {                        /* ligne trop longue : on jette le reste */
        int c;
        while ((c = getchar()) != '\n' && c != EOF) { }
    }
    utils_trim(buf);
    return 1;
}

int utils_lire_entier(const char *invite, int min, int max)
{
    char buf[64];
    for (;;) {
        char *fin;
        long  v;
        printf("%s", invite);
        fflush(stdout);
        if (!utils_lire_ligne(buf, sizeof buf)) return -1;
        v = strtol(buf, &fin, 10);
        if (buf[0] != '\0' && *fin == '\0' && v >= min && v <= max) return (int)v;
        printf("  Entrée invalide : saisissez un nombre entre %d et %d.\n", min, max);
    }
}

int utils_est_oui(const char *s)
{
    return s[0] != '\0' && (tolower((unsigned char)s[0]) == 'o' ||
                            tolower((unsigned char)s[0]) == 'y');
}
