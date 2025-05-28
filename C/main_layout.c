/* Programma C main.c che risolve la parte C della prova di esame */

#include <stdlib.h> 
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/wait.h> 
#include <string.h> 
#include <ctype.h>
#include <signal.h> 

/* Definizione dei bit dei permessi (in ottale) per gli eventuali file creati */
#define PERM 0644 	

/* Definizione del TIPO pipe_t che rappresenta un array di due interi, che useremo per la gestione dei lati delle varie pipe create */
typedef int pipe_t[2];

int main (int argc, char **argv)
{
	/*---------------- Variabili locali -----------------*/
	
	int k; 				/* indice generico per i cicli */
	int status; 		/* variabile di stato per la wait */
	int ritorno; 		/* valore ritornato dai processi figli */
	int nr, nw; 		/* variabili di controllo per lettura/scrittura da/su pipe */
	/*---------------------------------------------------*/
	

	/* Ciclo di attesa dei processi figli con recupero e stampa del valore tornato */

	exit(0); 
}
