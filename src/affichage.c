#include "affichage.h"
#include "file.h"
#include "plateau.h"
#include "regles.h"
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#endif

static int g_couleur    = 1;
static int g_silencieux = 0;

static const char *NOMS[4]    = { "Rouge", "Vert", "Jaune", "Bleu" };
static const char  LETTRES[4] = { 'R', 'V', 'J', 'B' };
static const int   FG[4]      = { 31, 32, 33, 34 };
static const int   BG[4]      = { 41, 42, 43, 44 };

/* ------------------------------------------------------------------ */
/* Initialisation et messages                                          */
/* ------------------------------------------------------------------ */

void affichage_init(int couleur)
{
    g_couleur = couleur;
    if (getenv("NO_COLOR") != NULL) g_couleur = 0;
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    {
        HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
        DWORD  mode = 0;
        if (h == INVALID_HANDLE_VALUE || !GetConsoleMode(h, &mode) ||
            !SetConsoleMode(h, mode | 0x0004))
            g_couleur = 0;
    }
#endif
}

void affichage_silencieux(int actif) { g_silencieux = actif; }

void affichage_message(const char *fmt, ...)
{
    va_list ap;
    if (g_silencieux) return;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
}

const char *affichage_nom_couleur(int siege) { return NOMS[siege]; }
char        affichage_lettre(int siege)      { return LETTRES[siege]; }

static void c_fg(int siege)   { if (g_couleur) printf("\033[1;%dm", FG[siege]); }
static void c_base(int siege) { if (g_couleur) printf("\033[%d;%dm", siege == 2 ? 30 : 37, BG[siege]); }
static void c_fin(void)       { if (g_couleur) printf("\033[0m"); }

/* ------------------------------------------------------------------ */
/* Descriptions textuelles                                             */
/* ------------------------------------------------------------------ */

void affichage_decrire_position(int pos, char *buf, size_t n)
{
    if (pos == POS_BASE)             snprintf(buf, n, "base");
    else if (pos <= POS_FIN_PISTE)   snprintf(buf, n, "case %d", pos + 1);
    else if (pos < POS_ARRIVEE)      snprintf(buf, n, "couloir %d/5", pos - POS_FIN_PISTE);
    else                             snprintf(buf, n, "arrivée");
}

void affichage_decrire_coup(const Partie *p, const Coup *c, char *buf, size_t n)
{
    char a[32], b[32], extra[96] = "";
    affichage_decrire_position(c->depart, a, sizeof a);
    affichage_decrire_position(c->arrivee, b, sizeof b);
    if (c->capture)
        snprintf(extra, sizeof extra, "  [capture le pion %d de %s !]",
                 c->victime_pion + 1, p->joueurs[c->victime_joueur].nom);
    else if (c->arrive_maison)
        snprintf(extra, sizeof extra, "  [arrivée à la maison !]");
    else if (c->sortie_base)
        snprintf(extra, sizeof extra, "  [sortie de la base]");
    snprintf(buf, n, "pion %d : %s -> %s%s", c->pion + 1, a, b, extra);
}

void affichage_decrire_evenement(const Partie *p, const Evenement *ev,
                                 char *buf, size_t n, int avec_tour)
{
    char pre[32] = "", a[32], b[32];
    const char *nom = p->joueurs[ev->joueur].nom;
    if (avec_tour) snprintf(pre, sizeof pre, "Tour %3d - ", ev->tour);
    switch (ev->type) {
    case EV_SORTIE:
        snprintf(buf, n, "%s%s (dé %d) : le pion %d sort de la base",
                 pre, nom, ev->de, ev->pion + 1);
        break;
    case EV_DEPLACEMENT:
        affichage_decrire_position(ev->depart, a, sizeof a);
        affichage_decrire_position(ev->arrivee, b, sizeof b);
        if (ev->victime_joueur >= 0)
            snprintf(buf, n, "%s%s (dé %d) : pion %d, %s -> %s, capture le pion %d de %s",
                     pre, nom, ev->de, ev->pion + 1, a, b, ev->victime_pion + 1,
                     p->joueurs[ev->victime_joueur].nom);
        else
            snprintf(buf, n, "%s%s (dé %d) : pion %d, %s -> %s",
                     pre, nom, ev->de, ev->pion + 1, a, b);
        break;
    case EV_PASSE:
        snprintf(buf, n, "%s%s (dé %d) : aucun déplacement possible", pre, nom, ev->de);
        break;
    case EV_TROIS_SIX:
        snprintf(buf, n, "%s%s : trois 6 de suite, le tour est annulé", pre, nom);
        break;
    case EV_VICTOIRE:
        snprintf(buf, n, "%s%s a amené ses 4 pions à l'arrivée (rang %d) !",
                 pre, nom, ev->rang);
        break;
    default:
        snprintf(buf, n, "?");
    }
}

/* ------------------------------------------------------------------ */
/* Plateau                                                             */
/* ------------------------------------------------------------------ */

static void etiquette(char t[4], int n, int prop, int idx, int mixte)
{
    if (n == 1)       { t[0] = LETTRES[prop]; t[1] = (char)('0' + idx); t[2] = ' '; }
    else if (mixte)   { t[0] = '*'; t[1] = (char)('0' + n); t[2] = '*'; }
    else              { t[0] = LETTRES[prop]; t[1] = 'x'; t[2] = (char)('0' + n); }
    t[3] = '\0';
}

static void dessiner_case(const CaseInfo *ci, int n, int prop, int idx, int mixte,
                          const int actif[MAX_JOUEURS])
{
    char t[4] = "   ";
    int  style = 0, siege = -1;   /* 0 normal, 1 couleur de texte, 2 fond de base */

    switch (ci->type) {
    case C_BASE:
        if (!actif[ci->siege]) break;
        style = 2; siege = ci->siege;
        if (ci->slot >= 0) {
            if (n > 0) etiquette(t, n, prop, idx, mixte);
            else       strcpy(t, " o ");
        }
        break;
    case C_PISTE:
        if (n > 0) { etiquette(t, n, prop, idx, mixte); style = 1; siege = prop; }
        else if (ci->depart_de >= 0) {
            t[1] = (char)tolower((unsigned char)LETTRES[ci->depart_de]);
            style = 1; siege = ci->depart_de;
        }
        else strcpy(t, ci->sure ? " * " : " . ");
        break;
    case C_COULOIR:
        if (n > 0) etiquette(t, n, prop, idx, mixte);
        else       strcpy(t, " : ");
        style = 1; siege = (n > 0) ? prop : ci->siege;
        break;
    case C_CENTRE:
        if (ci->slot == 0) strcpy(t, " # ");
        break;
    default:
        break;
    }

    if (style == 1)      c_fg(siege);
    else if (style == 2) c_base(siege);
    fputs(t, stdout);
    if (style != 0)      c_fin();
}

void affichage_plateau(const Partie *p)
{
    int nb[TAILLE_GRILLE][TAILLE_GRILLE];
    int prop[TAILLE_GRILLE][TAILLE_GRILLE];
    int idx[TAILLE_GRILLE][TAILLE_GRILLE];
    int mixte[TAILLE_GRILLE][TAILLE_GRILLE];
    int actif[MAX_JOUEURS] = {0, 0, 0, 0};
    int l, c, j, k;

    for (l = 0; l < TAILLE_GRILLE; l++)
        for (c = 0; c < TAILLE_GRILLE; c++) {
            nb[l][c] = 0; prop[l][c] = -1; idx[l][c] = 0; mixte[l][c] = 0;
        }

    for (j = 0; j < p->nb_joueurs; j++) {
        int siege = p->joueurs[j].siege;
        actif[siege] = 1;
        for (k = 0; k < NB_PIONS; k++) {
            int pos = p->joueurs[j].pions[k];
            if (pos == POS_ARRIVEE) continue;
            plateau_coordonnees(siege, pos, k, &l, &c);
            if (nb[l][c] == 0) { prop[l][c] = siege; idx[l][c] = k + 1; }
            else if (prop[l][c] != siege) mixte[l][c] = 1;
            nb[l][c]++;
        }
    }

    putchar('\n');
    for (l = 0; l < TAILLE_GRILLE; l++) {
        fputs("  ", stdout);
        for (c = 0; c < TAILLE_GRILLE; c++)
            dessiner_case(plateau_case(l, c), nb[l][c], prop[l][c], idx[l][c],
                          mixte[l][c], actif);
        putchar('\n');
    }
    printf("\n  Légende : R1 = pion 1 du Rouge | Rx2 = 2 pions du même joueur (tour)\n"
           "            *2* = pions de joueurs différents sur une case sûre\n"
           "            r,v,j,b = case de départ | * = case sûre | : = couloir | # = maison\n");
}

void affichage_etat_joueurs(const Partie *p)
{
    int j, courant = file_vide(&p->file) ? -1 : file_tete(&p->file);
    printf("\n  %-26s %5s %6s %8s %8s\n", "Joueur", "Base", "Piste", "Couloir", "Arrivés");
    for (j = 0; j < p->nb_joueurs; j++) {
        const Joueur *pl = &p->joueurs[j];
        int b, pi, co, ar;
        char desc[64];
        regles_compter(pl, &b, &pi, &co, &ar);
        snprintf(desc, sizeof desc, "%c %s (%s)", LETTRES[pl->siege], pl->nom,
                 pl->type == HUMAIN ? "humain" : "ordi");
        printf("%s ", j == courant ? ">" : " ");
        c_fg(pl->siege);
        printf("%-26s", desc);
        c_fin();
        printf(" %5d %6d %8d %8d", b, pi, co, ar);
        if (pl->rang > 0) printf("   (rang %d)", pl->rang);
        putchar('\n');
    }
}

/* ------------------------------------------------------------------ */
/* De, coups, historique, classement                                   */
/* ------------------------------------------------------------------ */

void affichage_de(int v)
{
    static const char *F[7][3] = {
        { "", "", "" },
        { "       ", "   o   ", "       " },
        { " o     ", "       ", "     o " },
        { " o     ", "   o   ", "     o " },
        { " o   o ", "       ", " o   o " },
        { " o   o ", "   o   ", " o   o " },
        { " o   o ", " o   o ", " o   o " }
    };
    if (g_silencieux) return;
    printf("    +-------+\n    |%s|\n    |%s|\n    |%s|\n    +-------+\n",
           F[v][0], F[v][1], F[v][2]);
}

void affichage_coups(const Partie *p, const Coup *coups, int n, int de)
{
    int i;
    char buf[160];
    printf("  Dé : %d. Coups possibles :\n", de);
    for (i = 0; i < n; i++) {
        affichage_decrire_coup(p, &coups[i], buf, sizeof buf);
        printf("    [%d] %s\n", coups[i].pion + 1, buf);
    }
}

void affichage_historique(const Partie *p, const Historique *h, int max)
{
    const Evenement *ev = h->sommet;
    int i = 0;
    char buf[200];
    if (ev == NULL) {
        printf("  (historique vide)\n");
        return;
    }
    printf("  Derniers événements (du plus récent au plus ancien) :\n");
    for (; ev != NULL && i < max; ev = ev->suivant, i++) {
        affichage_decrire_evenement(p, ev, buf, sizeof buf, 1);
        printf("    %s\n", buf);
    }
}

void affichage_classement(const Partie *p)
{
    int ordre[MAX_JOUEURS], i;
    regles_classement(p, ordre);
    printf("\n  ===== CLASSEMENT FINAL =====\n");
    for (i = 0; i < p->nb_joueurs; i++) {
        const Joueur *pl = &p->joueurs[ordre[i]];
        printf("   %d. ", i + 1);
        c_fg(pl->siege);
        printf("%s (%s)", pl->nom, NOMS[pl->siege]);
        c_fin();
        if (pl->rang == 0) printf("  - avancement : %d/%d", regles_score_avancement(pl),
                                  NB_PIONS * (POS_ARRIVEE + 1));
        putchar('\n');
    }
    printf("  Partie terminée en %d tours.\n", p->nb_tours);
}

/* ------------------------------------------------------------------ */
/* Pages de texte                                                      */
/* ------------------------------------------------------------------ */

void affichage_titre(void)
{
    printf("\n");
    printf("  +=====================================+\n");
    printf("  |          L U D O   en   C           |\n");
    printf("  |     Projet Algo et Prog - Groupe 1  |\n");
    printf("  +=====================================+\n");
}

void affichage_regles(void)
{
    printf(
    "\n  ===== RÈGLES DU JEU =====\n"
    "  But : amener ses 4 pions de la base jusqu'à la maison (#).\n\n"
    "  - À son tour, on lance le dé (1 à 6) et on déplace un pion.\n"
    "  - Il faut un 6 pour sortir un pion de la base.\n"
    "  - Un 6, une capture ou l'arrivée d'un pion à la maison permet de rejouer.\n"
    "  - Trois 6 de suite : le tour est annulé.\n"
    "  - Un pion qui s'arrête sur un pion adverse seul le renvoie à sa base,\n"
    "    sauf sur une case sûre (cases de départ et étoiles *).\n"
    "  - Deux pions du même joueur sur une même case forment une tour :\n"
    "    elle ne peut pas être capturée et on ne peut pas s'y arrêter.\n"
    "  - Après un tour complet de piste, on entre dans son couloir (:) où\n"
    "    les pions sont protégés. Il faut un nombre exact pour atteindre la maison.\n"
    "  - Fin de partie : au premier vainqueur, ou classement complet (au choix).\n");
}

void affichage_aide(void)
{
    printf(
    "\n  Commandes disponibles pendant la partie :\n"
    "    Entrée  lancer le dé\n"
    "    1-4     choisir le pion à jouer\n"
    "    p       réafficher le plateau\n"
    "    h       afficher l'historique des coups\n"
    "    s       sauvegarder la partie\n"
    "    r       rappel des règles\n"
    "    ?       cette aide\n"
    "    q       quitter la partie\n");
}

/* A MODIFIER : noms reels des membres du Groupe 1. */
void affichage_equipe(void)
{
    printf(
    "\n  ===== GROUPE 1 - ISESTMA 2026-2027 =====\n"
    "  Cours : Algo et Prog - Daniel MOUNE\n"
    "    - [Nom du chef d'équipe]  (chef d'équipe)\n"
    "    - [Membre 2]\n"
    "    - [Membre 3]\n"
    "    - [Membre 4]\n");
}