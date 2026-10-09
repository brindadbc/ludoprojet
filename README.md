# 🎲 LudoProjet

> Jeu de **Ludo** en langage **C** avec **interface graphique** (fenêtre, souris, dé animé), de 2 à 4 joueurs (humains ou ordinateur). Une version terminal est aussi fournie.

**Cours :** Algo et Prog – ISESTMA, année académique 2026-2027 · **Enseignant :** Daniel MOUNE
**Équipe :** Groupe 1

---

## 👥 Équipe (Groupe 1)

## Équipe (Groupe 1)

| Nom & prénom | Rôle | Responsabilités principales |
|---|---|---|
| **DJUIKOUO BRINDA CARELLE** | Chef d'équipe | Coordination, dépôt Git, intégration, `api_web.c`, démo finale |
| MAWE POKAM DIVINE | Développeur | Plateau, déplacements (`plateau.c`, `regles.c`), plateau web |
| BOUCHE FOTSO MERVEILLE | Développeur | Tours de jeu, IA, sauvegarde (`jeu.c`, `joueur.c`, `sauvegarde.c`) |
| TIWA BRICE | Développeur / Qualité | Affichage, interface web, tests, documentation (`affichage.c`, `web/`, `tests/`, `docs/`) |

---

## 📖 Description

**LudoProjet** reproduit le jeu de société Ludo : chaque joueur possède 4 pions qu'il doit faire sortir de sa base (avec un 6), faire avancer sur la piste de 52 cases, puis amener jusqu'à la maison en entrant dans son couloir d'arrivée.

### Fonctionnalités

- 2 à 4 joueurs, chacun **humain ou ordinateur** (IA à heuristique : arrivée > capture > sortie de base > prudence)
- **Interface graphique** (bibliothèque raylib) : plateau en couleurs, **dé cliquable animé**, **pions qui sautent de case en case**, pions jouables qui brillent, journal des coups, fenêtre redimensionnable
- Dé aléatoire ; **rejouer** sur 6, capture ou arrivée ; **3 six de suite = tour perdu**
- **Captures**, **cases sûres** (départs et étoiles), **tours** (2 pions superposés = imprenables)
- Arrivée à la maison au **nombre exact**
- Deux modes de fin : **premier vainqueur** ou **classement complet**
- **Sauvegarde / chargement** de partie (fichier texte)
- **Historique** des coups, règles consultables en jeu, écran de victoire avec classement
- Version **terminal** conservée (`make console`)
- Suite de **tests automatiques** (unitaires + 1800 parties simulées)

### Vision produit

| Question | Réponse |
|---|---|
| Quel problème ? | Jouer à un jeu de société connu de tous sans plateau physique, tout en mettant en pratique algorithmique et structures de données. |
| Pour qui ? | Étudiants et amis voulant jouer rapidement en console à 2–4. |
| Valeur ajoutée | Léger, sans dépendance, règles vérifiées automatiquement, IA pour jouer seul. |

---

## ▶️ Installation et lancement

### Windows (MSYS2)

1. Installer MSYS2 (https://www.msys2.org), puis ouvrir **MSYS2 UCRT64**.
2. Installer le compilateur, `make` et la bibliothèque graphique :
   ```bash
   pacman -S mingw-w64-ucrt-x86_64-gcc make mingw-w64-ucrt-x86_64-raylib mingw-w64-ucrt-x86_64-pkgconf
   ```
3. Aller dans le dossier du projet, compiler et lancer :
   ```bash
   cd /c/Users/<nom>/Desktop/ludoprojet
   make
   ./ludo_gui.exe
   ```

> Lancer `ludo_gui.exe` depuis le terminal MSYS2 UCRT64 : c'est là que les DLL nécessaires sont trouvées.

### Linux / macOS

Installer `gcc`, `make` et raylib (`sudo apt install libraylib-dev` ou compilation depuis https://www.raylib.com), puis `make && ./ludo_gui`.

### Commandes make

```bash
make           # jeu graphique -> ./ludo_gui
make console   # jeu en terminal -> ./ludo
make test      # tests automatiques
make clean
```

### Comment jouer (interface graphique)

| Action | Comment |
|---|---|
| Lancer le dé | cliquer sur le dé ou touche **Espace** |
| Déplacer un pion | cliquer sur un pion qui **brille** (ou touche 1–4) ; survol = case d'arrivée |
| Sauvegarder | bouton **Sauver** (fichier `sauvegarde.ludo`), rechargeable depuis le menu |
| Règles / historique | boutons **Règles** et **Histo.** |
| Quitter la partie | bouton **Menu** ou touche **Échap** |

Options de lancement : `--seed N` (partie reproductible), `--demo` (4 ordinateurs), `--help`.

---

## 🧱 Architecture

```
ludoprojet/
├── Makefile
├── README.md
├── docs/
│   ├── vision_produit.md      # fiche de vision produit (séance 1)
│   └── algorithmes.md         # algorithmes en pseudo-code (séances 2-4)
├── src/
│   ├── main.c                 # version terminal : menu et options
│   ├── types.h                # constantes et structures
│   ├── regles.c/.h            # logique pure : coups légaux, captures, victoire
│   ├── plateau.c/.h           # géométrie : piste de 52 cases, bases, couloirs
│   ├── jeu.c/.h               # tour de jeu (version terminal)
│   ├── joueur.c/.h            # création des joueurs, IA
│   ├── affichage.c/.h         # affichage terminal + textes des événements
│   ├── file.c/.h              # FILE circulaire : ordre de passage
│   ├── historique.c/.h        # PILE (liste chaînée) : historique des coups
│   ├── sauvegarde.c/.h        # sauvegarde / chargement
│   └── utils.c/.h             # dé, saisies sécurisées
├── gui/                       # INTERFACE GRAPHIQUE (raylib)
│   ├── main_gui.c             # fenêtre, boucle principale, mise à l'échelle
│   ├── gui_menu.c             # écran de démarrage (joueurs, noms, humain/ordi)
│   ├── gui_jeu.c              # machine à états d'un tour, animations, panneau
│   ├── gui_dessin.c           # plateau, pions, dé, boutons, texte
│   └── gui_common.h
└── tests/
    ├── test_regles.c          # tests unitaires
    └── test_simulation.c      # 1800 parties IA vs IA + vérification d'invariants
```

Le **moteur de jeu** (`regles`, `plateau`, `joueur`, `file`, `historique`, `sauvegarde`) ne contient aucun affichage : il est partagé par la version terminal et la version graphique, et c'est lui que `make test` vérifie.

### Structures de données utilisées (lien avec le cours)

| Structure | Où | Rôle |
|---|---|---|
| Tableaux | `Joueur.pions[]`, tables du plateau | positions des pions, coordonnées de la piste |
| **File circulaire** | `file.c` | ordre de passage des joueurs (les joueurs classés sortent de la file) |
| **Pile / liste chaînée** | `historique.c` | historique des événements, du plus récent au plus ancien |
| Tri par insertion | `regles_classement` | classement final |
| Structures (`struct`) | `types.h` | `Joueur`, `Partie`, `Coup` |

### Modèle de position d'un pion

`-1` base · `0..50` piste (0 = case de départ) · `51..55` couloir · `56` arrivée.
Case absolue sur la piste : `(siège × 13 + position) mod 52`.

---

## 📅 Calendrier des tâches

| Séance | Jalon | Tâches de l'équipe | Statut |
|:---:|---|---|:---:|
| **1** | Vision produit | Sujet, fiche de vision, présentation de l'équipe, dépôt Git | ✅ |
| 2–3 | Algorithmes | Pseudo-code : dé, tour de jeu, règles de sortie | ✅ (`docs/algorithmes.md`) |
| **4** | Convention pseudo-code | Algorithmes : déplacement, capture, victoire | ✅ |
| **5** | Premier algorithme en C | Dé, tour simple | ✅ |
| 6–7 | C structuré | Structures `Joueur`/`Partie`, découpage en modules, Makefile | ✅ |
| **8** | Module C fonctionnel | `regles` + `plateau` testés | ✅ |
| 9–11 | Structures de données | Pions, captures, tours | ✅ |
| **12** | Structures linéaires | File des tours, pile d'historique | ✅ |
| 13–15 | Structures avancées | Classement, sauvegarde/chargement | ✅ |
| **16** | Structures avancées | Intégration complète, IA, victoire | ✅ |
| 17–19 | Finalisation | Tests, correction, documentation | 🔄 relecture par l'équipe |
| **20** | **Démo finale** | Présentation d'une partie complète | ⬜ |

---

## 🌿 Organisation Git

- `main` : version stable, toujours compilable (`make test` doit passer)
- une branche par fonctionnalité : `feature/ia`, `feature/sauvegarde`…
- commits clairs : `feat: ...`, `fix: ...`, `docs: ...`, `test: ...`
- intégration par **Pull Request** relue par le chef d'équipe

## 📜 Licence

Projet pédagogique – ISESTMA 2026-2027.
