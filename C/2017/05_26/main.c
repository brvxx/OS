/* programma C main.c della seconda prova in itinere tenutasi il giorno 26 maggio 2017 */ 

/* breve descrizione del programma: 
Il programma accetta N + 1 parametri, dove gli N sono nomi di file, con N >= 2 e l'ultimo parametro e' un singolo carattere. Quello che fa il programma mediante l'utilizzo di N processi figli e' andare a determinare in quale file di quelli passati ci sia il MASSIMO numero di occorrenze del carattere anch'esso passato per parametro. Finita la ricerca che avviene attraverso uno schema di comunicazione a pipeline sara' il processo padre ad andare a stampare in output i risultati ottenuti */ 

#include <stdlib.h> 
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/wait.h> 
#include <string.h> 

/* definzione del TIPO pipe_t come array di due interi, ci servira' per poter gesitre facilmente le N pipe */
typedef int pipe_t[2]; 

/* definzione det TIPO tipo_s come una particolare struttura dati, che servira' per il passaggio di informazioni attraverso le pipe */
typedef struct { 
	long int occ; 	/* numero massimo di occorrenze */
	int id;		/* indice del figlio che nel suo file ha trovato il numero max di occorrenze */ 
	long int sum; 	/* somma di tutte le occorrenze nei vari file */
} tipo_s; 
	

int main (int argc, char **argv)
{ 
	/*---------------- Variabili Locali -----------------*/
	int N; 			/* numero di file passato per parametro, dunque numero di processi figli creati */ 
	char Cx; 		/* carattere passato per parametro da cercare */ 
	int *pid; 		/* array dinamico di pid ritornati dall'utilizzo delle varie fork di creazione dei processi figli. Questo serve al padre per poter risalire 
				   al pid del figlio tramite il suo indice per poterlo poi stampare in output */ 
	pipe_t *piped; 		/* array dinamico di pipe che permette di implementare lo schema di comunicazione a pipeline a partire dal primo figlio all'ultimo figlio e infine 
				   al padre: 
				   Ogni processo di indice i (a parte quando i == 0) legge dalla pipe i - 1 dell'array e SCRIVE sulla pipe i */ 
	int i, j;		/* indici per i cicli */ 
	int fdr; 		/* file descriptor per l'apertura in lettura dei vari file da parte dei processi figlio */
	long int occ; 		/* numero di occorrenze lette dal singolo processo nel rispettivo file */
	char c; 		/* carattere usato per la lettura dei file */		
	tipo_s msg; 		/* struct utilizzata dai figli e dal padre per la comunicazione */ 
	int pidFiglio; 		/* identificatore dei processi figli attesi tramite la wait */
	int status; 		/* variabile di stato per la wait */ 
	int ritorno; 		/* variabile per recuperare il valore di ritorno dei processi figlio */ 
	int nr, nw; 		/* variabili per il controllo che la lettura e scrittura sulla pipeline abbia avuto successo. Necessari nel caso della pipeline */ 
	/*---------------------------------------------------*/


	/* controllo lasco numero di parametri passato */ 
	if (argc < 4) 
	{ 
		printf("Errore: numero di parametri passato errato - Attesi almeno 3 (due nomi di file e un carattere) - Passati %d\n", argc - 1); 
		exit(1); 
	} 

	N = argc - 2; 	/* numero di file ottenuto sottraendo al numero di parametri 2 (nome dell'eseguibile e il carattere passato per parametro) */ 

	/* cotrollo che l'ultimo carattere sia un singolo carattere */ 
	if (strlen(argv[argc - 1]) != 1) 
	{ 
		printf("Errore: l'ultimo parametro fornito %s NON e' un singolo carattere\n", argv[argc - 1]); 
		exit(2); 
	} 

	Cx = argv[argc - 1][0];	/* estraiamo il carattere dalla stringa */ 


	/* allocazione dinamica della memoria necessaria all'array di pid */ 
	if ((pid = (int *)malloc(N * sizeof(int))) == NULL) 
	{ 
		printf("Errore: non e' stato possibile allocare l'array di pid con la malloc\n"); 
		exit(3); 
	} 

	/* allocazione dinamica della memroia necessaria all'array di pipe */ 
	if ((piped = (pipe_t *)malloc(N * sizeof(pipe_t))) == NULL) 
	{ 
		printf("Errore: non e' stato possibile allocare l'array di pipe con la malloc\n"); 
		exit(4); 
	} 

	/* creazione delle N pipe utilizzate nello schema a pipeline */ 
	for (i = 0; i < N; i++) 
	{ 
		if (pipe(piped[i]) < 0) 
		{ 
			printf("Errore: non e' stato possibile creare la %d-esima pipe\n", i); 
			exit(5); 
		} 
	} 


	/* creazione degli N processi figli */ 
	for (i = 0; i < N; i++)
	{ 
		if ((pid[i] = fork()) < 0) 
		{ 
			printf("Errore: non e' stata possibile la creazione del %d-esimo figlio con la fork\n", i); 
			exit(6); 
		} 

		if (pid[i] == 0)
		{ 
			/* CODICE DEL PROCESSO FIGLIO */ 
			
			/* dal momento che da specifica i processi figli dovranno ritornare il loro indice, supponendo che N non sia troppo elevato 
			e' valido pensare di poter ritornare valori negativi via via decrescenti per gli errori del figlio, che rappresenteranno
			valori elevati senza segno */ 

			printf("DEBUG-Sono il processo figlio con pid = %d di indice %d e sto per contare le occorrenze di %c nel file %s\n", getpid(), i, Cx, argv[i + 1]); 

			/* chisura dei lati delle pipe che non servono, in generale il figlio di indice i: 
			- chiude i lati di lettura di tutte le pipe TRANNE QUELLA DI INDICE i - 1. Il figlio con i = 0 invece chiudera' 
			  TUTTI i lati di lettura, non dovendosi comportare come consumatore per nessuno dei processi fratelli 
			- chiude i lati di scrittura di tutte le pipe tranne di quella di indice i su cui dovra' scrivere */ 

			for (j = 0; j < N; j++)
			{ 
				if ((j != i - 1) || (i == 0)) 
				{ 
					close(piped[j][0]);
				}

				if (j != i) 
				{ 
					close(piped[j][1]); 
				} 
			} 
			
			/* tentiamo l'apertura del file associato al i-esimo processo */ 
			if ((fdr = open(argv[i + 1], O_RDONLY)) < 0) 
			{ 
				printf("Errore: il file %s non esiste oppure non e' leggibile\n", argv[i + 1]); 
				exit(-1); 
			} 
			

			/* conteggio delle occorrenze del carattere Cx nel file associato all'i-esimo processo */ 
			occ = 0L;	/* inizializziamo a 0 la variabile contatore. L viene esplicitato perche' si usi 0 in forma long int */ 

			while (read(fdr, &c, 1) > 0)
			{ 
				if (c == Cx)
				{ 
					occ++; 
				} 
			} 

			/* lettura dalla pipeline della struttura spedita dal processo precedente ed eventuale aggiornamento dei parametri */ 
			if (i == 0) 
			{ 
				/* il processo di indice 0 non dovra' leggere da nessuna pipe, si limita ad inizializzare la struttura e 
				inserirla dentro la pipeline */ 
				
				msg.occ = occ; 
				msg.id = i; 
				msg.sum = occ; 
			} 
			else	/* tutti i processi che non sono di indice 0 */
			{ 
				/* lettura della struttura dalla pipeline */ 
				nr = read(piped[i - 1][0], &msg, sizeof(msg)); 

				/* controlliamo che siano stati effettivamamente letti tutti i byte, infatti nel caso della pipeline e' importante verificare 
				che questa non si sia rotta in corrispondenza di ogni processo, se no ne risentirebbero tutti i processi a valle e ovviamente 
				anche il padre */ 
				if (nr != sizeof(msg))
				{ 
					printf("Il processo figlio di indice %d ha letto un numero sbagliato di byte dalla pipeline: %d\n", i, nr); 
					exit(-2); 
				} 

				/* controlliamo se il numero di occorrenze occ sia per caso maggiore del numero massimo corrente trovato nei processi precedenti */
				if (occ > msg.occ) 
				{ 
					msg.occ = occ; 
					msg.id = i; 
				} 

				msg.sum += occ; 
			}

			/* a prescindere dall'indice del processo figlio questo deve mandare avanti la struttura nella pipeline */ 
			nw = write(piped[i][1], &msg, sizeof(msg)); 

			/* controllo che sia stato scritto sulla pipeline tutta la struttra */
			if (nw != sizeof(msg)) 
			{ 
				printf("Il processo figlio di indice %d ha scritto un numero sbagliato di byte nella pipeline: %d\n", i, nw); 
				exit(-3); 
			} 

			/* ritorno al padre dell'indice del processo */ 
			exit(i); 
		}
	} /* fine for */
	
	/* CODICE PROCESSO PADRE */ 

	/* chiusura dei lati di scrittura di tutte le pipe e di lettura di tutte le pipe tranne che per l'ultima */
	for (i = 0; i < N; i++) 
	{ 
		close(piped[i][1]); 

		if (i != N - 1)
		{ 
			close(piped[i][0]); 
		} 
	} 

	/* lettura della singola struttura proveniente dall'ultimo figlio */ 
	nr = read(piped[N-1][0], &msg, sizeof(msg));

	/* controllo che sia stata letta la struttura per intero anche per il padre */ 
	if (nr != sizeof(msg)) 
	{
		printf("Errore: il processo padre ha letto un numero sbagliato di byte dalla pipeline: %d\n", nr); 
		/* exit(7); sebbene sia un errore a tutti gli effetti, se terminassimo ora il padre questo non andrebbe ad attenere i figli che non e' cosa buona */
	} 
	else
	{ 
		/* stampa dei valori contenuti nella struttura uscita dalla pipeline */ 
		printf("Il numero massimo di occorrenze del carattere '%c' trovato nei vari file passati e': %ld\n", Cx, msg.occ);
		printf("Questo valore e' stato trovato dal processo con pid = %d di indice %d mediante la lettura del file %s\n", pid[msg.id], msg.id, argv[msg.id + 1]); 
		printf("Invece il numero totale di occorrenze trovate del carattere '%c' dentro a tutti i file e': %ld\n", Cx, msg.sum); 
	} 

	/* attesa dei vari process figli con recupero del valore tornato */
	for (i = 0; i < N; i++) 
	{ 
		if ((pidFiglio = wait(&status)) < 0)
		{ 
			printf("Errore: non e' stato possibile attendere uno dei processi figli con la wait\n"); 
			exit(8); 
		} 

		if (WIFEXITED(status))
		{ 
			ritorno = WEXITSTATUS(status); 
			printf("Il processo figlio con pid = %d ha ritornato il valore %d (se > di %d problemi!)\n", pidFiglio, ritorno, N - 1); 
		} 
		else
		{ 
			printf("Il processo figlio con pid = %d e' terminato in modo anomalo\n", pidFiglio); 
		} 
	}

	exit(0); 
}

/* piccola osservazione sul perche' sia IMPORTANTISSIMO IL CONTROLLO di nr e nw (soprattuto nr): si noti che l'effettiva scrittura del processo i-esimo sulla pipeline dipende 
dalla riuscita o meno dall'apertura del suo file, infatti questo se non riesce ad aprirlo termina senza toccare la pipeline, ma questo cosa significa?? 
Che NON SCRIVERA' NULLA SULLA PIPE i, dunque il processo di indice i + 1 che deve sempre leggere dalla pipe i leggera' 0 byte perche' il sistema riconosce che la pipe da cui 
sta tentando la lettura e' priva di scrittore, quindi il controllo di nr ha effetto e il processo termina. ANCH'ESSO SENZA STAMPARE NULLA nella pipeline e via dicendo con tutti
gli altri processi che seguono. La cosa fondamentale di questi controlli e' che permettono il blocco in catena di tutti i processi che seguono in caso di un errore in uno di 
quelli a monte, senza mandare avanti una struttura priva di significato. 
Infatti se non si effettuasse il controllo su nr e un figlio leggesse 0 byte tipo a causa di un errore nell'apertura del processo precedente, quello che accadrebbe e' che 
l'esecuzione di questo processo andrebbe avanti utilizzazndo come struttura quella NON INIZIALIZZATA e la spedirebbe al processo figlio successivo (schifeatoria) */
