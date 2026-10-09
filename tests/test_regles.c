/* Tests unitaires de la logique du jeu. Lancer avec : make test */
#include "affichage.h"
#include "file.h"
#include "historique.h"
#include "joueur.h"
#include "regles.h"
#include "sauvegarde.h"
#include <stdio.h>
#include <string.h>

static int total = 0, echecs = 0;

#define CHECK(cond) do { total++; if (!(cond)) { echecs++; \
    printf("  ECHEC ligne %d : %s\n", __LINE__, #cond); } } while (0)

/* Partie a 2 joueurs : Rouge (siege 0) et Jaune (siege 2). */
static void nouvelle_partie(Partie *p)
{
    memset(p, 0, sizeof *p);
    p->nb_joueurs = 2;
    joueur_init(&p->joueurs[0], "A", HUMAIN, 0);
    joueur_init(&p->joueurs[1], "B", ORDINATEUR, 2);
    regles_init_partie(p);
}

static void test_cases(void)
{
    printf("test_cases\n");
    CHECK(regles_case_absolue(0, 0) == 0);
    CHECK(regles_case_absolue(1, 0) == 13);
    CHECK(regles_case_absolue(3, 50) == 37);
    CHECK(regles_case_absolue(2, 51) == -1);
    CHECK(regles_case_absolue(0, POS_BASE) == -1);
    CHECK(regles_case_sure(0) && regles_case_sure(8) && regles_case_sure(13));
    CHECK(regles_case_sure(21) && regles_case_sure(47));
    CHECK(!regles_case_sure(1) && !regles_case_sure(12) && !regles_case_sure(-1));
}

static void test_sortie(void)
{
    Partie p; Coup c[NB_PIONS];
    printf("test_sortie\n");
    nouvelle_partie(&p);
    CHECK(regles_coups_possibles(&p, 0, 5, c) == 0);
    CHECK(regles_coups_possibles(&p, 0, 1, c) == 0);
    CHECK(regles_coups_possibles(&p, 0, 6, c) == 1);   /* coups equivalents regroupes */
    CHECK(c[0].sortie_base && c[0].arrivee == 0 && c[0].pion == 0);
    regles_appliquer_coup(&p, 0, &c[0]);
    CHECK(p.joueurs[0].pions[0] == 0);
    CHECK(regles_coups_possibles(&p, 0, 6, c) == 2);   /* avancer ou sortir un autre */
}

static void test_arrivee_exacte(void)
{
    Partie p; Coup c[NB_PIONS];
    printf("test_arrivee_exacte\n");
    nouvelle_partie(&p);
    p.joueurs[0].pions[0] = 54;
    CHECK(regles_coups_possibles(&p, 0, 3, c) == 0);
    CHECK(regles_coups_possibles(&p, 0, 2, c) == 1);
    CHECK(c[0].arrive_maison && c[0].arrivee == POS_ARRIVEE);
    CHECK(regles_coups_possibles(&p, 0, 1, c) == 1 && !c[0].arrive_maison);
}

static void test_capture(void)
{
    Partie p; Coup c[NB_PIONS];
    printf("test_capture\n");
    nouvelle_partie(&p);
    p.joueurs[0].pions[0] = 7;          /* case absolue 7 */
    p.joueurs[1].pions[2] = 36;         /* siege 2 : (26+36)%52 = 10 */
    CHECK(regles_coups_possibles(&p, 0, 3, c) == 1);
    CHECK(c[0].capture && c[0].victime_joueur == 1 && c[0].victime_pion == 2);
    regles_appliquer_coup(&p, 0, &c[0]);
    CHECK(p.joueurs[0].pions[0] == 10);
    CHECK(p.joueurs[1].pions[2] == POS_BASE);
}

static void test_case_sure(void)
{
    Partie p; Coup c[NB_PIONS];
    printf("test_case_sure\n");
    nouvelle_partie(&p);
    p.joueurs[0].pions[0] = 5;
    p.joueurs[1].pions[0] = 34;         /* (26+34)%52 = 8 : etoile */
    CHECK(regles_coups_possibles(&p, 0, 3, c) == 1);
    CHECK(!c[0].capture);
    regles_appliquer_coup(&p, 0, &c[0]);
    CHECK(p.joueurs[1].pions[0] == 34); /* l'adversaire n'a pas ete touche */
}

static void test_tour(void)
{
    Partie p; Coup c[NB_PIONS];
    printf("test_tour_bloquante\n");
    nouvelle_partie(&p);
    p.joueurs[0].pions[0] = 7;
    p.joueurs[1].pions[0] = 36;
    p.joueurs[1].pions[1] = 36;         /* deux pions adverses sur la case 10 */
    CHECK(regles_coups_possibles(&p, 0, 3, c) == 0);
    CHECK(regles_coups_possibles(&p, 0, 4, c) == 1);   /* on peut la depasser */
}

static void test_victoire_classement(void)
{
    Partie p; Coup c[NB_PIONS]; int ordre[MAX_JOUEURS];
    printf("test_victoire_classement\n");
    nouvelle_partie(&p);
    p.joueurs[0].pions[0] = p.joueurs[0].pions[1] = p.joueurs[0].pions[2] = POS_ARRIVEE;
    p.joueurs[0].pions[3] = 55;
    p.joueurs[1].pions[0] = 20;
    CHECK(!regles_partie_terminee(&p));
    CHECK(regles_coups_possibles(&p, 0, 1, c) == 1);
    regles_appliquer_coup(&p, 0, &c[0]);
    CHECK(regles_a_gagne(&p.joueurs[0]));
    CHECK(p.joueurs[0].rang == 1 && p.nb_classes == 1);
    CHECK(regles_partie_terminee(&p));          /* mode premier vainqueur */
    regles_classement(&p, ordre);
    CHECK(ordre[0] == 0 && ordre[1] == 1);
    p.mode = FIN_CLASSEMENT_COMPLET;
    CHECK(regles_partie_terminee(&p));          /* 2 joueurs : 1 classe suffit */
    regles_finaliser_classement(&p);
    CHECK(p.joueurs[1].rang == 2);
}

static void test_file(void)
{
    FileTours f;
    printf("test_file\n");
    file_init(&f);
    CHECK(file_vide(&f) && file_tete(&f) == -1 && file_defiler(&f) == -1);
    file_enfiler(&f, 2); file_enfiler(&f, 0); file_enfiler(&f, 1);
    CHECK(file_tete(&f) == 2);
    CHECK(file_defiler(&f) == 2);
    file_enfiler(&f, 2); file_enfiler(&f, 3);
    CHECK(file_enfiler(&f, 9) == -1);            /* pleine */
    CHECK(file_defiler(&f) == 0 && file_defiler(&f) == 1);
    CHECK(file_defiler(&f) == 2 && file_defiler(&f) == 3);
    CHECK(file_vide(&f));
}

static void test_historique(void)
{
    Historique h; Evenement e, out;
    printf("test_historique\n");
    historique_init(&h);
    memset(&e, 0, sizeof e);
    CHECK(historique_depiler(&h, &out) == -1);
    e.tour = 1; historique_empiler(&h, &e);
    e.tour = 2; historique_empiler(&h, &e);
    e.tour = 3; historique_empiler(&h, &e);
    CHECK(h.taille == 3 && h.sommet->tour == 3);
    CHECK(historique_depiler(&h, &out) == 0 && out.tour == 3 && h.taille == 2);
    historique_liberer(&h);
    CHECK(h.taille == 0 && h.sommet == NULL);
}

static void test_sauvegarde(void)
{
    Partie a, b; int j, k;
    printf("test_sauvegarde\n");
    nouvelle_partie(&a);
    a.nb_tours = 17;
    a.joueurs[0].pions[1] = 30; a.joueurs[1].pions[3] = 53;
    CHECK(sauvegarder_partie(&a, "build/test.ludo") == SAUV_OK);
    CHECK(charger_partie(&b, "build/test.ludo") == SAUV_OK);
    CHECK(b.nb_joueurs == 2 && b.nb_tours == 17 && b.mode == a.mode);
    for (j = 0; j < 2; j++) {
        CHECK(strcmp(a.joueurs[j].nom, b.joueurs[j].nom) == 0);
        CHECK(a.joueurs[j].siege == b.joueurs[j].siege && a.joueurs[j].type == b.joueurs[j].type);
        for (k = 0; k < NB_PIONS; k++) CHECK(a.joueurs[j].pions[k] == b.joueurs[j].pions[k]);
    }
    CHECK(file_tete(&b.file) == file_tete(&a.file));
    CHECK(charger_partie(&b, "build/inexistant.ludo") == SAUV_ERREUR_FICHIER);
    {   /* fichier corrompu */
        FILE *f = fopen("build/corrompu.ludo", "w");
        fputs("LUDO_SAVE_V1\n9 0 0 0 0\n", f);
        fclose(f);
        CHECK(charger_partie(&b, "build/corrompu.ludo") == SAUV_ERREUR_FORMAT);
    }
}

static void test_ia(void)
{
    Partie p; Coup c[NB_PIONS]; int n, choix;
    printf("test_ia\n");
    nouvelle_partie(&p);
    p.joueurs[0].pions[0] = 7;
    p.joueurs[0].pions[1] = 20;
    p.joueurs[1].pions[0] = 36;                  /* pion adverse sur la case 10 */
    n = regles_coups_possibles(&p, 0, 3, c);
    CHECK(n == 2);
    choix = joueur_choisir_coup_ia(&p, 0, c, n);
    CHECK(c[choix].capture);                     /* l'IA prefere capturer */

    nouvelle_partie(&p);
    p.joueurs[0].pions[0] = 52 - 3 + 0;          /* pas utile : test arrivee */
    p.joueurs[0].pions[0] = 54;
    p.joueurs[0].pions[1] = 10;
    n = regles_coups_possibles(&p, 0, 2, c);
    choix = joueur_choisir_coup_ia(&p, 0, c, n);
    CHECK(c[choix].arrive_maison);               /* ... et finir un pion */
}

int main(void)
{
    affichage_silencieux(1);
    test_cases();
    test_sortie();
    test_arrivee_exacte();
    test_capture();
    test_case_sure();
    test_tour();
    test_victoire_classement();
    test_file();
    test_historique();
    test_sauvegarde();
    test_ia();
    printf("\n%d verifications, %d echec(s)\n", total, echecs);
    return echecs == 0 ? 0 : 1;
}
