/* Programma C main.c relativo alla parte C della prova totale tenutasi il 09 settembre 2016*/

#include <stdlib.h> 
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/wait.h> 
#include <string.h> 

/* definizione del TIPO pipe_t che rappresenta un array di due interi, che useremo per la gestione dei lati delle varie pipe create */
typedef int pipe_t[2]; 

/* definizione del TIPO tipo_s che rappresenta una particolare struttura che useremo per contenere le informazioni relative all'esecuzione dei ogni processo figlio */ 
typedef struct { 
	char c; 	/* carattere associato al processo */
	long int occ; 	/* numero di occorrenze trovate di c dentro al file passato per parametro */
} tipo_s; 

/* definiamo la funzione che definisce il modo in cui confrontare gli elementi dell'array da ordinare, nel nosto caso si tratta di struct tipo_s */
int confronta_occ(const void *a, const void *b) 
{
	/* casting da void* a tipo_s*, puntatori al tipo di dato che effettivamente stiamo confrontando */
	const tipo_s *s1 = a; 	
	const tipo_s *s2 = b; 

	/* adesso dobbiamo fare in modo che la funzione ritorni: 
	- un valore negativo se a < b
	- un valore positivo se a > b 
	- il valore nullo se a == b

	il qsort quando va a confrontare due elementi dell'array si basa su queste informazioni per interpretare il valore tornato 
	da questa funzione ed andare ad ordinare l'array IN ORDINE CRESCENTE */ 

	return (int)(s1->occ - s2->occ); 
}

int main (int argc, char **argv)
{
	/*--------------- Variabili locali ---------------*/
	
	/*------------------------------------------------*/
	
	tipo_s v[4] = {
		{'a', 30L}, 
		{'b', 10L},
		{'c', 3L}, 
		{'d', 50L}
	}; 

	qsort(v, 4, sizeof(tipo_s), confronta_occ); 

	for (int i = 0; i < 4; i++) 
	{ 
		printf("l'elemento %d del vettore contiene il carattere '%c' e il numero di occorrenze %ld\n", i, v[i].c, v[i].occ); 
	} 

	exit(0); 
}

