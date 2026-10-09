/*
 * gui_dessin.c - Dessin du plateau, des pions, du de, des boutons et du texte.
 */
#include "gui_common.h"
#include "plateau.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

static Font g_police;
static int  g_police_chargee = 0;

static const Color COUL[4] = {
    { 226,  64,  64, 255 },   /* Rouge */
    {  56, 176,  84, 255 },   /* Vert  */
    { 244, 196,  48, 255 },   /* Jaune */
    {  58, 110, 226, 255 }    /* Bleu  */
};

/* ------------------------------------------------------------------ */
/* Couleurs                                                            */
/* ------------------------------------------------------------------ */

Color gui_couleur(int s) { return COUL[s]; }

Color gui_couleur_claire(int s)
{
    Color c = COUL[s];
    Color r = { (unsigned char)(c.r + (255 - c.r) * 0.6f),
                (unsigned char)(c.g + (255 - c.g) * 0.6f),
                (unsigned char)(c.b + (255 - c.b) * 0.6f), 255 };
    return r;
}

Color gui_couleur_sombre(int s)
{
    Color c = COUL[s];
    Color r = { (unsigned char)(c.r * 0.6f), (unsigned char)(c.g * 0.6f),
                (unsigned char)(c.b * 0.6f), 255 };
    return r;
}

Color gui_contraste(Color f)
{
    float lum = 0.299f * f.r + 0.587f * f.g + 0.114f * f.b;
    Color sombre = { 40, 36, 34, 255 };
    return lum > 170.0f ? sombre : WHITE;
}

/* ------------------------------------------------------------------ */
/* Texte                                                               */
/* ------------------------------------------------------------------ */

void gui_charger_police(void)
{
    static const char *chemins[] = {
        "C:/Windows/Fonts/segoeui.ttf",
        "C:/Windows/Fonts/arial.ttf",
        "/System/Library/Fonts/Supplemental/Arial.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/TTF/DejaVuSans.ttf",
        NULL
    };
    int cps[224], i;
    for (i = 0; i < 224; i++) cps[i] = 32 + i;       /* ASCII + Latin-1 (accents) */
    g_police = GetFontDefault();
    for (i = 0; chemins[i] != NULL; i++) {
        if (FileExists(chemins[i])) {
            Font f = LoadFontEx(chemins[i], 48, cps, 224);
            if (f.texture.id > 0) {
                g_police = f;
                SetTextureFilter(f.texture, TEXTURE_FILTER_BILINEAR);
                g_police_chargee = 1;
                break;
            }
        }
    }
}

void gui_liberer_police(void)
{
    if (g_police_chargee) UnloadFont(g_police);
    g_police_chargee = 0;
}

void gui_texte(const char *s, float x, float y, float taille, Color c)
{
    Vector2 pos = { x, y };
    DrawTextEx(g_police, s, pos, taille, taille * 0.04f, c);
}

void gui_texte_gras(const char *s, float x, float y, float taille, Color c)
{
    gui_texte(s, x, y, taille, c);
    gui_texte(s, x + 0.9f, y, taille, c);
}

float gui_largeur(const char *s, float taille)
{
    return MeasureTextEx(g_police, s, taille, taille * 0.04f).x;
}

void gui_texte_centre(const char *s, float cx, float y, float taille, Color c)
{
    gui_texte(s, cx - gui_largeur(s, taille) / 2.0f, y, taille, c);
}

void gui_texte_tronque(const char *s, float x, float y, float taille, Color c, float larg_max)
{
    char buf[170];
    size_t n;
    snprintf(buf, sizeof buf, "%s", s);
    n = strlen(buf);
    if (gui_largeur(buf, taille) > larg_max) {
        while (n > 0) {
            char essai[176];
            n--;
            while (n > 0 && ((unsigned char)buf[n] & 0xC0) == 0x80) n--;   /* ne pas couper un caractere UTF-8 */
            buf[n] = '\0';
            snprintf(essai, sizeof essai, "%s...", buf);
            if (gui_largeur(essai, taille) <= larg_max) { gui_texte(essai, x, y, taille, c); return; }
        }
    }
    gui_texte(buf, x, y, taille, c);
}

/* Les noms sont enregistres avec '_' a la place des espaces. */
void gui_nom_affiche(const char *nom, char *sortie, int taille)
{
    int i;
    snprintf(sortie, (size_t)taille, "%s", nom);
    for (i = 0; sortie[i]; i++) if (sortie[i] == '_') sortie[i] = ' ';
}

/* ------------------------------------------------------------------ */
/* Widgets                                                             */
/* ------------------------------------------------------------------ */

void gui_ombre_carte(Rectangle r, Color fond)
{
    Rectangle o = { r.x + 2, r.y + 5, r.width, r.height };
    DrawRectangleRounded(o, 0.04f, 10, (Color){ 0, 0, 0, 40 });
    DrawRectangleRounded(r, 0.04f, 10, fond);
}

int gui_bouton(Rectangle r, const char *label, Color fond, float taille, int actif)
{
    Vector2 m = GetMousePosition();
    int survol = actif && CheckCollisionPointRec(m, r);
    Color f = fond;
    Rectangle ombre = { r.x, r.y + 3, r.width, r.height };
    if (!actif) { f.r = 190; f.g = 190; f.b = 190; }
    else if (survol) {
        f.r = (unsigned char)(f.r + (255 - f.r) * 0.18f);
        f.g = (unsigned char)(f.g + (255 - f.g) * 0.18f);
        f.b = (unsigned char)(f.b + (255 - f.b) * 0.18f);
    }
    DrawRectangleRounded(ombre, 0.3f, 8, (Color){ 0, 0, 0, 55 });
    if (survol && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) r.y += 2;
    DrawRectangleRounded(r, 0.3f, 8, f);
    gui_texte_centre(label, r.x + r.width / 2.0f,
                     r.y + (r.height - taille) / 2.0f - 1.0f, taille, gui_contraste(f));
    return survol && IsMouseButtonReleased(MOUSE_BUTTON_LEFT);
}

/* ------------------------------------------------------------------ */
/* Plateau                                                             */
/* ------------------------------------------------------------------ */

void gui_centre_cellule(int l, int c, float *x, float *y)
{
    *x = BX + c * CS + CS / 2.0f;
    *y = BY + l * CS + CS / 2.0f;
}

/* Triangle dessine dans les deux sens pour ne pas dependre de l'orientation. */
static void triangle(Vector2 a, Vector2 b, Vector2 c, Color col)
{
    DrawTriangle(a, b, c, col);
    DrawTriangle(a, c, b, col);
}

static void etoile(float cx, float cy, float R, Color col)
{
    Vector2 pts[10], ctr = { cx, cy };
    int i;
    for (i = 0; i < 10; i++) {
        float a = -PI / 2.0f + i * PI / 5.0f;
        float r = (i % 2 == 0) ? R : R * 0.42f;
        pts[i].x = cx + cosf(a) * r;
        pts[i].y = cy + sinf(a) * r;
    }
    for (i = 0; i < 10; i++) triangle(ctr, pts[i], pts[(i + 1) % 10], col);
}

void gui_dessiner_plateau(const Partie *p)
{
    static const int ZONE[4][2] = { {0, 0}, {0, 9}, {9, 9}, {9, 0} };   /* ligne, colonne */
    int actif[MAX_JOUEURS] = { 0, 0, 0, 0 };
    int arrives[MAX_JOUEURS] = { 0, 0, 0, 0 };
    int s, k, l, c, j;
    float x0, y0, x1, y1, cx, cy;

    for (j = 0; j < p->nb_joueurs; j++) {
        int sg = p->joueurs[j].siege;
        actif[sg] = 1;
        for (k = 0; k < NB_PIONS; k++)
            if (p->joueurs[j].pions[k] == POS_ARRIVEE) arrives[sg]++;
    }

    /* cadre */
    DrawRectangle(BX - 4, BY - 2, 15 * CS + 12, 15 * CS + 12, (Color){ 0, 0, 0, 45 });
    DrawRectangle(BX - 8, BY - 8, 15 * CS + 16, 15 * CS + 16, (Color){ 74, 62, 56, 255 });
    DrawRectangle(BX, BY, 15 * CS, 15 * CS, WHITE);

    /* bases */
    for (s = 0; s < 4; s++) {
        Color fond = actif[s] ? COUL[s] : (Color){ 196, 196, 196, 255 };
        Rectangle in = { BX + (ZONE[s][1] + 1) * CS, BY + (ZONE[s][0] + 1) * CS, 4 * CS, 4 * CS };
        DrawRectangle(BX + ZONE[s][1] * CS, BY + ZONE[s][0] * CS, 6 * CS, 6 * CS, fond);
        DrawRectangleRounded(in, 0.12f, 8, (Color){ 255, 255, 255, 235 });
        for (k = 0; k < NB_PIONS; k++) {
            plateau_coordonnees(s, POS_BASE, k, &l, &c);
            gui_centre_cellule(l, c, &cx, &cy);
            DrawCircleV((Vector2){ cx, cy }, CS * 0.40f, fond);
            DrawCircleV((Vector2){ cx, cy }, CS * 0.32f, actif[s] ? gui_couleur_claire(s) : (Color){ 225, 225, 225, 255 });
        }
    }

    /* piste et couloirs */
    for (l = 0; l < TAILLE_GRILLE; l++)
        for (c = 0; c < TAILLE_GRILLE; c++) {
            const CaseInfo *ci = plateau_case(l, c);
            Rectangle r = { BX + c * CS, BY + l * CS, CS, CS };
            gui_centre_cellule(l, c, &cx, &cy);
            if (ci->type == C_PISTE) {
                DrawRectangleRec(r, (Color){ 250, 248, 244, 255 });
                if (ci->depart_de >= 0) {
                    DrawRectangleRec(r, COUL[ci->depart_de]);
                    DrawCircleV((Vector2){ cx, cy }, CS * 0.13f, WHITE);
                } else if (ci->sure) {
                    etoile(cx, cy, CS * 0.34f, (Color){ 168, 160, 150, 255 });
                }
                DrawRectangleLinesEx(r, 1.0f, (Color){ 176, 168, 158, 255 });
            } else if (ci->type == C_COULOIR) {
                DrawRectangleRec(r, COUL[ci->siege]);
                DrawRectangleLinesEx(r, 1.0f, (Color){ 255, 255, 255, 160 });
            }
        }

    /* maison centrale : 4 triangles */
    x0 = BX + 6 * CS; y0 = BY + 6 * CS; x1 = x0 + 3 * CS; y1 = y0 + 3 * CS;
    cx = (x0 + x1) / 2.0f; cy = (y0 + y1) / 2.0f;
    triangle((Vector2){ x0, y0 }, (Vector2){ x0, y1 }, (Vector2){ cx, cy }, COUL[0]);
    triangle((Vector2){ x0, y0 }, (Vector2){ x1, y0 }, (Vector2){ cx, cy }, COUL[1]);
    triangle((Vector2){ x1, y0 }, (Vector2){ x1, y1 }, (Vector2){ cx, cy }, COUL[2]);
    triangle((Vector2){ x0, y1 }, (Vector2){ x1, y1 }, (Vector2){ cx, cy }, COUL[3]);

    /* pions arrives a la maison */
    for (s = 0; s < 4; s++) {
        for (k = 0; k < arrives[s]; k++) {
            float dec = (k - (arrives[s] - 1) / 2.0f) * CS * 0.30f, px, py;
            switch (s) {
            case 0:  px = x0 + CS * 0.62f; py = cy + dec; break;
            case 1:  px = cx + dec; py = y0 + CS * 0.62f; break;
            case 2:  px = x1 - CS * 0.62f; py = cy + dec; break;
            default: px = cx + dec; py = y1 - CS * 0.62f; break;
            }
            DrawCircleV((Vector2){ px, py }, CS * 0.15f, WHITE);
            DrawCircleV((Vector2){ px, py }, CS * 0.11f, gui_couleur_sombre(s));
        }
    }
}

/* ------------------------------------------------------------------ */
/* Pions et de                                                         */
/* ------------------------------------------------------------------ */

void gui_dessiner_pion(float x, float y, float r, int siege, int numero, float lueur)
{
    char t[16];
    if (lueur > 0.0f) {
        DrawCircleV((Vector2){ x, y }, r + 6.0f + lueur * 3.0f, (Color){ 255, 255, 255, 120 });
        DrawRing((Vector2){ x, y }, r + 3.0f, r + 6.0f + lueur * 3.0f, 0, 360, 36,
                 (Color){ 255, 236, 120, 255 });
    }
    DrawEllipse((int)x, (int)(y + r * 0.78f), r * 0.85f, r * 0.30f, (Color){ 0, 0, 0, 70 });
    DrawCircleV((Vector2){ x, y }, r, gui_couleur_sombre(siege));
    DrawCircleV((Vector2){ x, y }, r * 0.86f, COUL[siege]);
    DrawCircleV((Vector2){ x - r * 0.30f, y - r * 0.32f }, r * 0.28f, (Color){ 255, 255, 255, 90 });
    if (numero > 0 && r >= CS * 0.30f) {
        float taille = r * 1.05f;
        snprintf(t, sizeof t, "%d", numero);
        gui_texte_centre(t, x, y - taille * 0.55f, taille, gui_contraste(COUL[siege]));
    }
}

void gui_dessiner_de(float cx, float cy, float t, int v, Color accent)
{
    static const int PIPS[7][6] = {
        { -1, -1, -1, -1, -1, -1 },
        {  4, -1, -1, -1, -1, -1 },
        {  0,  8, -1, -1, -1, -1 },
        {  0,  4,  8, -1, -1, -1 },
        {  0,  2,  6,  8, -1, -1 },
        {  0,  2,  4,  6,  8, -1 },
        {  0,  2,  3,  5,  6,  8 }
    };
    Rectangle r  = { cx - t / 2.0f, cy - t / 2.0f, t, t };
    Rectangle in = { r.x + 5, r.y + 5, t - 10, t - 10 };
    int i;
    DrawRectangleRounded((Rectangle){ r.x + 3, r.y + 7, t, t }, 0.22f, 10, (Color){ 0, 0, 0, 55 });
    DrawRectangleRounded(r, 0.22f, 10, accent);
    DrawRectangleRounded(in, 0.2f, 10, WHITE);
    if (v < 1 || v > 6) {
        gui_texte_centre("?", cx, cy - t * 0.28f, t * 0.55f, (Color){ 150, 150, 150, 255 });
        return;
    }
    for (i = 0; i < 6 && PIPS[v][i] >= 0; i++) {
        int g = PIPS[v][i];
        float px = cx + ((g % 3) - 1) * t * 0.26f;
        float py = cy + ((g / 3) - 1) * t * 0.26f;
        DrawCircleV((Vector2){ px, py }, t * 0.075f, (Color){ 36, 32, 30, 255 });
    }
}

/* ------------------------------------------------------------------ */
/* Fenetres modales                                                    */
/* ------------------------------------------------------------------ */

void gui_fond_modal(void)
{
    DrawRectangle(0, 0, LARGEUR, HAUTEUR, (Color){ 20, 16, 14, 150 });
}

int gui_modal_regles(void)
{
    static const char *LIGNES[] = {
        "But : amener ses 4 pions de la base jusqu'à la maison (le centre).",
        "",
        "- Lancez le dé, puis cliquez sur le pion à déplacer.",
        "- Il faut un 6 pour sortir un pion de la base.",
        "- Un 6, une capture ou l'arrivée d'un pion à la maison : on rejoue.",
        "- Trois 6 de suite : le tour est annulé.",
        "- S'arrêter sur un pion adverse seul le renvoie à sa base,",
        "   sauf sur une case sûre (cases de départ et étoiles).",
        "- Deux pions du même joueur sur une case forment une tour :",
        "   elle est imprenable et on ne peut pas s'y arrêter.",
        "- Après un tour de piste, on entre dans son couloir coloré,",
        "   où les pions sont protégés. Il faut le nombre exact pour arriver.",
        NULL
    };
    Rectangle carte = { 150, 90, 800, 540 };
    int i;
    gui_fond_modal();
    gui_ombre_carte(carte, WHITE);
    gui_texte_centre("Règles du jeu", carte.x + carte.width / 2.0f, carte.y + 24, 38, (Color){ 50, 44, 40, 255 });
    for (i = 0; LIGNES[i] != NULL; i++)
        gui_texte(LIGNES[i], carte.x + 40, carte.y + 100 + i * 34, 22, (Color){ 70, 62, 58, 255 });
    gui_texte_centre("Cliquez n'importe où pour fermer", carte.x + carte.width / 2.0f,
                     carte.y + carte.height - 44, 18, (Color){ 140, 130, 124, 255 });
    return IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsKeyPressed(KEY_ESCAPE);
}
