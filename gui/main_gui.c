/*
 * LudoProjet - Jeu de Ludo avec interface graphique (raylib).
 * Projet Algo et Prog - ISESTMA 2026-2027 - Groupe 1.
 */
#include "gui_common.h"
#include "utils.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv)
{
    Config cfg;
    Ecran ecran = ECRAN_MENU;
    unsigned int graine = utils_graine_horloge();
    const char *capture = NULL;
    int demo = 0, demo_humain = 0, image_apres = 120, image = 0, quitter = 0, i;
    float vitesse = 1.0f;
    int nb_demo = 4;

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--seed") == 0 && i + 1 < argc)        graine = (unsigned int)strtoul(argv[++i], NULL, 10);
        else if (strcmp(argv[i], "--demo") == 0)                   demo = 1;           /* 4 ordinateurs */
        else if (strcmp(argv[i], "--demo-humain") == 0)            demo_humain = 1;    /* 1 humain + 3 ordinateurs */
        else if (strcmp(argv[i], "--nb") == 0 && i + 1 < argc)      nb_demo = atoi(argv[++i]);
        else if (strcmp(argv[i], "--vitesse") == 0 && i + 1 < argc) vitesse = (float)atof(argv[++i]);
        else if (strcmp(argv[i], "--capture") == 0 && i + 1 < argc) capture = argv[++i];
        else if (strcmp(argv[i], "--images") == 0 && i + 1 < argc)  image_apres = atoi(argv[++i]);
        else {
            printf("Usage : %s [--seed N] [--demo] [--demo-humain] [--nb 2-4] [--vitesse X] [--capture fichier.png --images N]\n", argv[0]);
            return strcmp(argv[i], "--help") == 0 ? 0 : 1;
        }
    }
    utils_init_alea(graine);
    SetRandomSeed(graine);

    SetTraceLogLevel(LOG_WARNING);
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(LARGEUR, HAUTEUR, "LudoProjet - Groupe 1");
    SetWindowMinSize(550, 360);
    {   /* petit ecran (ex. portable 1366x768) : on reduit la fenetre pour qu'elle tienne */
        int mh = GetMonitorHeight(GetCurrentMonitor());
        if (mh > 0 && mh < HAUTEUR + 130) {
            float f = (mh - 130.0f) / HAUTEUR;
            SetWindowSize((int)(LARGEUR * f), (int)(HAUTEUR * f));
        }
    }
    SetExitKey(KEY_NULL);
    SetTargetFPS(60);
    gui_charger_police();

    gui_config_defaut(&cfg);
    if (demo || demo_humain) {
        for (i = 0; i < MAX_JOUEURS; i++) cfg.ia[i] = (demo_humain && i == 0) ? 0 : 1;
        snprintf(cfg.noms[0], MAX_NOM, demo_humain ? "Amir" : "Ordi 1");
        for (i = 1; i < MAX_JOUEURS; i++) snprintf(cfg.noms[i], MAX_NOM, "Ordi %d", i + 1);
        if (nb_demo >= 2 && nb_demo <= 4) cfg.nb_joueurs = nb_demo;
        gui_jeu_demarrer(&cfg);
        gui_jeu_vitesse(vitesse);
        ecran = ECRAN_JEU;
    }

    while (!WindowShouldClose() && !quitter) {
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
        if (ecran == ECRAN_MENU) {
            switch (gui_menu(&cfg)) {
            case MENU_JOUER:
                gui_jeu_demarrer(&cfg);
                gui_jeu_vitesse(1.0f);
                ecran = ECRAN_JEU;
                break;
            case MENU_CHARGER:
                if (gui_jeu_charger("sauvegarde.ludo")) { gui_jeu_vitesse(1.0f); ecran = ECRAN_JEU; }
                else gui_menu_message("Impossible de charger sauvegarde.ludo");
                break;
            case MENU_QUITTER:
                quitter = 1;
                break;
            default:
                break;
            }
        } else if (gui_jeu_frame()) {
            ecran = ECRAN_MENU;
        }
        EndMode2D();
        EndDrawing();

        image++;
        if (capture != NULL && image == image_apres) {
            TakeScreenshot(capture);
            break;
        }
    }
    gui_liberer_police();
    CloseWindow();
    return 0;
}
