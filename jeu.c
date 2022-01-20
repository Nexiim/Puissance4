/*
	Canvas pour algorithmes de jeux à 2 joueurs
	
	joueur 0 : humain
	joueur 1 : ordinateur
			
*/
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

// Paramètres du jeu
#define LARGEUR_MAX 7        // nb max de fils pour un noeud (= nb max de coups possibles)

#define TEMPS 15        // temps de calcul pour un coup avec MCTS (en secondes)

// macros
#define AUTRE_JOUEUR(i) (1-(i))
#define min(a, b)       ((a) < (b) ? (a) : (b))
#define max(a, b)       ((a) < (b) ? (b) : (a))

#define NBCOL 7
#define NBLIG 6
#define PUISSANCE 4

// Critères de fin de partie
typedef enum {
    NON, MATCHNUL, ORDI_GAGNE, HUMAIN_GAGNE
} FinDePartie;

// Definition du type Etat (état/position du jeu)
typedef struct EtatSt {

    int joueur; // à qui de jouer ?

    // TODO: à compléter par la définition de l'état du jeu

    /* par exemple, pour morpion: */
    char plateau[NBLIG][NBCOL];

} Etat;

// Definition du type Coup
typedef struct {
    int colonne;

} Coup;

// Copier un état 
Etat *copieEtat(Etat *src) {
    Etat *etat = (Etat *) malloc(sizeof(Etat));
    etat->joueur = src->joueur;

    // TODO: à compléter avec la copie de l'état src dans etat

    int i, j;
    for (i = 0; i < NBLIG; i++)
        for (j = 0; j < NBCOL; j++)
            etat->plateau[i][j] = src->plateau[i][j];

    return etat;
}

// Etat initial 
Etat *etat_initial(void) {
    Etat *etat = (Etat *) malloc(sizeof(Etat));

    int i, j;
    for (i = 0; i < NBLIG; i++)
        for (j = 0; j < NBCOL; j++)
            etat->plateau[i][j] = ' ';

    return etat;
}

void afficheJeu(Etat *etat) {
    int i, j;
    printf("   |");
    for (j = 0; j < NBCOL; j++)
        printf(" %d |", j);
    printf("\n");
    printf("--------------------------------");
    printf("\n");

    for (i = 0; i < NBLIG; i++) {
        printf(" %d |", i);
        for (j = 0; j < NBCOL; j++)
            printf(" %c |", etat->plateau[i][j]);
        printf("\n");
        printf("--------------------------------");
        printf("\n");
    }
    printf("\n");
}

// Nouveau coup
Coup *nouveauCoup(int i) {
    Coup *coup = (Coup *) malloc(sizeof(Coup));

    coup->colonne = i;

    return coup;
}

// Demander à l'humain quel coup jouer 
Coup *demanderCoup() {

    int i;
    printf(" quelle colonne ? ");
    scanf("%d", &i);

    return nouveauCoup(i);
}

// Modifier l'état en jouant un coup 
// retourne 0 si le coup n'est pas possible
int jouerCoup(Etat *etat, Coup *coup) {

    for (int i = NBLIG - 1; i >= 0; i--) {
        if (etat->plateau[i][coup->colonne] == ' ') {
            etat->plateau[i][coup->colonne] = etat->joueur ? 'O' : 'X';
            etat->joueur = AUTRE_JOUEUR(etat->joueur);
            return 1;
        }
    }
    return 0;
}

// Retourne une liste de coups possibles à partir d'un etat 
// (tableau de pointeurs de coups se terminant par NULL)
Coup **coups_possibles(Etat *etat) {

    Coup **coups = (Coup **) malloc((LARGEUR_MAX) * sizeof(Coup *));

    int k = 0;

    for (int i = 0; i < NBCOL; i++) {
        if (etat->plateau[0][i] == ' ') {
            coups[k] = nouveauCoup(i);
            k++;
        }
    }

    coups[k] = NULL;

    return coups;
}

// Definition du type Noeud 
typedef struct NoeudSt {

    int joueur; // joueur qui a joué pour arriver ici
    Coup *coup;   // coup joué par ce joueur pour arriver ici

    Etat *etat; // etat du jeu

    struct NoeudSt *parent;
    struct NoeudSt *enfants[LARGEUR_MAX]; // liste d'enfants : chaque enfant correspond à un coup possible
    int nb_enfants;    // nb d'enfants présents dans la liste

    int nb_victoires;
    int nb_simus;

} Noeud;

// Créer un nouveau noeud en jouant un coup à partir d'un parent 
// utiliser nouveauNoeud(NULL, NULL) pour créer la racine
Noeud *nouveauNoeud(Noeud *parent, Coup *coup) {
    Noeud *noeud = (Noeud *) malloc(sizeof(Noeud));

    if (parent != NULL && coup != NULL) {
        noeud->etat = copieEtat(parent->etat);
        jouerCoup(noeud->etat, coup);
        noeud->coup = coup;
        noeud->joueur = AUTRE_JOUEUR(parent->joueur);
    } else {

        noeud->etat = NULL;
        noeud->coup = NULL;
        noeud->joueur = 0;
    }
    for (int i = 0; i < LARGEUR_MAX; i++)
        noeud->enfants[i] = NULL;
    noeud->parent = parent;
    noeud->nb_enfants = 0;

    // POUR MCTS:
    noeud->nb_victoires = 0;
    noeud->nb_simus = 0;


    return noeud;
}

// Ajouter un enfant à un parent en jouant un coup
// retourne le pointeur sur l'enfant ajouté
Noeud *ajouterEnfant(Noeud *parent, Coup *coup) {
    Noeud *enfant = nouveauNoeud(parent, coup);
    parent->enfants[parent->nb_enfants] = enfant;
    parent->nb_enfants++;
    return enfant;
}

void freeNoeud(Noeud *noeud) {
    if (noeud->etat != NULL)
        free(noeud->etat);

    while (noeud->nb_enfants > 0) {
        freeNoeud(noeud->enfants[noeud->nb_enfants - 1]);
        noeud->nb_enfants--;
    }
    if (noeud->coup != NULL)
        free(noeud->coup);

    free(noeud);
}

// Test si l'état est un état terminal 
// et retourne NON, MATCHNUL, ORDI_GAGNE ou HUMAIN_GAGNE
FinDePartie testFin(Etat *etat) {
    // tester si un joueur a gagné
    int i, j, k, n = 0;
    for (int i = 0; i < NBLIG; i++) {
        for (int j = 0; j < NBCOL; j++) {
            if (etat->plateau[i][j] != ' ') {
                // nb coups joués
                n++;

                // lignes
                k = 0;
                while (k < PUISSANCE && i + k < NBLIG && etat->plateau[i + k][j] == etat->plateau[i][j])
                    k++;
                if (k == PUISSANCE)
                    return etat->plateau[i][j] == 'O' ? ORDI_GAGNE : HUMAIN_GAGNE;

                // colonnes
                k = 0;
                while (k < PUISSANCE && j + k < NBCOL && etat->plateau[i][j + k] == etat->plateau[i][j])
                    k++;
                if (k == PUISSANCE)
                    return etat->plateau[i][j] == 'O' ? ORDI_GAGNE : HUMAIN_GAGNE;

                // diagonales
                k = 0;
                while (k < PUISSANCE && i + k < NBLIG && j + k < NBCOL &&
                       etat->plateau[i + k][j + k] == etat->plateau[i][j])
                    k++;
                if (k == PUISSANCE)
                    return etat->plateau[i][j] == 'O' ? ORDI_GAGNE : HUMAIN_GAGNE;

                k = 0;
                while (k < PUISSANCE && i + k < NBLIG && j - k >= 0 &&
                       etat->plateau[i + k][j - k] == etat->plateau[i][j])
                    k++;
                if (k == PUISSANCE)
                    return etat->plateau[i][j] == 'O' ? ORDI_GAGNE : HUMAIN_GAGNE;
            }
        }
    }

    // et sinon tester le match nul
    if (n == NBCOL * NBLIG)
        return MATCHNUL;

    return NON;
}

//Simule à partir d'un état donné, à l'aide d'une marche aléatoire
//et retourne MATCHNUL, ORDI_GAGNE ou HUMAIN_GAGNE
FinDePartie simuler(Etat *etat) {
    FinDePartie fin = testFin(etat);
    Coup **coups;
    Coup *meilleur_coup;
    int k, random;

    while (fin == NON) {
        coups = coups_possibles(etat);
        k = 0;
        while (coups[k] != NULL) {
            k++;
        }

        random = rand() % k;
        meilleur_coup = coups[random]; // choix aléatoire
        jouerCoup(etat, meilleur_coup);
        fin = testFin(etat);

        printf("%i\n",k);
        for (int i = 0; i < k; i++) {
            printf("%i\n",coups[i]->colonne);
            free(coups[i]);
            printf("%i\n",i);
        }
        printf("%i\n",k);

        free(coups);

    }
    return fin;
}

//Simule nb_simulation à partir d'un état donné
// et retourne le nombre de partie gagner par le joueur qui joue l'état
float simulation(int nb_simulation, Noeud *noeud) {
    float nb_victoire = 0;
    FinDePartie joueur_etudie;
    if (noeud->joueur == 1) {
        joueur_etudie = ORDI_GAGNE;
    } else
        joueur_etudie = HUMAIN_GAGNE;

    for (int i = 0; i < nb_simulation; i++) {
        Etat *copie = copieEtat(noeud->etat);
        if (simuler(copie) == joueur_etudie) {
            nb_victoire++;
        }
        free(copie);
    }
    return nb_victoire / nb_simulation;
}

Noeud *selectionner_noeud(Noeud *parent) {
    float B;
    double maximum = INT_MIN;
    int enfant = 0;
    for (int i = 0; i < LARGEUR_MAX; i++) {
        if (parent->enfants[i] == NULL) {
            parent->enfants[i] = ajouterEnfant(parent, nouveauCoup(i));
            return parent->enfants[i];
        } else {
            //printf("non null\n");
            //afficheJeu(parent->enfants[i]->etat);
            //printf("fin %i\n",parent->enfants[i]->nb_simus);
            if (parent->joueur == 1)
                B = (parent->enfants[i]->nb_victoires / parent->enfants[i]->nb_simus) +
                    1.414 * (sqrt(log(parent->nb_simus) / parent->enfants[i]->nb_simus));
            else
                B = -((parent->enfants[i]->nb_victoires / parent->enfants[i]->nb_simus) +
                      1.414 * (sqrt(log(parent->nb_simus) / parent->enfants[i]->nb_simus)));
            if (B > maximum) {
                maximum = B;
                enfant = i;
            }

        }
    }
    //afficheJeu(meilleur_enfant->etat);
    return selectionner_noeud(parent->enfants[enfant]);
}


void mise_a_jour(Noeud *racine, Noeud *choisi, FinDePartie fin) {
    printf("test\n");
    while (choisi != racine) {
        printf("test\n");
        choisi->nb_simus++;
        if (fin == ORDI_GAGNE) {
            choisi->nb_victoires++;
        }
        choisi = choisi->parent;
    }
}

Coup *attendue(Noeud *noeud) {
    int enfant = 0;
    int max = 0;
    for (int i = 0; i < noeud->nb_enfants; i++) {
        /*printf("enfant %i victoire %i simus %i\n", i, noeud->enfants[i]->nb_victoires, noeud->enfants[i]->nb_simus);
        afficheJeu(noeud->enfants[i]->etat);*/
        if (max <= noeud->enfants[i]->nb_victoires / noeud->enfants[i]->nb_simus) {
            max = noeud->enfants[i]->nb_victoires / noeud->enfants[i]->nb_simus;
            enfant = i;
        }
    }

    return noeud->enfants[enfant]->coup;
}

// Calcule et joue un coup de l'ordinateur avec MCTS-UCT
// en tempsmax secondes
void ordijoue_mcts(Etat *etat, int tempsmax) {

    clock_t tic, toc;
    tic = clock();
    int temps;

    // Créer l'arbre de recherche
    Noeud *racine = nouveauNoeud(NULL, NULL);
    racine->etat = copieEtat(etat);


    int iter = 0;
    simuler(racine->etat);
    afficheJeu(racine->etat);

    do {
        Noeud *n = selectionner_noeud(racine);
        afficheJeu(n->etat);

        Etat *copie = copieEtat(racine->etat);

        mise_a_jour(racine, n, simuler(copie));


        toc = clock();
        temps = (int) (((double) (toc - tic)) / CLOCKS_PER_SEC);
        iter++;
    } while (iter < 1);//temps < tempsmax);

    printf("nb iteration : %i\n", iter);
    //printf("nb simus %i\n", racine->nb_simus);
    /* fin de l'algorithme*/

    // Jouer le meilleur premier coup
    jouerCoup(etat, attendue(racine));

    // Penser à libérer la mémoire :
    freeNoeud(racine);
}

int main(void) {

    Coup *coup;
    FinDePartie fin;
    srand(time(NULL));

    // initialisation
    Etat *etat = etat_initial();

    // Choisir qui commence :
    printf("Qui commence (0 : humain, 1 : ordinateur) ? ");
    scanf("%d", &(etat->joueur));

    // boucle de jeu
    do {
        printf("\n");
        afficheJeu(etat);

        if (etat->joueur == 0) {
            // tour de l'humain

            do {
                coup = demanderCoup();
            } while (!jouerCoup(etat, coup));

        } else {
            // tour de l'Ordinateur

            ordijoue_mcts(etat, TEMPS);

        }

        fin = testFin(etat);
    } while (fin == NON);

    printf("\n");
    afficheJeu(etat);

    if (fin == ORDI_GAGNE)
        printf("** L'ordinateur a gagné **\n");
    else if (fin == MATCHNUL)
        printf(" Match nul !  \n");
    else
        printf("** BRAVO, l'ordinateur a perdu  **\n");
    return 0;
}
