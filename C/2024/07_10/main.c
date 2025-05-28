/* Programma C main.c che risolve la parte C della prova di esame totale tenutasi il 10 luglio 2024 */

#include <stdlib.h> 
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/wait.h> 
#include <string.h> 
#include <ctype.h>
#include <signal.h> 
#include <time.h>

/* Definizione dei bit dei permessi (in ottale) per gli eventuali file creati */
#define PERM 0644 	

/* Definizione del TIPO pipe_t che rappresenta un array di due interi, che useremo per la gestione dei lati delle varie pipe create */
typedef int pipe_t[2];

/* Definizione di una funzione che calcoli un valore randomico, compreso tra 1 e n */
int mia_random(int n)
{ 
	int casuale; 
	casuale = rand() % n; 
	casuale++; 
	return casuale;
}

/* Definizone de tipo tipoL che rappresenta un array di 250 caratteri, usato come buffer di memoria per le singole linee lette da file. 
	Si e' scelto il valore 250 dal momento che il testo specifica che le linee abbiano lunghezza strettamenete inferiore a 250 caratteri */
typedef char tipoL[250];

int main (int argc, char **argv)
{
	/*---------------- Variabili locali -----------------*/
	int pid; 			/* identificatore dei processi figli creati con la fork e attesi con la wait */
	int N; 				/* numero di file passati per parametro */
	int L; 				/* lunghezza in linee del file associati ai singoli processi */
	int n; 				/* indice dei processi figli */
	int k; 				/* indice generico per i cicli */
	pipe_t *piped;		/* array dinamico di pipe che viene utilizzato per creare uno schema di comunicazione a PIPELINE: 
							- il primo processo P0 comunica con P1, che a sua volta comunica con P2, etc... fino a PN-1 che comunica col processo padre 
							- il generico processo figlio Pn (con n != 0) dunque legge dalla pipe di indice n - 1 e scrive su quella di indice n 
							- il processo figlio P0 essendo il primo non legge da nessuna pipe, si limita a scrivere sulla pipe di indice 0 */
	tipoL linea; 		/* buffer di 250 caratteri atto alla lettura delle singole linee da parte dei figli */
	tipoL *tutteLinee; 	/* array dinamico di array di caratteri che viene mandato avanti nella pipeline e viene aggiornato da ogni processo 
							figlio con la prorpia linea da cercare */
	int fdr; 			/* file descriptor associato all'apertura dei file associati ai vari figli */
	int r; 				/* valore randomico generato da ogni figlio che corrisponde all'indice della linea da cercare */
	int rows; 			/* variabile contatore delle linee lette da file nei vari processi figli */
	int status; 		/* variabile di stato per la wait */
	int ritorno; 		/* valore ritornato dai processi figli */
	int nr, nw; 		/* variabili di controllo per lettura/scrittura da/su pipe */
	/*---------------------------------------------------*/
	
	/* ciao eddu!*/

	/* Ciclo di attesa dei processi figli con recupero e stampa del valore tornato */

	exit(0); 
}
