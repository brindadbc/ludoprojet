/*
 * gui_menu.c - Ecran de demarrage : nombre de joueurs, noms, humain/ordinateur.
 */
#include "gui_common.h"
#include <stdio.h>
#include <string.h>

static int   g_champ = -1;                 /* champ de nom en cours d'edition */
static int   g_edite[MAX_JOUEURS] = { 0, 0, 0, 0 };
static int   g_regles = 0;
static char  g_msg[96] = "";
static float g_msg_fin = 0.0f;

int gui_siege_de_rang(int nb, int rang)
{
    static const int SIEGES_2[2] = { 0, 2 };   /* a deux : sieges opposes */
    return nb == 2 ? SIEGES_2[rang] : rang;
}

static void nom_defaut(Config *c, int i)
{
    snprintf(c->noms[i], MAX_NOM, "%s %d", c->ia[i] ? "Ordi" : "Joueur", i + 1);
}

void gui_config_defaut(Config *c)
{
    int i;
    memset(c, 0, sizeof *c);
    c->nb_joueurs = 4;
    c->mode = FIN_PREMIER_VAINQUEUR;
    for (i = 0; i < MAX_JOUEURS; i++) {
        c->ia[i] = (i == 0) ? 0 : 1;
        g_edite[i] = 0;
        nom_defaut(c, i);
    }
}

void gui_menu_message(const char *msg)
{
    snprintf(g_msg, sizeof g_msg, "%s", msg);
    g_msg_fin = (float)GetTime() + 3.5f;
}

/* Saisie au clavier dans le champ de nom en cours. */
static void saisir_nom(Config *c)
{
    char *nom = c->noms[g_champ];
    size_t n = strlen(nom);
    int cp;
    while ((cp = GetCharPressed()) > 0) {
        if (cp >= 32 && cp < 127 && n < 16) {
            nom[n++] = (char)cp;
            nom[n] = '\0';
            g_edite[g_champ] = 1;
        }
    }
    if ((IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE)) && n > 0) {
        nom[n - 1] = '\0';
        g_edite[g_champ] = 1;
    }
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER) || IsKeyPressed(KEY_TAB)) {
        if (nom[0] == '\0') { g_edite[g_champ] = 0; nom_defaut(c, g_champ); }
        g_champ = -1;
    }
}

MenuAction gui_menu(Config *cfg)
{
    MenuAction action = MENU_RIEN;
    const Color encre = { 50, 44, 40, 255 };
    const Color gris  = { 130, 120, 112, 255 };
    int i, n;
    float t = (float)GetTime();
    int modal = g_regles;

    /* ---- en-tete ---- */
    gui_texte_centre("LUDO", LARGEUR / 2.0f, 28, 76, encre);
    for (i = 0; i < 4; i++)
        gui_dessiner_pion(LARGEUR / 2.0f - 135 + i * 90, 128, 17, i, 0, 0.0f);
    gui_texte_centre("Projet Algo et Prog - Groupe 1 - ISESTMA 2026-2027", LARGEUR / 2.0f, 156, 20, gris);

    /* ---- nombre de joueurs ---- */
    gui_texte("Nombre de joueurs", 330, 212, 24, encre);
    for (n = 2; n <= 4; n++) {
        char lab[24];
        Rectangle r = { 560 + (n - 2) * 100, 204, 90, 44 };
        snprintf(lab, sizeof lab, "%d", n);
        if (gui_bouton(r, lab, cfg->nb_joueurs == n ? (Color){ 56, 176, 84, 255 } : (Color){ 160, 152, 144, 255 },
                       24, !modal)) {
            cfg->nb_joueurs = n;
            g_champ = -1;
        }
    }

    /* ---- lignes des joueurs ---- */
    for (i = 0; i < cfg->nb_joueurs; i++) {
        int siege = gui_siege_de_rang(cfg->nb_joueurs, i);
        float y = 276 + i * 62;
        Rectangle champ = { 410, y, 330, 46 };
        Rectangle bascule = { 760, y, 180, 46 };
        Color bord = (g_champ == i) ? gui_couleur(siege) : (Color){ 200, 192, 184, 255 };
        char tampon[MAX_NOM + 2];

        gui_dessiner_pion(360, y + 23, 18, siege, i + 1, 0.0f);
        DrawRectangleRounded(champ, 0.25f, 8, bord);
        DrawRectangleRounded((Rectangle){ champ.x + 3, champ.y + 3, champ.width - 6, champ.height - 6 },
                             0.25f, 8, WHITE);
        snprintf(tampon, sizeof tampon, "%s", cfg->noms[i]);
        gui_texte(tampon, champ.x + 14, champ.y + 9, 26, encre);
        if (g_champ == i && ((int)(t * 2.0f) % 2 == 0))
            DrawRectangle((int)(champ.x + 16 + gui_largeur(tampon, 26)), (int)champ.y + 9, 2, 28, encre);

        if (!modal && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
            CheckCollisionPointRec(GetMousePosition(), champ))
            g_champ = i;

        if (gui_bouton(bascule, cfg->ia[i] ? "Ordinateur" : "Humain",
                       cfg->ia[i] ? (Color){ 120, 100, 170, 255 } : (Color){ 58, 110, 226, 255 },
                       22, !modal)) {
            cfg->ia[i] = !cfg->ia[i];
            if (!g_edite[i]) nom_defaut(cfg, i);
        }
    }
    if (!modal && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        int dans_un_champ = 0;
        for (i = 0; i < cfg->nb_joueurs; i++) {
            Rectangle champ = { 410, 276 + i * 62, 330, 46 };
            if (CheckCollisionPointRec(GetMousePosition(), champ)) dans_un_champ = 1;
        }
        if (!dans_un_champ) g_champ = -1;
    }
    if (g_champ >= 0 && !modal) saisir_nom(cfg);

    /* ---- mode de fin ---- */
    gui_texte("Fin de partie", 330, 538, 24, encre);
    if (gui_bouton((Rectangle){ 560, 530, 190, 44 }, "Premier vainqueur",
                   cfg->mode == FIN_PREMIER_VAINQUEUR ? (Color){ 56, 176, 84, 255 } : (Color){ 160, 152, 144, 255 },
                   19, !modal))
        cfg->mode = FIN_PREMIER_VAINQUEUR;
    if (gui_bouton((Rectangle){ 760, 530, 190, 44 }, "Classement complet",
                   cfg->mode == FIN_CLASSEMENT_COMPLET ? (Color){ 56, 176, 84, 255 } : (Color){ 160, 152, 144, 255 },
                   19, !modal))
        cfg->mode = FIN_CLASSEMENT_COMPLET;

    /* ---- boutons principaux ---- */
    if (gui_bouton((Rectangle){ 410, 604, 330, 64 }, "JOUER", (Color){ 226, 64, 64, 255 }, 34, !modal))
        action = MENU_JOUER;
    if (gui_bouton((Rectangle){ 760, 604, 180, 30 }, "Règles", (Color){ 140, 130, 124, 255 }, 18, !modal))
        g_regles = 1;
    if (gui_bouton((Rectangle){ 760, 640, 85, 30 }, "Charger", (Color){ 140, 130, 124, 255 }, 18,
                   !modal && FileExists(FICHIER_SAUVEGARDE)))
        action = MENU_CHARGER;
    if (gui_bouton((Rectangle){ 855, 640, 85, 30 }, "Quitter", (Color){ 140, 130, 124, 255 }, 18, !modal))
        action = MENU_QUITTER;

    if (g_msg[0] != '\0' && (float)GetTime() < g_msg_fin)
        gui_texte_centre(g_msg, LARGEUR / 2.0f, 686, 20, (Color){ 200, 60, 50, 255 });

    if (g_regles && gui_modal_regles()) g_regles = 0;
    return action;
}
