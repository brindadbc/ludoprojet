/*
 * LudoProjet - Jeu de Ludo avec interface graphique (raylib).
 * Projet Algo et Prog - ISESTMA 2026-2027 - Groupe 1.
 *
 * Deux plateformes :
 *   - bureau (Windows / Linux / macOS) : boucle classique
 *   - web (PLATFORM_WEB, via Emscripten) : le navigateur appelle une_frame()
 */
#include "gui_common.h"
#include "utils.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef PLATFORM_WEB
#include <emscripten/emscripten.h>
#endif

static Config      g_cfg;
static Ecran       g_ecran = ECRAN_MENU;
static int         g_quitter = 0;
static const char *g_capture = NULL;     /* options de test : capture d'ecran */
static int         g_image_apres = 120, g_image = 0;

void gui_sauvegarde_persistante(void)
{
#ifdef PLATFORM_WEB
    EM_ASM(FS.syncfs(false, function (err) { }););
#endif
}

/* Une image : mise a jour + dessin. */
static void une_frame(void)
{
    /* La scene est dessinee en 1100x720 puis mise a l'echelle de la fenetre. */
    float echelle = fminf((float)GetScreenWidth() / LARGEUR, (float)GetScreenHeight() / HAUTEUR);
    float dx = (GetScreenWidth() - LARGEUR * echelle) / 2.0f;
    float dy = (GetScreenHeight() - HAUTEUR * echelle) / 2.0f;
    Camera2D cam = { { dx, dy }, { 0.0f, 0.0f }, 0.0f, echelle };
    SetMouseOffset((int)-dx, (int)-dy);
    SetMouseScale(1.0f / echelle, 1.0f / echelle);

    BeginDrawing();
    ClearBackground((Color){ 238, 233, 222, 255 });
    BeginMode2D(cam);
    if (g_ecran == ECRAN_MENU) {
        switch (gui_menu(&g_cfg)) {
        case MENU_JOUER:
            gui_jeu_demarrer(&g_cfg);
            gui_jeu_vitesse(1.0f);
            g_ecran = ECRAN_JEU;
            break;
        case MENU_CHARGER:
            if (gui_jeu_charger(FICHIER_SAUVEGARDE)) { gui_jeu_vitesse(1.0f); g_ecran = ECRAN_JEU; }
            else gui_menu_message("Impossible de charger la sauvegarde");
            break;
        case MENU_QUITTER:
#ifdef PLATFORM_WEB
            gui_menu_message("Fermez simplement l'onglet du navigateur.");
#else
            g_quitter = 1;
#endif
            break;
        default:
            break;
        }
    } else if (gui_jeu_frame()) {
        g_ecran = ECRAN_MENU;
    }
    EndMode2D();
    EndDrawing();

#ifndef PLATFORM_WEB
    g_image++;
    if (g_capture != NULL && g_image == g_image_apres) {
        TakeScreenshot(g_capture);
        g_quitter = 1;
    }
#endif
}

static int ludo_main(int argc, char **argv)
{
    unsigned int graine = utils_graine_horloge();
    int demo = 0, demo_humain = 0, nb_demo = 4, i;
    float vitesse = 1.0f;

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--seed") == 0 && i + 1 < argc)         graine = (unsigned int)strtoul(argv[++i], NULL, 10);
        else if (strcmp(argv[i], "--demo") == 0)                    demo = 1;           /* 4 ordinateurs */
        else if (strcmp(argv[i], "--demo-humain") == 0)             demo_humain = 1;    /* 1 humain + 3 ordinateurs */
        else if (strcmp(argv[i], "--nb") == 0 && i + 1 < argc)      nb_demo = atoi(argv[++i]);
        else if (strcmp(argv[i], "--vitesse") == 0 && i + 1 < argc) vitesse = (float)atof(argv[++i]);
        else if (strcmp(argv[i], "--capture") == 0 && i + 1 < argc) g_capture = argv[++i];
        else if (strcmp(argv[i], "--images") == 0 && i + 1 < argc)  g_image_apres = atoi(argv[++i]);
        else {
            printf("Usage : %s [--seed N] [--demo] [--demo-humain] [--nb 2-4] [--vitesse X] [--capture fichier.png --images N]\n", argv[0]);
            return strcmp(argv[i], "--help") == 0 ? 0 : 1;
        }
    }
    utils_init_alea(graine);
    SetRandomSeed(graine);

    SetTraceLogLevel(LOG_WARNING);
#ifdef PLATFORM_WEB
    SetConfigFlags(FLAG_MSAA_4X_HINT);
#else
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE);
#endif
    InitWindow(LARGEUR, HAUTEUR, "LudoProjet - Groupe 1");
#ifndef PLATFORM_WEB
    SetWindowMinSize(550, 360);
    {   /* petit ecran (ex. portable 1366x768) : on reduit la fenetre pour qu'elle tienne */
        int mh = GetMonitorHeight(GetCurrentMonitor());
        if (mh > 0 && mh < HAUTEUR + 130) {
            float f = (mh - 130.0f) / HAUTEUR;
            SetWindowSize((int)(LARGEUR * f), (int)(HAUTEUR * f));
        }
    }
    SetTargetFPS(60);
#endif
    SetExitKey(KEY_NULL);
    gui_charger_police();

#ifdef PLATFORM_WEB
    /* Sauvegarde persistante : /data est stocke dans le navigateur (IndexedDB). */
    EM_ASM(
        try { FS.mkdir('/data'); } catch (e) { }
        FS.mount(IDBFS, {}, '/data');
        FS.syncfs(true, function (err) { });
    );
#endif

    gui_config_defaut(&g_cfg);
    if (demo || demo_humain) {
        for (i = 0; i < MAX_JOUEURS; i++) g_cfg.ia[i] = (demo_humain && i == 0) ? 0 : 1;
        snprintf(g_cfg.noms[0], MAX_NOM, demo_humain ? "Amir" : "Ordi 1");
        for (i = 1; i < MAX_JOUEURS; i++) snprintf(g_cfg.noms[i], MAX_NOM, "Ordi %d", i + 1);
        if (nb_demo >= 2 && nb_demo <= 4) g_cfg.nb_joueurs = nb_demo;
        gui_jeu_demarrer(&g_cfg);
        gui_jeu_vitesse(vitesse);
        g_ecran = ECRAN_JEU;
    }

#ifdef PLATFORM_WEB
    emscripten_set_main_loop(une_frame, 0, 1);
#else
    while (!WindowShouldClose() && !g_quitter) une_frame();
#endif
    gui_liberer_police();
    CloseWindow();
    return 0;
}

#ifdef PLATFORM_WEB
/* Sur le web, pas d'arguments : main(void) (aussi requis par les anciens Emscripten). */
int main(void) { return ludo_main(0, NULL); }
#else
int main(int argc, char **argv) { return ludo_main(argc, argv); }
#endif
