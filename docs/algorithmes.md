# Algorithmes en pseudo-code

Convention : `←` affectation · `=` test d'égalité · `POUR … FAIRE … FIN POUR` · `SI … ALORS … SINON … FIN SI` · `TANT QUE … FAIRE … FIN TANT QUE`.

Position d'un pion : `-1` base · `0..50` piste · `51..55` couloir · `56` arrivée.

## 1. Case absolue

```
FONCTION caseAbsolue(siege, pos) : entier
    SI pos < 0 OU pos > 50 ALORS RETOURNER -1
    RETOURNER (siege * 13 + pos) MOD 52
FIN FONCTION
```

## 2. Case sûre

```
FONCTION caseSure(abs) : booléen
    SI abs < 0 ALORS RETOURNER FAUX
    RETOURNER (abs MOD 13 = 0) OU (abs MOD 13 = 8)
FIN FONCTION
```

## 3. Coups possibles pour un dé

```
FONCTION coupsPossibles(partie, j, de) : liste de coups
    liste ← vide
    POUR chaque pion k du joueur j FAIRE
        pos ← position(j, k)
        SI pos = 56 OU un pion de même position a déjà été traité ALORS continuer
        SI pos = -1 ALORS
            SI de ≠ 6 ALORS continuer
            dest ← 0
        SINON
            dest ← pos + de
            SI dest > 56 ALORS continuer          // nombre exact requis
        FIN SI
        abs ← caseAbsolue(siege(j), dest)
        SI abs ≥ 0 ET NON caseSure(abs) ALORS
            SI un adversaire a ≥ 2 pions sur abs ALORS continuer   // tour
            SI un adversaire a 1 pion sur abs ALORS marquer capture
        FIN SI
        ajouter le coup (k, pos, dest) à liste
    FIN POUR
    RETOURNER liste
FIN FONCTION
```

## 4. Appliquer un coup

```
PROCÉDURE appliquerCoup(partie, j, coup)
    position(j, coup.pion) ← coup.arrivee
    SI coup.capture ALORS position(victime) ← -1
    SI les 4 pions de j sont à 56 ALORS
        partie.nbClasses ← partie.nbClasses + 1
        rang(j) ← partie.nbClasses
    FIN SI
FIN PROCÉDURE
```

## 5. Tour d'un joueur

```
PROCÉDURE jouerTour(partie)
    j ← tête de la file des tours
    six ← 0
    RÉPÉTER
        rejouer ← FAUX
        de ← lancerDe()
        SI de = 6 ALORS six ← six + 1 SINON six ← 0
        SI six = 3 ALORS enregistrer(« trois six ») ; SORTIR
        liste ← coupsPossibles(partie, j, de)
        SI liste est vide ALORS
            rejouer ← (de = 6)
        SINON
            SI taille(liste) = 1 ALORS coup ← liste[0]
            SINON SI j est humain ALORS coup ← choix du joueur
            SINON coup ← meilleurCoupIA(liste)
            appliquerCoup(partie, j, coup)
            empiler(historique, événement)
            SI j a gagné ALORS SORTIR
            rejouer ← (de = 6) OU coup.capture OU coup.arriveMaison
        FIN SI
    JUSQU'À rejouer = FAUX
    défiler(file)
    SI j n'est pas classé ALORS enfiler(file, j)
FIN PROCÉDURE
```

## 6. Boucle principale

```
PROCÉDURE jouerPartie(partie)
    TANT QUE NON partieTerminée(partie) FAIRE
        jouerTour(partie)
    FIN TANT QUE
    finaliserClassement(partie)
    afficher(classement)
FIN PROCÉDURE
```

## 7. Choix de l'ordinateur (heuristique)

```
FONCTION score(coup) : entier
    s ← 0
    SI arrive à la maison     ALORS s ← s + 100
    SI capture                ALORS s ← s + 80
    SI sortie de base         ALORS s ← s + 50
    SI entre dans le couloir  ALORS s ← s + 40
    SI arrive sur case sûre   ALORS s ← s + 20
    SINON s ← s - 15 × nbMenaces(case d'arrivée)
    s ← s + 12 × nbMenaces(case de départ)       // fuir le danger
    s ← s + arrivee / 4
    RETOURNER s
FIN FONCTION
→ l'ordinateur joue le coup de score maximal.
```

## 8. Machine à états de l'interface graphique

```
ST_LANCER  --clic sur le dé / Espace (ou délai pour l'ordinateur)--> ST_DE
ST_DE      --animation 0,75 s--> traiter le dé :
              trois 6 de suite        -> ST_PAUSE -> fin du tour
              aucun coup possible     -> ST_PAUSE -> (relancer si 6) sinon fin du tour
              un seul coup            -> ST_PAUSE -> animation du pion
              plusieurs coups         -> humain : ST_CHOIX   |  ordinateur : meilleur coup
ST_CHOIX   --clic sur un pion lumineux--> ST_ANIM
ST_ANIM    --pion avance case par case--> appliquer le coup --> ST_PAUSE
ST_PAUSE   --> relancer (6, capture, arrivée) | fin du tour
fin du tour: défiler la file, ré-enfiler le joueur s'il n'est pas classé,
             tester la fin de partie -> ST_FIN (classement) sinon ST_LANCER
```
