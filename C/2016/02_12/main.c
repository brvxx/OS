/* Programma C main.c che risolve la parte in C della prova di esdame totale del 12 febbraio 2016 */ 

/* Breve descrizione del programma: 
Il programma accetta N + 1 parametri, con N >= 2 nomi di file e 1 singolo carattere alfabetico minuscolo Cx. Il processo padre crea N processi figli, ognuno dei quali associato ad un file in base all'ordine in cui si trovano i file dentro argv. Ogni processo figli conta il numero di occorrenze del carattere Cx dentro al rispettivo file e comunica l'informazione al processo di indice successivo attraverso una pipeline. L'informazione passata tra i processi fratelli è un array dinamico che contiene delle strutture contenenti informazioni sul numero di occorrenze lette da un certo figlio e l'indice di quel figlio. Via via che si procede con la comunicazione l'array diventa sempre più grande, così che possa contenere le strutture dei nuovi figli. Al termine della pipeline si trova il processo padre che legge questo array stampando il contenuto di ogni struct, assieme al carattere Cx e il pid associato all'indice del processo. 
Alla fine della procedura il processo padre attende gli N figli e stampa in output il loro pid assieme al valore ritornato, che altro non e' che il loro indice d'ordine */

#include <stdlib.h> 
#include <unistd.h> 
#include <stdio.h> 
#include <fcntl.h> 
#include <string.h>
#include <ctype.h> 
#include <sys/wait.h> 

/* definizione del TIPO pipe_t che rappresenta un array di due interi, il quale ci servira' per la gestione delle pipe */ 
typedef int pipe_t[2]; 

/* definizione del TIPO tipo_s che rappresenta una particolare struttura dati che servira' per passare le informazioni 
relative alla lettura di file di ogni processo tramite la pipeline */
typedef struct { 
	int id; 
	long int occ; 
} tipo_s; 


int main (int argc, char **argv)
{ 
	/*---------------- Variabili locali -----------------*/
	int N;			/* numero di file e qundi di figli creati */
	char Cx; 		/* carattere da cercare nei vari file */ 
	int i, j; 		/* indici per i cicli */ 
	int *pid; 		/* array dinamico che conterra' i pid dei processi figli */ 
	pipe_t *piped; 		/* array dinamico di pipe usate secondo uno schema di comunicazione a pipeline che parte dal processo di indice 0 
				fino all'ultimo processo di indice N - 1 per poi arrivare al padre. Dunque ogni processi di indice i legge dal lato 
				di lettura della pipe i - 1 per poi andare a scrivere su quella di indice i */ 
	tipo_s *cur; 		/* array dinamico di strutture. Ogni processo alloca dinamicamente il quantitiativo minimo necessario di memoria per
				questo array in base alle proprie esigenze */ 
	int fdr;		/* file descriptor dei file aperti in lettura dai figli */ 
	char c; 		/* carattere per salvare i byte letti da file */ 
	int nr, nw; 		/* variabili per controllare se la lettura e scrittura su pipeline abbia avuto successo */ 
	int pidFiglio;		/* identificatore dei figli attesi dal padre */ 
	int status; 		/* variabile di stato per la wait */ 
	int ritorno; 		/* variabile per recuperare il valore tornato dai figli al padre */ 
	/*---------------------------------------------------*/


	/* controllo lasco numero di parametri */ 
	if (argc < 4) 
	{ 
		printf("Errore: numero di parametri passato errato - Attesi almeno 3 (due file e un singolo carattere) - Passati %d\n", argc - 1); 
		exit(1); 
	} 

	N = argc - 2; 		/* inizializziamo N al numero di file effettivo, ottenuto sottraendo dal numero totale di parametri 2 (nome dell'eseguibile e il carattere) */

	/* controllo che l'ultimo parametro sia un singolo carattere alfabetico minusccolo */ 
	if (strlen(argv[argc - 1]) != 1) 
	{ 
		printf("Errore: il parametro %s non e' un singolo carattere\n", argv[argc - 1]); 
		exit(2); 
	} 
	
	Cx = argv[argc - 1][0];	/* estraiamo il primo e unico carattere della stringa corrispondente all'ultimo parametro dentro Cx */ 

	if (!islower(Cx))
	{ 
		printf("Errore: il carattere %c non e' un carattere alfabetico minuscolo\n", Cx); 
		exit(3);
	} 

	/* allocazione dell'array di pid che servira' al padre per associare all'indice dei figli il realtivo pid */ 
	if ((pid = (int *)malloc(N * sizeof(int))) == NULL) 
	{ 
		printf("Errore: l'allocazione dell'array di pid mediante la malloc non e' riuscita\n"); 
		exit(4); 
	} 

	/* allocazione dell'array di N pipe */ 
	if ((piped = (pipe_t *)malloc(N * sizeof(pipe_t))) == NULL) 
	{ 
		printf("Errore: l'allocazione dell'array di pipe mediante la malloc non e' riuscita\n"); 
		exit(5); 
	}

	/* creazione delle N pipe che implementeranno la pipeline */ 
	for (i = 0; i < N; i++) 
	{ 
		if (pipe(piped[i]) < 0)
		{ 
			printf("Errore: non e' stato possibile creare la pipe %d\n", i); 
			exit(6); 
		} 
	} 

	/* creazione da parte del padre degli N processi figli tramite un ciclo di fork. Gli identificatori dei processi creati verranno messi dentro all'array di pid */ 
	for (i = 0; i < N; i++) 
	{ 
		if ((pid[i] = fork()) < 0) 
		{ 
			printf("Errore: non e' stata possibile la creazione del figlio %d con la fork()\n", i); 
			exit(7); 
		}

		if (pid[i] == 0) 
		{ 
			/* CODICE DEL PROCESSO FIGLIO */ 
			
			/* Dal momento che i processi figli di norma ritornano il loro indice, il quale sara' < N, possiam pensare di utilizzare 
			valori negativi decrescenti (-1, -2, -3, ...) per indicare i vari casi di errore dei figli e riuscire comunque a distinguerli
			dai ritorni normali */ 

			printf("DEBUG-Sono il processo figlio con indice d'ordine %d e pid = %d "
				"e sto per contare il numero di occorrenze del carattere '%c' dal file %s\n", i, getpid(), Cx, argv[i + 1]); 

			/* chisura dei lati non usati delle pipe non usati dal processo di indice i: 
			- lati di lettura di tutte le pipe, tranne quella di indice i - 1, se i == 0 questi lati verranno chiusi tutti 
			- lati di scrittura di tutte le pipe tranne di quella con indice i */ 

			for (j = 0; j < N; j++) 
			{
				if (j != i) 
				{ 	/*chiusura lato scrittura */
					close(piped[j][1]); 
				} 

				if ((i == 0) || (j != i - 1))
				{ 	/* chiusura lato lettura */ 
					close(piped[j][0]); 
				} 
			} 
			
			/* allocazione dinamica dell'array di strutture di dimensione i + 1, affinche' questo possa contenere le i strutture lette
			ma anche la struttura del processo corrente */
			
			if ((cur = (tipo_s *)malloc((i + 1) * sizeof(tipo_s))) == NULL) 
			{ 
				printf("Errore: il figlio di indice %d non e' riuscito ad allocare l'array di struct\n", i); 
				exit(-1); 
			} 

			/* inizializzazione della struttura del processo corrente nella posizione i dell'array */ 
			cur[i].id = i; 
			cur[i].occ = 0L; 

			/* apertura file associato al processo di indice i */ 
			if((fdr = open(argv[i + 1], O_RDONLY)) < 0)
			{ 
				printf("Errore: il file %s non esiste oppure non e' leggibile\n", argv[i + 1]); 
				exit(-2); 
			} 

			/* lettuara da file carattere per carattere incremementando via via il numero di occorrenze trovate di Cx */ 
			while (read(fdr, &c, 1) > 0) 
			{ 
				if (c == Cx) 
				{ 
					(cur[i].occ)++; 	/* trovata occorrenza, dunque incrementiamo occ */ 
				} 
			} 

			/* lettura dalla pipe di indice i - 1 l'array di strutture spedito dal figlio precedenete. Questa parte per il processo figlio 
			di indice 0 non va effettuata poiche' nessuno gli spedisce un array a lui, sara' il primo a mandare dentro la pipeline */
			if (i != 0) 
			{ 
				nr = read(piped[i - 1][0], cur, i * sizeof(tipo_s)); 

				/* controllo che sia stato letto il numero giusto di byte e che quindi non si sia rotta la pipe */ 
				if (nr != (i * sizeof(tipo_s)))
				{ 
					printf("Errore: Il processo figlio di indice %d ha letto un numero sbagliato di byte dalla pipeline: %d\n", i, nr); 
					exit(-3);
				} 
			}

			/* tutti i processi a prescindere dall'indice scrivono tutti gli i + 1 elementi del loro array dentro la pipe di indice i */ 
			nw = write(piped[i][1], cur, (i + 1) * sizeof(tipo_s)); 

			/* controllo che il numero di byte scritti sia corretto */ 
			if (nw != ((i + 1) * sizeof(tipo_s))) 
			{ 
				printf("Errore: Il processo figlio di indice %d ha scritto un numero sbagliato di byte nella pipeline: %d\n", i, nw); 
				exit(-4); 
			} 	
			
			/* ogni figlio ritorna al padre il proprio indice */ 
			exit(i); 
		} 
	} /* fine for */ 

	/* CODICE DEL PROCESSO PADRE */ 

	/* chiusura dei lati delle pipe non utilizzati: 
	- i lati di scrittura di TUTTE le pipe
	- i lati di lettura di TUTTE le pipe tranne l'ultima (di indice N - 1) */

	for (i = 0; i < N; i++) 
	{ 
		close(piped[i][1]); 

		if (i != N - 1) 
		{ 
			close(piped[i][0]); 
		} 
	}

	/* allocazione dinamica di memoria per l'array cur che dovra' contenere le N strutture create inizializzate dai vari figli */ 
	if ((cur = (tipo_s *)malloc(N * sizeof(tipo_s))) == NULL) 
	{ 
		printf("Errore: il processo padre non e' riuscito ad allocare memoria per l'array di struct\n");
		exit(8); 
	} 

	/* lettura dell'array di N struct dalla pipeline con controllo sul numero di byte letti */ 
	nr = read(piped[N - 1][0], cur, N * sizeof(tipo_s)); 

	if (nr != (N * sizeof(tipo_s)))
	{ 
		printf("Errore: il processo padre ha letto un numero sbagliato di byte dalla pipeline: %d\n", nr); 
		/* exit(9);   commentato perche' se no il padre terminerebbe senza attendere i figli */
	} 
	else 	/* se il numero di struct lette e' corretto */ 
	{ 
		/* scrittura su standard output delle varie informazioni spedite dai figli */ 
		for (i = 0; i < N; i++) 
		{ 
			printf("Il processo di indice %d e pid = %d ha trovato %ld occorrenze del "
				"carattere '%c' dentro al file %s\n", cur[i].id, pid[cur[i].id], cur[i].occ, Cx, argv[cur[i].id + 1]); 

			/* nota che andiamo ad usare cur[i].id perche' in questo caso effettivamente le cose corrispondono, perche' dentro 
			all'i-esimo elemento di cur troveremo la struct dell'i-esimo processo, ma se questo array fosse stato ordinato dal 
			padre a quel punto nulla ci avrebbe più garantito la cosa, quindi si usa sempre l'id contenuto nella struct */ 
		}
	}

	/* attesa dei figli e recupero dei valori tornati */ 
	for (i = 0; i < N; i++)
	{ 
		if ((pidFiglio = wait(&status)) < 0) 
		{ 
			printf("Errore: non e' stato possibile attendere uno dei figli con la wait\n"); 
			exit(10); 
		} 

		if (WIFEXITED(status)) 
		{ 
			ritorno = WEXITSTATUS(status); 
			printf("Il processo figlio di pid = %d ha ritornato il valore %d (se > di %d ci sono stati dei problemi nel figlio)\n", pidFiglio, ritorno, N - 1); 
		} 
		else
		{ 
			printf("Il processo figlio di pid = %d e' terminato in modo anomalo\n", pidFiglio); 
		} 
	} 
	
	exit(0); 
}

