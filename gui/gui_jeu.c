/*
 * gui_jeu.c - Ecran de jeu : machine a etats d'un tour, animations, panneau.
 *
 * Etats d'un tour :
 *   ST_LANCER -> ST_DE (animation du de) -> ST_CHOIX (humain, plusieurs coups)
 *             -> ST_ANIM (deplacement du pion) -> ST_PAUSE -> (relancer | fin du tour)
 * La logique des regles est entierement dans regles.c ; ce fichier ne fait
 * que la mettre en scene.
 */
#include "gui_common.h"
#include "affichage.h"
#include "file.h"
#include "historique.h"
#include "joueur.h"
#include "plateau.h"
#include "regles.h"
#include "sauvegarde.h"
#include "utils.h"
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define PL        720          /* bord gauche du panneau lateral */
#define PW        340
#define PCX       (PL + PW / 2)
#define STEP      0.13f        /* duree d'un pas de pion (secondes) */
#define NB_LOG    7

typedef enum { ST_LANCER, ST_DE, ST_PAUSE, ST_CHOIX, ST_ANIM, ST_FIN } Etat;
typedef enum { PA_FIN_TOUR, PA_RELANCER, PA_JOUER } ApresPause;
typedef enum { MOD_AUCUN, MOD_REGLES, MOD_QUITTER, MOD_HISTO } Modal;

typedef struct { float x, y, r; int j, k, pos; } PionVis;

static Partie     g_p;
static Historique g_h;
static Config     g_cfg;
static char       g_noms[MAX_JOUEURS][MAX_NOM + 4];

static Etat       g_etat;
static ApresPause g_apres;
static Modal      g_modal = MOD_AUCUN;
static float      g_t;                 /* minuterie de l'etat courant */
static float      g_vitesse = 1.0f;
static float      g_temps = 0.0f;      /* horloge d'animation */
static float      g_fin_t = 0.0f;
static int        g_retour_menu = 0;

static int        g_de = 0, g_de_final = 0, g_face = 1, g_six = 0;
static float      g_face_t = 0.0f;
static Coup       g_coups[NB_PIONS];
static int        g_n = 0, g_choix = 0;

static Coup       g_coup_anim;
static int        g_joueur_anim = -1;
static float      g_anim_t = 0.0f;
static Vector2    g_chemin[10];
static int        g_nch = 0;

static PionVis    g_vis[MAX_JOUEURS * NB_PIONS];
static int        g_nvis = 0;

static char       g_log[NB_LOG][110];
static int        g_log_s[NB_LOG];

static float      g_pct[MAX_JOUEURS];      /* pourcentage affiche (anime) */
static float      g_de_x = 0.0f, g_de_y = 0.0f;
static int        g_de_init = 0;

/* ------------------------------------------------------------------ */
/* Utilitaires                                                         */
/* ------------------------------------------------------------------ */

static int courant(void)   { return file_tete(&g_p.file); }
static int siege_de(int j) { return g_p.joueurs[j].siege; }
static int humain(int j)   { return g_p.joueurs[j].type == HUMAIN; }

static void log_ajouter(int siege, const char *fmt, ...)
{
    va_list ap;
    int i;
    for (i = 0; i < NB_LOG - 1; i++) {
        memcpy(g_log[i], g_log[i + 1], sizeof g_log[i]);
        g_log_s[i] = g_log_s[i + 1];
    }
    va_start(ap, fmt);
    vsnprintf(g_log[NB_LOG - 1], sizeof g_log[NB_LOG - 1], fmt, ap);
    va_end(ap);
    g_log_s[NB_LOG - 1] = siege;
}

static void calculer_noms(void)
{
    int j;
    for (j = 0; j < g_p.nb_joueurs; j++)
        gui_nom_affiche(g_p.joueurs[j].nom, g_noms[j], (int)sizeof g_noms[j]);
}

static void reinitialiser_affichage(void)
{
    int i;
    historique_liberer(&g_h);
    historique_init(&g_h);
    for (i = 0; i < NB_LOG; i++) { g_log[i][0] = '\0'; g_log_s[i] = -1; }
    g_modal = MOD_AUCUN;
    g_six = 0;
    g_de = 0;
    g_n = 0;
    g_joueur_anim = -1;
    g_de_init = 0;
    g_pct[0] = g_pct[1] = g_pct[2] = g_pct[3] = -1.0f;   /* sera initialise au premier affichage */
    calculer_noms();
}

static void empiler_evenement(TypeEvenement type, int j, const Coup *c, int rang)
{
    Evenement ev;
    memset(&ev, 0, sizeof ev);
    ev.type = type; ev.tour = g_p.nb_tours; ev.joueur = j; ev.de = g_de;
    ev.victime_joueur = -1; ev.victime_pion = -1; ev.rang = rang;
    if (c != NULL) {
        ev.pion = c->pion; ev.depart = c->depart; ev.arrivee = c->arrivee;
        ev.victime_joueur = c->victime_joueur; ev.victime_pion = c->victime_pion;
    }
    historique_empiler(&g_h, &ev);
}

/* ------------------------------------------------------------------ */
/* Demarrage                                                           */
/* ------------------------------------------------------------------ */

static void commencer_lancer(float delai)
{
    g_de = 0;                               /* le de affiche "?" avant le lancer */
    g_etat = ST_LANCER;
    g_t = delai;
}

void gui_jeu_vitesse(float facteur) { g_vitesse = facteur; }

void gui_jeu_demarrer(const Config *cfg)
{
    int i;
    g_cfg = *cfg;
    memset(&g_p, 0, sizeof g_p);
    g_p.nb_joueurs = cfg->nb_joueurs;
    g_p.mode = cfg->mode;
    for (i = 0; i < cfg->nb_joueurs; i++) {
        char nom[MAX_NOM];
        char *c;
        snprintf(nom, sizeof nom, "%s", cfg->noms[i]);
        utils_trim(nom);
        if (nom[0] == '\0') snprintf(nom, sizeof nom, "Joueur%d", i + 1);
        for (c = nom; *c; c++) if (*c == ' ') *c = '_';
        joueur_init(&g_p.joueurs[i], nom, cfg->ia[i] ? ORDINATEUR : HUMAIN,
                    gui_siege_de_rang(cfg->nb_joueurs, i));
    }
    regles_init_partie(&g_p);
    g_p.nb_tours = 1;
    reinitialiser_affichage();
    commencer_lancer(0.5f);
}

int gui_jeu_charger(const char *chemin)
{
    Partie t;
    int i;
    if (charger_partie(&t, chemin) != SAUV_OK) return 0;
    g_p = t;
    memset(&g_cfg, 0, sizeof g_cfg);
    g_cfg.nb_joueurs = t.nb_joueurs;
    g_cfg.mode = t.mode;
    for (i = 0; i < t.nb_joueurs; i++) {
        snprintf(g_cfg.noms[i], MAX_NOM, "%s", t.joueurs[i].nom);
        g_cfg.ia[i] = (t.joueurs[i].type == ORDINATEUR);
    }
    reinitialiser_affichage();
    log_ajouter(-1, "Partie chargée.");
    if (regles_partie_terminee(&g_p) || file_vide(&g_p.file)) {
        regles_finaliser_classement(&g_p);
        g_etat = ST_FIN;
        g_fin_t = 0.0f;
    } else {
        commencer_lancer(0.5f);
    }
    return 1;
}

/* ------------------------------------------------------------------ */
/* Logique d'un tour                                                   */
/* ------------------------------------------------------------------ */

static void fin_de_tour(void)
{
    int j = file_defiler(&g_p.file);
    if (j >= 0 && g_p.joueurs[j].rang == 0) file_enfiler(&g_p.file, j);
    g_six = 0;
    if (regles_partie_terminee(&g_p) || file_vide(&g_p.file)) {
        regles_finaliser_classement(&g_p);
        g_etat = ST_FIN;
        g_fin_t = 0.0f;
        return;
    }
    g_p.nb_tours++;
    commencer_lancer(0.6f);
}

static void pause_puis(ApresPause a, float duree)
{
    g_etat = ST_PAUSE;
    g_apres = a;
    g_t = duree;
}

static void lancer_de(void)
{
    g_etat = ST_DE;
    g_t = 0.75f;
    g_face_t = 0.0f;
    g_de_final = utils_lancer_de();
}

static void construire_chemin(int j, const Coup *c)
{
    int pos, l, col;
    float x, y;
    g_nch = 0;
    plateau_coordonnees(siege_de(j), c->depart, c->pion, &l, &col);
    gui_centre_cellule(l, col, &x, &y);
    g_chemin[g_nch++] = (Vector2){ x, y };
    if (c->sortie_base) {
        plateau_coordonnees(siege_de(j), POS_DEPART, c->pion, &l, &col);
        gui_centre_cellule(l, col, &x, &y);
        g_chemin[g_nch++] = (Vector2){ x, y };
        return;
    }
    for (pos = c->depart + 1; pos <= c->arrivee && g_nch < 10; pos++) {
        plateau_coordonnees(siege_de(j), pos, c->pion, &l, &col);
        gui_centre_cellule(l, col, &x, &y);
        g_chemin[g_nch++] = (Vector2){ x, y };
    }
}

static void debuter_animation(int choix)
{
    int j = courant();
    const Coup *c = &g_coups[choix];
    g_coup_anim = *c;
    g_joueur_anim = j;
    construire_chemin(j, c);
    g_anim_t = 0.0f;
    g_etat = ST_ANIM;
    if (c->sortie_base) log_ajouter(siege_de(j), "%s sort le pion %d", g_noms[j], c->pion + 1);
    else                log_ajouter(siege_de(j), "%s : pion %d avance de %d", g_noms[j], c->pion + 1, g_de);
}

static void terminer_coup(void)
{
    const Coup *c = &g_coup_anim;
    int j = g_joueur_anim;
    int rejouer;

    regles_appliquer_coup(&g_p, j, c);
    empiler_evenement(c->sortie_base ? EV_SORTIE : EV_DEPLACEMENT, j, c, 0);
    g_joueur_anim = -1;

    if (c->capture)
        log_ajouter(siege_de(j), "%s capture un pion de %s !", g_noms[j], g_noms[c->victime_joueur]);
    if (c->arrive_maison && g_p.joueurs[j].rang == 0)
        log_ajouter(siege_de(j), "%s : un pion est arrivé !", g_noms[j]);

    if (g_p.joueurs[j].rang > 0) {
        log_ajouter(siege_de(j), "%s a gagné (rang %d) !", g_noms[j], g_p.joueurs[j].rang);
        empiler_evenement(EV_VICTOIRE, j, c, g_p.joueurs[j].rang);
        pause_puis(PA_FIN_TOUR, 0.7f);
        return;
    }
    rejouer = (g_de == VALEUR_SORTIE) || c->capture || c->arrive_maison;
    pause_puis(rejouer ? PA_RELANCER : PA_FIN_TOUR, c->capture ? 0.7f : 0.4f);
}

/* Apres que le de est lance : que peut-on jouer ? */
static void traiter_de(void)
{
    int j = courant();
    g_de = g_de_final;
    g_six = (g_de == VALEUR_SORTIE) ? g_six + 1 : 0;
    log_ajouter(siege_de(j), "%s lance : %d", g_noms[j], g_de);

    if (g_six >= MAX_SIX_CONSECUTIFS) {
        log_ajouter(siege_de(j), "Trois 6 : tour annulé !");
        empiler_evenement(EV_TROIS_SIX, j, NULL, 0);
        pause_puis(PA_FIN_TOUR, 1.1f);
        return;
    }
    g_n = regles_coups_possibles(&g_p, j, g_de, g_coups);
    if (g_n == 0) {
        log_ajouter(siege_de(j), "Aucun déplacement possible");
        empiler_evenement(EV_PASSE, j, NULL, 0);
        pause_puis(g_de == VALEUR_SORTIE ? PA_RELANCER : PA_FIN_TOUR, 1.0f);
        return;
    }
    if (g_n == 1) {
        g_choix = 0;
        pause_puis(PA_JOUER, 0.45f);
    } else if (humain(j)) {
        TraceLog(LOG_INFO, "LUDO: choix de pion attendu (%d coups)", g_n);
        g_etat = ST_CHOIX;
    } else {
        g_choix = joueur_choisir_coup_ia(&g_p, j, g_coups, g_n);
        pause_puis(PA_JOUER, 0.6f);
    }
}

/* ------------------------------------------------------------------ */
/* Position visuelle des pions                                         */
/* ------------------------------------------------------------------ */

static void calculer_visuels(void)
{
    static const float OFF[4][2] = { {-0.2f, -0.2f}, {0.2f, -0.2f}, {-0.2f, 0.2f}, {0.2f, 0.2f} };
    int cnt[TAILLE_GRILLE * TAILLE_GRILLE] = { 0 };
    int vu[TAILLE_GRILLE * TAILLE_GRILLE] = { 0 };
    int j, k, l, c;
    float x, y;

    g_nvis = 0;
    for (j = 0; j < g_p.nb_joueurs; j++)
        for (k = 0; k < NB_PIONS; k++) {
            int pos = g_p.joueurs[j].pions[k];
            if (pos == POS_ARRIVEE) continue;
            plateau_coordonnees(siege_de(j), pos, k, &l, &c);
            cnt[l * TAILLE_GRILLE + c]++;
        }
    for (j = 0; j < g_p.nb_joueurs; j++)
        for (k = 0; k < NB_PIONS; k++) {
            int pos = g_p.joueurs[j].pions[k], n, i;
            PionVis *v;
            if (pos == POS_ARRIVEE) continue;
            plateau_coordonnees(siege_de(j), pos, k, &l, &c);
            n = cnt[l * TAILLE_GRILLE + c];
            i = vu[l * TAILLE_GRILLE + c]++;
            gui_centre_cellule(l, c, &x, &y);
            v = &g_vis[g_nvis++];
            v->j = j; v->k = k; v->pos = pos;
            if (n == 1)      { v->x = x; v->y = y; v->r = CS * 0.36f; }
            else if (n == 2) { v->x = x + (i == 0 ? -0.2f : 0.2f) * CS; v->y = y; v->r = CS * 0.27f; }
            else             { v->x = x + OFF[i & 3][0] * CS; v->y = y + OFF[i & 3][1] * CS; v->r = CS * 0.22f; }
        }
}

/* Indice du coup associe a un pion cliquable sous la souris, ou -1. */
static int coup_sous_souris(void)
{
    Vector2 m = GetMousePosition();
    int i, v, j = courant();
    for (v = g_nvis - 1; v >= 0; v--) {
        PionVis *pv = &g_vis[v];
        float dx = m.x - pv->x, dy = m.y - pv->y, rr = pv->r + 5.0f;
        if (pv->j != j || dx * dx + dy * dy > rr * rr) continue;
        for (i = 0; i < g_n; i++)
            if (g_coups[i].depart == pv->pos) return i;
    }
    return -1;
}

static int pion_est_jouable(const PionVis *pv)
{
    int i;
    if (g_etat != ST_CHOIX || pv->j != courant()) return 0;
    for (i = 0; i < g_n; i++)
        if (g_coups[i].depart == pv->pos) return 1;
    return 0;
}

/* ------------------------------------------------------------------ */
/* Mise a jour                                                         */
/* ------------------------------------------------------------------ */

static void mettre_a_jour(float dt)
{
    int j = (g_etat == ST_FIN) ? -1 : courant();
    int libre = (g_modal == MOD_AUCUN);

    switch (g_etat) {
    case ST_LANCER:
        if (j < 0) break;
        if (humain(j)) {
            if (libre && (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER) ||
                (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
                 CheckCollisionPointRec(GetMousePosition(),
                     (Rectangle){ g_de_x - 42, g_de_y - 42, 84, 84 }))))
                lancer_de();
        } else {
            g_t -= dt;
            if (g_t <= 0.0f) lancer_de();
        }
        break;
    case ST_DE:
        g_t -= dt;
        g_face_t -= dt;
        if (g_face_t <= 0.0f) { g_face = GetRandomValue(1, 6); g_face_t = 0.07f; }
        if (g_t <= 0.0f) traiter_de();
        break;
    case ST_PAUSE:
        g_t -= dt;
        if (g_t <= 0.0f) {
            if (g_apres == PA_FIN_TOUR)      fin_de_tour();
            else if (g_apres == PA_RELANCER) commencer_lancer(0.5f);
            else                             debuter_animation(g_choix);
        }
        break;
    case ST_CHOIX:
        if (libre) {
            int i = -1, touche, m;
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) i = coup_sous_souris();
            for (touche = 1; touche <= NB_PIONS && i < 0; touche++)
                if (IsKeyPressed(KEY_ONE + touche - 1))
                    for (m = 0; m < g_n; m++)
                        if (g_coups[m].pion + 1 == touche) i = m;
            if (i >= 0) { g_choix = i; debuter_animation(i); }
        }
        break;
    case ST_ANIM:
        g_anim_t += dt;
        if (g_anim_t >= (g_nch - 1) * STEP) terminer_coup();
        break;
    case ST_FIN:
        g_fin_t += dt;
        break;
    }
}

/* ------------------------------------------------------------------ */
/* Dessin                                                              */
/* ------------------------------------------------------------------ */

static void dessiner_pions(void)
{
    int v;
    float pulse = 0.5f + 0.5f * sinf(g_temps * 6.0f);
    int survol = (g_etat == ST_CHOIX && g_modal == MOD_AUCUN) ? coup_sous_souris() : -1;

    for (v = 0; v < g_nvis; v++) {
        PionVis *pv = &g_vis[v];
        if (g_etat == ST_ANIM && pv->j == g_joueur_anim && pv->k == g_coup_anim.pion) continue;
        gui_dessiner_pion(pv->x, pv->y, pv->r, siege_de(pv->j), pv->k + 1,
                          pion_est_jouable(pv) ? pulse : 0.0f);
    }

    /* ombre de destination au survol d'un pion jouable */
    if (survol >= 0) {
        int j = courant(), l, c;
        float x, y;
        plateau_coordonnees(siege_de(j), g_coups[survol].arrivee, g_coups[survol].pion, &l, &c);
        gui_centre_cellule(l, c, &x, &y);
        DrawCircleV((Vector2){ x, y }, CS * 0.40f, Fade(gui_couleur(siege_de(j)), 0.35f));
        DrawRing((Vector2){ x, y }, CS * 0.38f, CS * 0.44f, 0, 360, 30, gui_couleur(siege_de(j)));
    }

    /* pion en mouvement */
    if (g_etat == ST_ANIM && g_nch >= 2) {
        float s = g_anim_t / STEP;
        int seg = (int)s;
        float f, x, y, saut;
        if (seg >= g_nch - 1) { seg = g_nch - 2; f = 1.0f; }
        else f = s - seg;
        x = g_chemin[seg].x + (g_chemin[seg + 1].x - g_chemin[seg].x) * f;
        y = g_chemin[seg].y + (g_chemin[seg + 1].y - g_chemin[seg].y) * f;
        saut = sinf(f * PI) * 9.0f;
        gui_dessiner_pion(x, y - saut, CS * 0.38f, siege_de(g_joueur_anim), g_coup_anim.pion + 1, 0.0f);
    }
}

static const char *rang_texte(int r)
{
    static const char *T[] = { "", "1er", "2e", "3e", "4e" };
    return (r >= 1 && r <= 4) ? T[r] : "";
}

/* Pourcentage d'avancement d'un joueur : somme des positions de ses pions
 * rapportee au maximum (4 pions x 57 etapes). Le pion en train de bouger est
 * interpole pour que la barre monte en douceur. */
static float progression(int j)
{
    const Joueur *pl = &g_p.joueurs[j];
    float total = 0.0f;
    int k;
    for (k = 0; k < NB_PIONS; k++) {
        float pos = (float)pl->pions[k];
        if (g_etat == ST_ANIM && j == g_joueur_anim && k == g_coup_anim.pion && g_nch >= 2) {
            float f = g_anim_t / ((g_nch - 1) * STEP);
            if (f > 1.0f) f = 1.0f;
            pos = g_coup_anim.depart + (g_coup_anim.arrivee - g_coup_anim.depart) * f;
        }
        total += pos + 1.0f;
    }
    return 100.0f * total / (NB_PIONS * (POS_ARRIVEE + 1));
}

static void pct_texte(float v, char *buf, size_t n)
{
    snprintf(buf, n, "%d%%", (int)(v + 0.5f));
}

static void barre(float x, float y, float w, float h, float pct, Color c)
{
    DrawRectangleRounded((Rectangle){ x, y, w, h }, 0.5f, 6, (Color){ 226, 220, 212, 255 });
    if (pct > 0.5f)
        DrawRectangleRounded((Rectangle){ x, y, fmaxf(h, w * pct / 100.0f), h }, 0.5f, 6, c);
}

/* Point d'ancrage du de : au bord de la zone interieure de la base du joueur. */
static void ancre_de(int siege, float *x, float *y)
{
    static const int ZONE[4][2] = { {0, 0}, {0, 9}, {9, 9}, {9, 0} };
    float bx = BX + (ZONE[siege][1] + 1) * CS;
    float by = BY + (ZONE[siege][0] + 1) * CS;
    *x = bx + 2 * CS;
    *y = (siege <= 1) ? by + 4 * CS : by;      /* bases du haut : dé en bas de la zone ; bases du bas : en haut */
}

/* Nom + pourcentage dans le coin de chaque base, cadre autour du joueur courant. */
static void dessiner_bases_infos(void)
{
    static const int ZONE[4][2] = { {0, 0}, {0, 9}, {9, 9}, {9, 0} };
    int j, cj = (g_etat == ST_FIN) ? -1 : courant();
    float pulse = 0.5f + 0.5f * sinf(g_temps * 5.0f);

    for (j = 0; j < g_p.nb_joueurs; j++) {
        int s = siege_de(j);
        Rectangle box = { BX + (ZONE[s][1] + 1) * CS, BY + (ZONE[s][0] + 1) * CS, 4 * CS, 4 * CS };
        float cx = box.x + box.width / 2.0f;
        float y0 = (s <= 1) ? box.y + 8 : box.y + box.height - 48;
        char pc[16];

        if (j == cj)
            DrawRectangleLinesEx((Rectangle){ box.x - 4, box.y - 4, box.width + 8, box.height + 8 }, 4.0f,
                                 Fade(WHITE, 0.35f + 0.65f * pulse));
        gui_texte_tronque(g_noms[j], cx - 78, y0, 19, (Color){ 50, 44, 40, 255 }, 156);
        barre(cx - 78, y0 + 29, 108, 9, g_pct[j], gui_couleur(s));
        pct_texte(g_pct[j], pc, sizeof pc);
        gui_texte_gras(pc, cx + 38, y0 + 22, 17, gui_couleur_sombre(s));
    }
}

/* Le de, pose a cote de la base du joueur qui joue. */
static void dessiner_de_plateau(void)
{
    int j = (g_etat == ST_FIN) ? -1 : courant();
    Color accent = (j >= 0) ? gui_couleur(siege_de(j)) : (Color){ 150, 140, 130, 255 };
    float tremble = 0.0f, saut = 0.0f, taille = 66.0f;
    int valeur = g_de;

    if (g_etat == ST_DE) {
        tremble = sinf(g_temps * 60.0f) * 3.0f;
        saut = fabsf(sinf(g_temps * 14.0f)) * 12.0f;
        taille = 72.0f;
        valeur = g_face;
    }
    if (j >= 0 && humain(j) && g_etat == ST_LANCER && g_modal == MOD_AUCUN) {
        float p = sinf(g_temps * 5.0f);
        DrawCircleV((Vector2){ g_de_x, g_de_y }, 56.0f + 4.0f * p, Fade(accent, 0.30f));
        DrawRing((Vector2){ g_de_x, g_de_y }, 52.0f + 4.0f * p, 56.0f + 4.0f * p, 0, 360, 40, Fade(WHITE, 0.9f));
    }
    gui_dessiner_de(g_de_x + tremble, g_de_y - saut + tremble * 0.5f, taille, valeur, accent);
}

static void dessiner_panneau(void)
{
    const Color encre = { 50, 44, 40, 255 };
    const Color gris  = { 130, 120, 112, 255 };
    int i, j = (g_etat == ST_FIN) ? -1 : courant();
    int actif = (g_modal == MOD_AUCUN);
    char buf[160];
    Rectangle carte = { PL - 10, 22, PW + 40, 676 };

    gui_ombre_carte(carte, WHITE);
    gui_texte_gras("LUDO", PL + 4, 30, 34, encre);
    for (i = 0; i < 4; i++) DrawCircleV((Vector2){ PL + 126 + i * 24, 52 }, 8, gui_couleur(i));

    /* joueur courant */
    if (j >= 0) {
        int s = siege_de(j);
        Rectangle band = { PL, 82, PW + 20, 62 };
        DrawRectangleRounded(band, 0.3f, 8, gui_couleur(s));
        gui_texte("Au tour de", band.x + 16, band.y + 6, 16, Fade(gui_contraste(gui_couleur(s)), 0.85f));
        gui_texte_tronque(g_noms[j], band.x + 16, band.y + 24, 28, gui_contraste(gui_couleur(s)), 220);
        gui_texte(humain(j) ? "Humain" : "Ordinateur",
                  band.x + band.width - 14 - gui_largeur(humain(j) ? "Humain" : "Ordinateur", 17),
                  band.y + 22, 17, gui_contraste(gui_couleur(s)));
    }

    /* consigne */
    buf[0] = '\0';
    if (g_etat == ST_LANCER && j >= 0)
        snprintf(buf, sizeof buf, humain(j) ? "Cliquez sur le dé (ou ESPACE)" : "%s va lancer le dé...", g_noms[j]);
    else if (g_etat == ST_DE)    snprintf(buf, sizeof buf, "Lancer en cours...");
    else if (g_etat == ST_CHOIX) snprintf(buf, sizeof buf, "Cliquez sur un pion lumineux");
    else if (g_etat == ST_FIN)   snprintf(buf, sizeof buf, "Partie terminée");
    else if (g_de > 0)           snprintf(buf, sizeof buf, "Résultat : %d", g_de);
    gui_texte_centre(buf, PCX + 10, 156, 20, g_etat == ST_CHOIX ? encre : gris);

    /* avancement des joueurs */
    gui_texte("Avancement", PL, 188, 16, gris);
    for (i = 0; i < g_p.nb_joueurs; i++) {
        const Joueur *pl = &g_p.joueurs[i];
        float y = 214 + i * 58, droite = PL + PW + 14;
        int k, arr = 0;
        char pc[16];
        if (i == j) DrawRectangleRounded((Rectangle){ PL - 2, y - 7, PW + 24, 54 }, 0.2f, 8,
                                         Fade(gui_couleur(pl->siege), 0.16f));
        DrawCircleV((Vector2){ PL + 14, y + 11 }, 11, gui_couleur(pl->siege));
        gui_texte_tronque(g_noms[i], PL + 34, y - 1, 20, encre, 190);
        if (pl->rang > 0) {
            const char *rt = rang_texte(pl->rang);
            gui_texte_gras(rt, droite - gui_largeur(rt, 21), y - 2, 21, gui_couleur_sombre(pl->siege));
        } else {
            pct_texte(g_pct[i], pc, sizeof pc);
            gui_texte_gras(pc, droite - gui_largeur(pc, 21), y - 2, 21, gui_couleur_sombre(pl->siege));
        }
        barre(PL + 34, y + 25, droite - (PL + 34), 10, g_pct[i], gui_couleur(pl->siege));
        for (k = 0; k < NB_PIONS; k++) if (pl->pions[k] == POS_ARRIVEE) arr++;
        snprintf(buf, sizeof buf, "%d/4 pions arrivés", arr);
        gui_texte(buf, PL + 34, y + 37, 13, gris);
    }

    /* journal */
    {
        float y0 = 214 + 4 * 58 + 2;
        gui_texte("Journal", PL, y0, 17, gris);
        DrawLine(PL, (int)y0 + 22, PL + PW + 20, (int)y0 + 22, (Color){ 225, 218, 210, 255 });
        for (i = 0; i < NB_LOG; i++) {
            Color c = g_log_s[i] >= 0 ? gui_couleur_sombre(g_log_s[i]) : gris;
            if (i < NB_LOG - 1) c = Fade(c, 0.45f + 0.09f * i);
            if (g_log[i][0] != '\0')
                gui_texte_tronque(g_log[i], PL, y0 + 26 + i * 21, 17, c, PW + 16);
        }
    }

    /* boutons */
    if (gui_bouton((Rectangle){ PL, 654, 76, 34 }, "Sauver", (Color){ 56, 140, 90, 255 }, 17, actif)) {
        if (sauvegarder_partie(&g_p, FICHIER_SAUVEGARDE) == SAUV_OK) {
            gui_sauvegarde_persistante();
            log_ajouter(-1, "Partie sauvegardée.");
        }
        else
            log_ajouter(-1, "Erreur de sauvegarde !");
    }
    if (gui_bouton((Rectangle){ PL + 86, 654, 76, 34 }, "Règles", (Color){ 140, 130, 124, 255 }, 17, actif))
        g_modal = MOD_REGLES;
    if (gui_bouton((Rectangle){ PL + 172, 654, 76, 34 }, "Histo.", (Color){ 140, 130, 124, 255 }, 17, actif))
        g_modal = MOD_HISTO;
    if (gui_bouton((Rectangle){ PL + 258, 654, 82, 34 }, "Menu", (Color){ 226, 64, 64, 255 }, 17, actif))
        g_modal = MOD_QUITTER;
}

static void dessiner_fin(void)
{
    Rectangle carte = { BX + 100, BY + 130, 460, 400 };
    int ordre[MAX_JOUEURS], i;
    char buf[128];
    int p;

    /* confettis */
    for (p = 0; p < 70; p++) {
        float x = fmodf((float)(p * 97 % 660) + sinf(g_fin_t * 2.0f + p) * 18.0f, 660.0f) + BX;
        float y = fmodf(g_fin_t * (60.0f + (p % 5) * 18.0f) + p * 41.0f, 700.0f);
        DrawRectanglePro((Rectangle){ x, y, 9, 5 }, (Vector2){ 4, 2 }, g_fin_t * 120.0f + p * 30.0f,
                         gui_couleur(p % 4));
    }
    gui_ombre_carte(carte, WHITE);
    regles_classement(&g_p, ordre);
    snprintf(buf, sizeof buf, "Victoire de %s !", g_noms[ordre[0]]);
    gui_texte_centre(buf, carte.x + carte.width / 2, carte.y + 26, 36, gui_couleur_sombre(siege_de(ordre[0])));
    gui_texte_centre("Classement", carte.x + carte.width / 2, carte.y + 82, 22, (Color){ 130, 120, 112, 255 });
    for (i = 0; i < g_p.nb_joueurs; i++) {
        float y = carte.y + 120 + i * 44;
        int s = siege_de(ordre[i]);
        char nb[16];
        snprintf(nb, sizeof nb, "%d.", i + 1);
        gui_texte_gras(nb, carte.x + 70, y + 2, 26, (Color){ 50, 44, 40, 255 });
        gui_dessiner_pion(carte.x + 130, y + 15, 14, s, 0, 0.0f);
        gui_texte_tronque(g_noms[ordre[i]], carte.x + 160, y + 2, 26, (Color){ 50, 44, 40, 255 }, 260);
    }
    snprintf(buf, sizeof buf, "Partie terminée en %d tours", g_p.nb_tours);
    gui_texte_centre(buf, carte.x + carte.width / 2, carte.y + 306, 18, (Color){ 130, 120, 112, 255 });
    if (gui_bouton((Rectangle){ carte.x + 40, carte.y + 336, 180, 46 }, "Rejouer", (Color){ 56, 176, 84, 255 }, 24,
                   g_modal == MOD_AUCUN))
        gui_jeu_demarrer(&g_cfg);
    if (gui_bouton((Rectangle){ carte.x + 240, carte.y + 336, 180, 46 }, "Menu", (Color){ 226, 64, 64, 255 }, 24,
                   g_modal == MOD_AUCUN))
        g_retour_menu = 1;
}

static int dessiner_modal(void)
{
    int retour_menu = 0;
    if (g_modal == MOD_REGLES) {
        if (gui_modal_regles()) g_modal = MOD_AUCUN;
    } else if (g_modal == MOD_HISTO) {
        Rectangle carte = { 150, 90, 800, 540 };
        const Evenement *ev = g_h.sommet;
        int i;
        char buf[220];
        gui_fond_modal();
        gui_ombre_carte(carte, WHITE);
        gui_texte_centre("Historique (du plus récent au plus ancien)", carte.x + carte.width / 2,
                         carte.y + 20, 30, (Color){ 50, 44, 40, 255 });
        if (ev == NULL)
            gui_texte_centre("Aucun coup joué pour le moment.", carte.x + carte.width / 2, carte.y + 120, 22,
                             (Color){ 130, 120, 112, 255 });
        for (i = 0; ev != NULL && i < 13; ev = ev->suivant, i++) {
            affichage_decrire_evenement(&g_p, ev, buf, sizeof buf, 1);
            DrawCircleV((Vector2){ carte.x + 34, carte.y + 92 + i * 33 }, 7, gui_couleur(siege_de(ev->joueur)));
            gui_texte_tronque(buf, carte.x + 52, carte.y + 80 + i * 33, 20, (Color){ 70, 62, 58, 255 }, 720);
        }
        gui_texte_centre("Cliquez n'importe où pour fermer", carte.x + carte.width / 2,
                         carte.y + carte.height - 36, 17, (Color){ 140, 130, 124, 255 });
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsKeyPressed(KEY_ESCAPE)) g_modal = MOD_AUCUN;
    } else if (g_modal == MOD_QUITTER) {
        Rectangle carte = { 300, 220, 500, 280 };
        gui_fond_modal();
        gui_ombre_carte(carte, WHITE);
        gui_texte_centre("Retourner au menu ?", carte.x + carte.width / 2, carte.y + 24, 32, (Color){ 50, 44, 40, 255 });
        if (gui_bouton((Rectangle){ carte.x + 50, carte.y + 90, 400, 44 }, "Sauvegarder et quitter",
                       (Color){ 56, 140, 90, 255 }, 21, 1)) {
            if (sauvegarder_partie(&g_p, FICHIER_SAUVEGARDE) == SAUV_OK) gui_sauvegarde_persistante();
            retour_menu = 1;
        }
        if (gui_bouton((Rectangle){ carte.x + 50, carte.y + 148, 400, 44 }, "Quitter sans sauvegarder",
                       (Color){ 226, 64, 64, 255 }, 21, 1))
            retour_menu = 1;
        if (gui_bouton((Rectangle){ carte.x + 50, carte.y + 206, 400, 44 }, "Continuer la partie",
                       (Color){ 140, 130, 124, 255 }, 21, 1) || IsKeyPressed(KEY_ESCAPE))
            g_modal = MOD_AUCUN;
    }
    return retour_menu;
}

int gui_jeu_frame(void)
{
    float dt = GetFrameTime() * g_vitesse;
    int retour = 0;

    g_retour_menu = 0;
    g_temps += GetFrameTime();
    if (IsKeyPressed(KEY_ESCAPE) && g_modal == MOD_AUCUN) g_modal = MOD_QUITTER;
    if (g_modal == MOD_AUCUN) mettre_a_jour(dt);

    /* pourcentages et de : mouvements doux */
    {
        int i, cj = (g_etat == ST_FIN) ? -1 : courant();
        float k = fminf(1.0f, GetFrameTime() * 8.0f);
        for (i = 0; i < g_p.nb_joueurs; i++) {
            float cible = progression(i);
            g_pct[i] = (g_pct[i] < 0.0f) ? cible : g_pct[i] + (cible - g_pct[i]) * k;
        }
        if (cj >= 0) {
            float ax, ay, kd = fminf(1.0f, GetFrameTime() * 9.0f);
            ancre_de(siege_de(cj), &ax, &ay);
            if (!g_de_init) { g_de_x = ax; g_de_y = ay; g_de_init = 1; }
            else { g_de_x += (ax - g_de_x) * kd; g_de_y += (ay - g_de_y) * kd; }
        }
    }

    calculer_visuels();
    gui_dessiner_plateau(&g_p);
    dessiner_bases_infos();
    dessiner_pions();
    if (g_de_init) dessiner_de_plateau();
    dessiner_panneau();
    if (g_etat == ST_FIN) {
        dessiner_fin();
        if (g_retour_menu) retour = 1;
    }
    if (g_modal != MOD_AUCUN && dessiner_modal()) retour = 1;
    return retour;
}
