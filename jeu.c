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
#define LARGEUR_MAX 8		// nb max de fils pour un noeud (= nb max de coups possibles)

#define TEMPS 5		// temps de calcul pour un coup avec MCTS (en secondes)

// macros
#define AUTRE_JOUEUR(i) (1-(i))
#define min(a, b)       ((a) < (b) ? (a) : (b))
#define max(a, b)       ((a) < (b) ? (b) : (a))

#define NBCOL 7
#define NBLIG 6
#define PUISSANCE 4

// Critères de fin de partie
typedef enum {NON, MATCHNUL, ORDI_GAGNE, HUMAIN_GAGNE } FinDePartie;

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
Etat * copieEtat( Etat * src ) {
    Etat * etat = (Etat *)malloc(sizeof(Etat));
    etat->joueur = src->joueur;

    // TODO: à compléter avec la copie de l'état src dans etat

    int i,j;
    for (i=0; i< NBLIG; i++)
        for ( j=0; j<NBCOL; j++)
            etat->plateau[i][j] = src->plateau[i][j];

    return etat;
}

// Etat initial 
Etat * etat_initial( void ) {
    Etat * etat = (Etat *)malloc(sizeof(Etat));

    int i,j;
    for (i=0; i< NBLIG; i++)
        for ( j=0; j<NBCOL; j++)
            etat->plateau[i][j] = ' ';

    return etat;
}

void afficheJeu(Etat * etat) {
    int i,j;
    printf("   |");
    for ( j = 0; j < NBCOL; j++)
        printf(" %d |", j);
    printf("\n");
    printf("--------------------------------");
    printf("\n");

    for(i=0; i < NBLIG; i++) {
        printf(" %d |", i);
        for ( j = 0; j < NBCOL; j++)
            printf(" %c |", etat->plateau[i][j]);
        printf("\n");
        printf("--------------------------------");
        printf("\n");
    }
    printf("\n");
}

// Nouveau coup
Coup * nouveauCoup( int i) {
    Coup * coup = (Coup *)malloc(sizeof(Coup));

    coup->colonne = i;

    return coup;
}

// Demander à l'humain quel coup jouer 
Coup * demanderCoup () {

    int i;
    printf(" quelle colonne ? ") ;
    scanf("%d",&i);

    return nouveauCoup(i);
}

// Modifier l'état en jouant un coup 
// retourne 0 si le coup n'est pas possible
int jouerCoup( Etat * etat, Coup * coup ) {

    for (int i =NBLIG-1; i>=0; i--){
        if (etat->plateau[i][coup->colonne] == ' '){
            etat->plateau[i][coup->colonne] = etat->joueur ? 'O' : 'X';
            etat->joueur = AUTRE_JOUEUR(etat->joueur);
            return 1;
        }
    }
    return 0;
}

// Retourne une liste de coups possibles à partir d'un etat 
// (tableau de pointeurs de coups se terminant par NULL)
Coup ** coups_possibles( Etat * etat ) {

    Coup ** coups = (Coup **) malloc((LARGEUR_MAX) * sizeof(Coup *) );

    int k = 0;

    for (int i =0; i<NBCOL;i++){
        if ( etat->plateau[0][i] == ' ' ) {
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
    Coup * coup;   // coup joué par ce joueur pour arriver ici

    Etat * etat; // etat du jeu

    struct NoeudSt * parent;
    struct NoeudSt * enfants[LARGEUR_MAX]; // liste d'enfants : chaque enfant correspond à un coup possible
    int nb_enfants;	// nb d'enfants présents dans la liste

    int nb_victoires;
    int nb_simus;

} Noeud;

// Créer un nouveau noeud en jouant un coup à partir d'un parent 
// utiliser nouveauNoeud(NULL, NULL) pour créer la racine
Noeud * nouveauNoeud (Noeud * parent, Coup * coup ) {
    Noeud * noeud = (Noeud *)malloc(sizeof(Noeud));

    if ( parent != NULL && coup != NULL ) {
        noeud->etat = copieEtat ( parent->etat );
        jouerCoup ( noeud->etat, coup );
        noeud->coup = coup;
        noeud->joueur = AUTRE_JOUEUR(parent->joueur);
    }
    else {
        noeud->etat = NULL;
        noeud->coup = NULL;
        noeud->joueur = 0;
    }
    noeud->parent = parent;
    noeud->nb_enfants = 0;

    // POUR MCTS:
    noeud->nb_victoires = 0;
    noeud->nb_simus = 0;


    return noeud;
}

// Ajouter un enfant à un parent en jouant un coup
// retourne le pointeur sur l'enfant ajouté
Noeud * ajouterEnfant(Noeud * parent, Coup * coup) {
    Noeud * enfant = nouveauNoeud (parent, coup ) ;
    parent->enfants[parent->nb_enfants] = enfant;
    parent->nb_enfants++;
    return enfant;
}

void freeNoeud ( Noeud * noeud) {
    if ( noeud->etat != NULL)
        free (noeud->etat);

    while ( noeud->nb_enfants > 0 ) {
        freeNoeud(noeud->enfants[noeud->nb_enfants-1]);
        noeud->nb_enfants --;
    }
    if ( noeud->coup != NULL)
        free(noeud->coup);

    free(noeud);
}

// Test si l'état est un état terminal 
// et retourne NON, MATCHNUL, ORDI_GAGNE ou HUMAIN_GAGNE
FinDePartie testFin( Etat * etat ) {
    // tester si un joueur a gagné
    int i,j,k,n = 0;
    for (int i =0;i<NBLIG;i++){
        for (int j =0;j<NBCOL;j++){
            if ( etat->plateau[i][j] != ' ') {
                // nb coups joués
                n++;

                // lignes
                k=0;
                while ( k < PUISSANCE && i+k < NBLIG && etat->plateau[i+k][j] == etat->plateau[i][j] )
                    k++;
                if ( k == PUISSANCE )
                    return etat->plateau[i][j] == 'O'? ORDI_GAGNE : HUMAIN_GAGNE;

                // colonnes
                k=0;
                while ( k < PUISSANCE && j+k < NBCOL && etat->plateau[i][j+k] == etat->plateau[i][j] )
                    k++;
                if ( k == PUISSANCE )
                    return etat->plateau[i][j] == 'O'? ORDI_GAGNE : HUMAIN_GAGNE;

                // diagonales
                k=0;
                while ( k < PUISSANCE && i+k < NBLIG && j+k < NBCOL && etat->plateau[i+k][j+k] == etat->plateau[i][j] )
                    k++;
                if ( k == PUISSANCE )
                    return etat->plateau[i][j] == 'O'? ORDI_GAGNE : HUMAIN_GAGNE;

                k=0;
                while ( k < PUISSANCE && i+k < NBLIG && j-k >= 0 && etat->plateau[i+k][j-k] == etat->plateau[i][j] )
                    k++;
                if ( k == PUISSANCE )
                    return etat->plateau[i][j] == 'O'? ORDI_GAGNE : HUMAIN_GAGNE;
            }
        }
    }

    // et sinon tester le match nul
    if ( n == NBCOL*NBLIG )
        return MATCHNUL;

    return NON;
}

//Simule à partir d'un état donné, à l'aide d'une marche aléatoire
//et retourne MATCHNUL, ORDI_GAGNE ou HUMAIN_GAGNE
FinDePartie simuler(Etat * etat) {
    FinDePartie fin = testFin(etat);
    Coup **coups;
    Coup *meilleur_coup;
    int k,random;
    while (fin == NON) {
        coups = coups_possibles(etat);
        k = 0;
        while (coups[k] != NULL) {
            k++;
        }
        random = rand() % k;
        meilleur_coup = coups[random]; // choix aléatoire
        jouerCoup(etat, meilleur_coup );
        fin = testFin(etat);

        for(int i = 0;i<k;i++){
            free(coups[i]);
        }
        free(coups);
        //free(meilleur_coup);
    }
    return fin;
}

//Simule nb_simulation à partir d'un état donné
// et retourne le nombre de partie gagner par le joueur qui joue l'état
float simulation(int nb_simulation,Noeud * noeud){
    float nb_victoire = 0;

    for(int i = 0;i<nb_simulation;i++){
        Etat * copie = copieEtat(noeud->etat);
        if (simuler(copie)== ORDI_GAGNE){
            nb_victoire++;
        }
        free(copie);
    }
    return nb_victoire/nb_simulation;
}

Noeud * selectionner_noeud (Noeud * parent){

    for (int i =0 ;i<LARGEUR_MAX-1;i++){
        if(parent->enfants[i] == NULL){
            return parent->enfants[i];
        }
        else {

            //printf("%i\n",parent->enfants[i]->nb_victoires);
            /*maximum = (parent->enfants[i]->nb_victoires / parent->enfants[i]->nb_simus) +
                      1.414 * (sqrt(log(parent->nb_simus) / parent->enfants[i]->nb_simus))*/;
        }
    }
    return parent;

}
// Calcule et joue un coup de l'ordinateur avec MCTS-UCT
// en tempsmax secondes
void ordijoue_mcts(Etat * etat, int tempsmax) {

    clock_t tic, toc;
    tic = clock();
    int temps;

    Coup ** coups;
    Coup * meilleur_coup ;

    // Créer l'arbre de recherche
    Noeud * racine = nouveauNoeud(NULL, NULL);
    racine->etat = copieEtat(etat);

    // créer les premiers noeuds:
    coups = coups_possibles(racine->etat);
    int k = 0;
    Noeud * enfant;
    while ( coups[k] != NULL) {
        enfant = ajouterEnfant(racine, coups[k]);
        k++;
    }

    int random =  rand()%k ;
    meilleur_coup = coups[ random ]; // choix aléatoire
    printf("colonne %i\n",random);

    //selectionner_noeud(racine);
    tic = clock();
    printf("%f\n",simulation(1000,racine));
    toc = clock();
    printf("temps %f\n",( ((double) (toc - tic)) / CLOCKS_PER_SEC ));


    simuler(etat);
    afficheJeu(etat);

    /*TODO :
      - supprimer la sélection aléatoire du meilleur coup ci-dessus
      - implémenter l'algorithme MCTS-UCT pour déterminer le meilleur coup ci-dessous*/

    /*int iter = 0;

    do {

        // selectionner




        // à compléter par l'algorithme MCTS-UCT...




        toc = clock();
        temps = (int)( ((double) (toc - tic)) / CLOCKS_PER_SEC );
        iter ++;
    } while ( temps < tempsmax );*/

    /* fin de l'algorithme*/

    // Jouer le meilleur premier coup
    //jouerCoup(etat, meilleur_coup );

    // Penser à libérer la mémoire :
    freeNoeud(racine);
    free (coups);
}

int main(void) {

    Coup * coup;
    FinDePartie fin;
    srand(time(NULL));

    // initialisation
    Etat * etat = etat_initial();

    // Choisir qui commence :
    printf("Qui commence (0 : humain, 1 : ordinateur) ? ");
    scanf("%d", &(etat->joueur) );

    // boucle de jeu
    do {
        printf("\n");
        afficheJeu(etat);

        if ( etat->joueur == 0 ) {
            // tour de l'humain

            do {
                coup = demanderCoup();
            } while ( !jouerCoup(etat, coup) );

        }
        else {
            // tour de l'Ordinateur

            ordijoue_mcts( etat, TEMPS );

        }

        fin = testFin( etat );
    }	while ( fin == NON ) ;

    printf("\n");
    afficheJeu(etat);

    if ( fin == ORDI_GAGNE )
        printf( "** L'ordinateur a gagné **\n");
    else if ( fin == MATCHNUL )
        printf(" Match nul !  \n");
    else
        printf( "** BRAVO, l'ordinateur a perdu  **\n");
    return 0;
}
