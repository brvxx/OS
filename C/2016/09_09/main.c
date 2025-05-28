/* Programma C main.c relativo alla parte C della prova totale tenutasi il 09 settembre 2016*/

#include <stdlib.h> 
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/wait.h> 
#include <string.h> 

#define N 26	/* infatti per questo programma il numero di processi da creare e' fisso e non dipende dal numero di parametri passati, anche perche' questo ne 
		riceve esattamente 1. Quindi per conformarci alla notazione usate nei programmi precedenti andiamo a definire N come 26, ossia il numero 
		PREFISSATO di processi da creare, uno per singolo carattere dell'alfabeto inglese */
		

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
	int pid[N]; 		/* array statico che conterra' i pid dei processi figli creati */ 
	pipe_t piped[N]; 	/* array statico di N pipe che implementano uno schema di comunicazione a pipeline: 
				- lo scambio di informazioni va nel verso: dal processo di indice 0 al padre 
				- ogni processo di indice i legge dalla pipe di indice i - 1 e scrive sulla pipe di indice i */ 
	tipo_s data[N]; 	/* array statico per il passaggio di informazioni tra i processi della pipeline. Ogni processo scrive le 
				proprie informazioni dentro all'array letto dal processo precedente e passa l'array al processo successivo */ 
	int i, j; 		/* indice per i cicli */
	char Cx; 		/* carattere target da cercare nei singoli processi figli */
	char c; 		/* variabile atta alla lettura dei singoli carattere da file */
	long int occ; 		/* contatore di occorrenze di Cx di ogni figlio dentro al file */
	int fdr; 		/* file descriptor relativo all'apertura del file in ogni processo figlio */
	int pidFiglio; 		/* identificatore dei processi figli attesi dal padre */ 
	int status; 		/* variabile di stato della wait */ 
	int ritorno; 		/* valore ritornato dai processi figli */ 
	int nr, nw; 		/* variabili per il controllo dell'integrita' della pipeline */
	/*------------------------------------------------*/



	/* controllo stretto del numero di parametri passato: 1 filename */ 
	if (argc != 2) 
	{ 
		printf("Errore: numero di parametri passato errato - Attesi 1 - passati %d\n", argc - 1); 
		exit(1); 
	} 

	/* CREAZIONE DELLE N PIPE CHE IMPLEMENTANO LA PIPELINE */ 
	for (i = 0; i < N; i++)
	{ 
		if (pipe(piped[i]) < 0) 
		{ 
			printf("Errore: creazione della pipe %d non riuscita\n", i); 
			exit(2); 
		} 
	} 

	/* ciclo di creazione dei processi figli */
	for (i = 0; i < N; i++) 
	{ 
		if ((pid[i] = fork()) < 0) 
		{
			printf("Errore: creazione del processo figlio di indice %d non riuscita\n", i); 
			exit(3); 
		}
		
		if (pid[i] == 0) 
		{ 
			/* CODICE FIGLI */ 
			
			/* in caso di errore gli N processi figli ritorneranno il valore -1 (255 senza segno), cosi' che il ritorno 
			non venga confuso con quello normale dei processi
			Si noti poi che non si potranno andare ad inizializzare i dati dello specifico processo nella rispettiva struttura 
			dell'array, perchè quando si va leggere l'array lo si va a leggere tutto, dunque tutte 26 le strutture, dunque gli 
			eventuali dati scritti verrebbero sovrascritti e quindi persi */ 
		
			/* inizializzazione del carattere target per l'i-esimo processo */ 
			Cx = 'a' + i; 

			printf("DEBUG-Sono il processo figlio con pid = %d e indice %d e sto per contare le occorrenze del "
				"carattere '%c' dal file %s\n", getpid(), i, Cx, argv[1]); 
			
			/* chiusura del: 
			- lato di lettura di TUTTE le pipe della pipeline, tranne quella di indice i - 1 
			- lato di scrittura di TUTTE le pipe della pipeline, tranne quella di indice i 
			
			il processo di indice 0 non avendo bisogno di leggere da nessuna pipe potra' chiudere 
			direttametne tutti i lati di lettura */ 

			for (j = 0; j < N; j++)
			{ 
				if ((i == 0) || (j != i - 1)) 
				{ 
					close(piped[j][0]); 
				}
				
				if (j != i) 
				{ 
					close(piped[j][1]); 
				} 
			} 
			
			/* tentiamo l'apertura del file passato per parametro. Si noti che viene aperto nei processi figli e non nel padre, questo 
			perche' se fosse aperto una sola volta nel padre tutti i figli condividerebbero il file pointer su questo, dunque non riuscirebbero 
			a leggere correttamente il file */ 
			if ((fdr = open(argv[1], O_RDONLY)) < 0) 
			{ 
				printf("Errore: il file %s non esiste oppure non e' leggibile\n", argv[1]); 
				exit(-1); 
			} 

			/* leggiamo il file carattere per carattere andando contestualmente a contare il numero di occorrenze aggiornando la variabile occ */
			occ = 0L; 
			while (read(fdr, &c, 1) > 0) 
			{ 
				if (c == Cx) 
				{ 
					occ++; 
				} 
			} 
			
			/* lettura dell'array inviato via pipeline solo se i != 0, infatti il primo processo non ha processi da cui leggere */ 
			if (i != 0) 
			{ 
				nr = read(piped[i - 1][0], data, sizeof(data));		/* possibile passare direttamente l'array perche' e' statico, sarebbe identico fare 
											N * sizeof(tipo_s) */ 

				/* controllo che il numero di byte letti sia quello effettivo di cui e' composto data */ 
				if (nr != sizeof(data)) 
				{ 
					printf("Errore: il processo figlio di indice %d ha letto un numero errato di byte dalla pipeline\n", i); 
					exit(-1); 
				} 
			} 

			/* inizializziamo la struttura propria dell'i-esimo processo dentro l'array */ 
			data[i].c = Cx; 
			data[i].occ = occ; 

			/* a prescindere dall'indice del processo, andiamo a stampare l'array ricevuto E AGGIORNATO al processo successivo tramite la pipeline */
			nw = write(piped[i][1], data, sizeof(data)); 

			/* controllo byte scritti */ 
			if (nw != sizeof(data))
			{ 
				printf("Errore: il processo figlio di indice %d ha scritto un numero errato di byte nella pipeline\n", i); 
				exit(-1); 
			}

			/* ritorno al padre dell'utlimo carattere letto da file */ 
			exit(c); 
		} 
	} /* fine for */

	/* CODICE PADRE */ 
	
	/* chisura lato di scrittura di TUTTE le pipe e di lettura di TUTTE tranne che l'ultima, quella che lo collega all'ultimo processo */
	for (i = 0; i < N; i++) 
	{ 	
		close(piped[i][1]); 

		if (i != N - 1) 
		{ 
			close(piped[i][0]);
		} 
	} 

	/* lettura dalla pipeline dell'array completamente inizializzato */ 
	nr = read(piped[N - 1][0], data, sizeof(data)); 

	if (nr != sizeof(data))
	{ 
		printf("Errore: il processo padre ha letto un numero errato di byte dalla pipeline: %d\n", nr); 
		/* exit(4); non usciamo perche' bisogna prima attendere i figli */
	} 
	else	/* padre ha letto correttamente dalla pipeline */ 
	{ 
		/* a questo punto il processo padre deve ORDINARE L'ARRAY in senso crescente in base al numero di occorrenze. Per l'ordinamento noi utilizziamo la funzione qsort 
		della libreria, per tale motivo abbiamo definito la funzione confronta_occ che DEFINISCE il criterio in base al quale ordinare gli elementi dell'array di tipo_s */
		qsort(data, N, sizeof(tipo_s), confronta_occ); 

		/* stampa delle informazioni contenute in ognuna delle struttre dell'array, assieme alla stampa dell'indice dei processi associati ad ognuna e il loro pid: 
		- per risalire all'indice dal carattre sara' sufficiente applicare il processo inverso usato per risalire dall'indice al carattere, dunque sottrare l'offset 'a'
		- per risalire al pid una volta ottenuto l'indice bastera' sfruttare l'array di pid inizializzato nel ciclo di creazione dei processi figli */ 
		for (i = 0; i < N; i++) 
		{ 
			printf("Il processo figlio di indice %d con pid = %d ha trovato %ld occorrenze del carattere '%c' nel " 
				"file %s passato\n", data[i].c - 'a', pid[data[i].c - 'a'], data[i].occ, data[i].c, argv[1]);
		} 
	}	

	/* attesa degli N processi figli con recupero dei valori tornati e stampa di questi */ 
	for (i = 0; i < N; i++) 
	{ 	
		if ((pidFiglio = wait(&status)) < 0) 
		{ 
			printf("Errore: wait non riuscita\n"); 
			exit(5); 
		}

		if (WIFEXITED(status))
		{ 
			ritorno = WEXITSTATUS(status); 
			printf("Il processo figlio con pid = %d e' terminato ritornando il valore '%c' (in decimale %d, se 255 errori!)\n", pidFiglio, ritorno, ritorno); 
		} 
		else
		{ 
			printf("Il processo figlio con pid = %d e' terminato in modo anomalo\n", pidFiglio); 
		}
	}

	exit(0); 
}

