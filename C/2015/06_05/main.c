/* programma C relativo alla seconda prova in itinere del 15 giugno 2015. */ 

/* Il programma accetta esattamente M parametri, che rappresentano ognuno un nome di file. Il compito del processo padre e' quello di dar vita ad esattamente M processi figl, ognuno dei quali e' implicitamente associato ad uno di questi file mediante il suo indice. Ogni processo figlio a sua volta dovra' poi creare UN SINGOLO processo nipote che vada ad eseguire il comando tail sul file associato al relativo processo figlio. Il processo nipote ritorna ovviamente al figlio associato il valore di ritorno della tail, a sua volta il processo figlio dovra' andare a riportare questo valore al padre */ 

/* protocollo di comunicazione: 
	- il processo padre crea M pipe per permettre la comunicazione con i vari processi figli. La necessita' di M pipe e' dettata dal fatto che il padre deve 
	ricevere i valori di ritorno dei figli in ordine inverso rispeto a quello di creazione, il che fa si' che sia necessario avere 1 pipe per ogni processo figlio
	che possono poi essere lette nell'ordine inverso rispetto a quello di creazione da parte del padre 

	- il processo figlio a sua volta crea 1 pipe per la comunicazione col nipote da lui creato. Questa pipe seguira' l'implementazione del piping dei comandi, 
	infatti cosi' facendo il processo nipote andra' ad eseguire il comando tail andando a scrivere l'ultima riga da lui letta da file direttamente in questa pipe,
	cosi' che possa essere letta dal processo figlio e se ne possano contare da lui i vari caratteri

	N.B: le varie M pipe di comunicazioni tra figli e nipoti VANNO CREATE DAI FIGLI STESSI E NON DAL PADRE, questo perche' il padre non ha nulla a che fare con 
	queste pipe. Non sara' ne' uno scrittore ne' un lettore, dunque sebbene sarebbe possibile dal punto di vista pratico farle creare direttamente dal padre, 
	logicamente la cosa ha poco senso 
*/ 

#include <stdlib.h> 
#include <unistd.h> 
#include <stdio.h> 
#include <sys/wait.h> 
#include <errno.h> 

/* definizione del tipo pipe_t come array di due interi, utile per la gestione dell'array degli fd delle varie pipe */
typedef int pipe_t[2];

int main (int argc, char **argv) 
{ 
	/*---------------------------------- Variabili Locali --------------------------------------*/ 
	int pid; 		/* identificatore dei processi utilizzato per le fork e le wait */ 
	int M; 			/* numero di file passati per parametro, che corrispondera' al numero di processi figli creati */ 
	int m, j; 		/* indici utilizzati nei vari cicli */ 
	pipe_t *piped; 		/* array dinamico di pipe, utilizzate per la comunicazione tra padre e figli */ 
	pipe_t p; 		/* singola pipe utilizzata per la comunicazione tra i figli e i rispettivi nipoti */ 
	int cnt; 		/* contatore dei caratteri letti dalla pipe */
	int lunghezza; 		/* lunghezza dell'ultima riga dei file privata del terminatore */ 
	char ch; 		/* carattere utilizzato per la lettura della pipe tra figlio e nipote */
	int status; 		/* variabile di stato per la wait */
	int ritorno;		/* valore ritornato dai processi figli e recuperato tramite status */ 
	/*------------------------------------------------------------------------------------------*/ 


	/* controllo lasco numero di parametri passati */ 
	if (argc < 3) 
	{ 
		printf("Errore: numero di parametri passato errato - Attesi almeno 2 - passati %d\n", argc - 1); 
		exit(1);
	} 

	/* inizializziamo il valore di M al numero di file passati come argc - 1. L'uno tolto e' dovuto al fatto che argc conta anche il nome dell'eseguibile */
	M = argc - 1; 

	/* allocazione della memoria per l'array di pipe_t tramite malloc */ 
	piped = (pipe_t *) malloc(M * sizeof(pipe_t)); 

	if (piped == NULL) 
	{ 
		printf("Errore: non e' stato possibile allocare memoria per l'array dinamico piped mediante la malloc\n"); 
		exit(2); 
	} 

	/* creazione delle M pipe padre-figlio */ 
	for (m = 0; m < M; m++) 
	{
		if (pipe(piped[m]) < 0)
		{ 
			printf("Errore: non e' stato possibile creare la %d-esima pipe dal processo padre\n", m); 
			exit(3); 
		} 
	} 

	
	printf("DEBUG-Sono il processo PADRE con pid = %d e sto per creare %d processi figli\n", getpid(), M); 

	/* creazione degli M processi figlio */
	for (m = 0; m < M; m++) 
	{
		if ((pid = fork()) < 0)
		{ 
			printf("Errore: non e' stata possibile la creazione del %d-esimo processo figlio mediante la fork\n", m); 
			exit(4);
		} 

		if (pid == 0) 
		{ 
			/* CODICE DEL PROCESSO FIGLIO */ 

			printf("DEBUG-Sono il processo figlio con pid = %d di indice %d e sto per creare il processo nipote che "
				"recuperera' l'ultima riga del file: %s\n", getpid(), m, argv[m + 1]); 

			/* chisura dei lati delle pipe che non vengono utilizzati dal processo corrente: 
				- chiusura dei lati di lettura di tutte le pipe 
				- chiusura dei lati di scrittura di tutte le pipe tranne di quella associata al processo corrente */

			for (j = 0; j < M; j++) 
			{ 	
				/* chiusura del lato di lettura */
				close(piped[j][0]); 

				/* chisura del lato di scrittura sse non si tratta della pipe del processo corrente */ 
				if (j != m) 
				{ 
					close(piped[j][1]); 
				} 
			} 
			
			/* creazione della pipe per la comunicazione col processo nipote */
			if (pipe(p) < 0) 
			{ 
				printf("Errore: non e' stato possibile creare la pipe di per la comunicazione tra il figlio %d e il "
					"relativo nipote\n", m); 
				exit(-1); 
			} 

			/* creazione del processo nipote */ 
			if ((pid = fork()) < 0) 
			{ 
				printf("Errore: non e' stato possibile creare il processo nipote relativo al figlio di indice %d con la fork\n", m); 
				exit(-2); 	

				/* OSS: e' lecito tornare un valore più piccolo di -1 perche' potranno comunque essere distinti dal processo padre dai valori di 
				ritorno della tail (processo nipote), questo perche' verosimilmente la tail ritorna valori piccoli, mentre i valori negativi 
				corrispondono a valori senza segno grandi */ 
			} 

			if (pid == 0) 
			{ 
				/* CODICE DEL PROCESSO NIPOTE */ 

				printf("DEBUG-Sono il processo nipote con pid = %d e figlio del processo figlio di indice %d " 
					"e sto per andare a leggere l'ultima riga del file: %s\n", getpid(), m, argv[m + 1]); 

				/* chisura del lato di scrittura della pipe utilizzata dal figlio associato per la comunicazione col padre */ 
				close(piped[m][1]); 

				/* ridirezione dello standard output sul lato di scrittura della pipe di comunicazione col figlio */ 
				close(1); 
				dup(p[1]); 

				/* chiusura di entrambi i lati di p, infatti quello di scrittura e' stato duplicato nel fd = 1 */ 
				close(p[0]); 
				close(p[1]); 
			
				/* facciamo eseguire al nipote il comando tail con opzione -1 per leggere l'ultima riga del file associato. 
				Si noti che si fa utilizzo della versione dell'exec con la p per poter cercare il comando dentro alla PATH, 
				dove si trovera' sicuramente in quanto comando di sistema */ 

				execlp("tail", "tail", "-1", argv[m + 1], (char *) 0); 
				
				/* questa parte di codice non dovrebbe essere eseguita dal nipote. Se accade significa che c'e' stato un errore con l'exec. 
				comunichiamo l'errore mediante la perror perche' lo stdout e' stato ridirezionato */
				perror("Errore: non e' stato possibile eseguire la tail da parte di uno dei nipoti meidante l'exec");
				exit(-3); 	/* si procede con la numerazione progressiva in negativo iniziata nel figlio */ 
			} 

			/* ritorno al codice del PROCESSO FIGLIO */ 

			/* chisura del lato di scrittura della pipe p */ 
			close(p[1]); 

			/* ciclo di lettura carattere per carattere dalla pipe p */ 
			
			cnt = 0; 
			while (read(p[0], &ch, 1) > 0) 
			{ 
				/* dei caratteri non ce ne frega nulla, ci serve solo contarli, andando dunque ad incrementare il contatore cnt */
				
				cnt++; /* incremento del contatore */ 
			}
			
			if (cnt) 
			{ 
				/* se sono stati effettivamente letti dei caratteri, che non e' scontato infatti: 
					- il file passato poteva essere vuoto quindi la tail non ha stampato nulla sulla pipe 
					- il file passato poteva non esistere o non essere leggibile, allora la tail non ha stampato nulla */ 
				
				lunghezza = cnt - 1; 	/* lunghezza comunicata al padre sara' cnt - 1 dovuto alla rimozione del carattere newline '\n' */ 
			} 
			else 
			{ 
				/* caso in cui non sono stati letti caratteri */ 
				lunghezza = 0; 		/* fosse stata cnt - 1 sarebbe diventata negativa e non avrebbe senso come cosa */
			} 

			/* invio della lunghezza al padre */ 
			write(piped[m][1], &lunghezza, sizeof(lunghezza)); 

			/* attesa del processo nipote con recupero del valore di ritorno da ritornare al padre */ 
			if ((pid = wait(&status)) < 0) 
			{ 
				printf("Errore: il processo figlio %d-esimo non e' riuscito ad attendere il nipote con la wait\n", m); 
				exit(-4); 
			} 

			/* a questo punto andiamo ad pre-inizializzare il valore di ritorno a -1, cosi' facendo se il figlio fosse terminato in modo anomalo 
			allora il valore di ritorno non verrebbe aggiornato e rimane -1 */ 

			ritorno = -1; 

			if ((status & 0xFF) != 0)
			{ 
				printf("Il processo nipote con pid = %d e' terminato in modo anomalo\n", pid); 
			} 
			else
			{ 
				ritorno = (int) ((status >> 8) & 0xFF); 
				printf("DEBUG-il nipote con pid = %d che ha eseguito un tail, ha ritornato: %d\n", pid, ritorno); 
			} 
			
			exit(ritorno); 
		} 
	}

	/* CODICE DEL PROCESSO PADRE */ 

	/* chisura di tutti i lati di scrittura delle M pipe da esso create */ 
	for (m = 0; m < M; m++) 
	{ 
		close(piped[m][1]); 
	} 

	/* recupero dei valori comunicati dai figli in ordine inverso rispetto a quello dei file */ 
	for (m = M - 1; m >= 0; m--) 
	{ 
		read(piped[m][0], &lunghezza, sizeof(lunghezza)); 
		printf("Il processo figlio di indice %d ha comunicato il valore %d per il file associato %s\n", m, lunghezza, argv[m + 1]); 
	} 

	/* attesa dei processi figli con stampa dei valori ritornati */ 
	for (m = 0; m < M; m++)
	{ 
		if ((pid = wait(&status)) < 0)
		{ 
			printf("Errore: non e' stato possibile attendere uno dei processi figli con la wait\n"); 
			exit(5); 
		} 

		if ((status & 0xFF) != 0) 
		{ 
			printf("Il processo figlio con pid = %d e' terminato in modo anomalo\n", pid); 
		} 
		else 
		{ 
			ritorno = (int)((status >> 8) & 0xFF); 
			
			if (ritorno) 	/* caso in cui ci sono sati problemi da qualche parte (o nel figlio o nel relativo nipote) */ 
			{ 
				printf("Il processo figlio con pid = %d ha ritornato %d, questo significa che il tail ha ritornato qualche errore "
					"oppure il figlio o il nipote associato sono incorsi in qualche problematica\n", pid, ritorno); 
			} 
			else		/* caso in cui tutto e' andato bene ed il tail ha avuto successso */ 
			{
				printf("Il processo figlio con pid = %d ha ritornato %d\n", pid, ritorno);
			} 
		} 
	} 

	exit(0); 
} 
