/* programma C main.c che risolve la parte in C della prova di esame totale tenutasi il 13 luglio 2022 */

#include <stdlib.h> 
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/wait.h> 
#include <string.h> 
#include <ctype.h> 
#include <signal.h>

/* definizione del TIPO pipe_t che rappresenta un array di due interi, che useremo per la gestione dei lati delle varie pipe create */
typedef int pipe_t[2];

int main (int argc, char **argv)
{
	/*################ Variabili locali #################*/
	int pid; 		/* identificatore dei processi figli creati con fork e attesi con wait */
	int Q; 			/* numerodi figli da crearer, che corrisponde al numero di caratteri passati */	
	int q; 			/* indice dei processi figli */
	int k; 			/* generico indice per i cicli */
	int L; 			/* lunghezza in linee del file passato */
	pipe_t *piped; 	/* array dinamico di Q + 1 pipe che implementa uno schema di comunicazione/sincronizzazione tra i processi a PIPE RING, dove: 
						- la comunicazione parte dal padre e passa per i vari figli in ordine, quindi a P0, P1, ... fino a PQ - 1 che tornera' a comunicare col padre
						- il processo padre legge dalla pipe di indice Q e scrive su quella di indice 0
						- i processi figli leggono dalla pipe di indice q e scrivono su quella di indice q + 1 */
	int row;		/* contatore della riga corrente nel padre */
	int fdr; 		/* file descriptor per l'apertura del file nei vari processi */
	char ctrl; 		/* carattere di controllo usato per la sincronizzazione tra processi. Si noti che il suo valore non verra' utilizzato da nessun processo
						dunque e' possibile lasciarlo non inizializzato */
	char c; 		/* carattere letto dai figli di volta in volta */
	char CZ; 		/* carattere target del singolo figlio */
	int occ; 		/* contantore di occorrenze del carattere CZ nella linea corrente */
	int status; 	/* variabile di stato per la wait */
	int ritorno; 	/* valore ritornato dai vari processi figli */
	int nr, nw; 	/* variabili per il controllo della lettura/scirttura su pipe ring */
	/*###################################################*/
	
	/* controllo lasco numero di parametri */
	if (argc < 5)
	{
		printf("Errore: numero di parametri passato errato - attesi almeno 4: [file_name, lunghezza, char1, char2] - passati: %d\n", argc - 1); 
		exit(1); 
	}

	/* inizializziamo il valore di Q, togliendo al numero totale di parametri (argc), il nome dell'eseguibile, del file parametro e della sua lunghezza */
	Q = argc - 3; 

	/* controllo che il secondo parametro (L) sia un numero strettamente positivo */
	L = atoi(argv[2]); 
	if (L <= 0)
	{ 
		printf("Errore: il parametro %s NON e' un numero strettamente positivo\n", argv[2]);
		exit(2); 
	}

	/* controllo che gli ultimi Q parametri siano dei SINGOLI caratteri */
	for (k = 0; k < Q; k++)
	{ 
		if (strlen(argv[k + 3]) != 1)
		{
			printf("Errore: il parametro %s NON e' un singolo carattere\n", argv[k + 3]);
			exit(3);
		}
	}

	/*
		Decidiamo di trattare il segnale SIGPIPE in modo da IGNORARLO, questo per far fronte al caso in cui il file passato per parametro sia NON leggibile, oppure inesistente. Infatti 
		se accadesse cio', il processo padre tenterebbe inizialmente di andare a scrivere al processo P0 il carattere ci controllo, scrivendo dunque su una pipe senza lettore, perche' 
		il processo P0 (cosi' come tutti gli altri) termina se non riesce ad apire il file; Cosi' facendo dunque riceverebbe il SIGPIPE, cosa che non vogliamo perche' il padre terminerebbe 
		senza aver atteso i processi figli, che rimarrebbero zombie 
	*/
	signal(SIGPIPE, SIG_IGN); 

	/* allocazione dinamica della memoria per l'array di Q + 1 pipe che implementano uno schema di comunicazione a pipe ring */
	if ((piped = (pipe_t *)malloc((Q + 1)*sizeof(pipe_t))) == NULL)
	{
		printf("Errore: allocazione della memoria dell'array di pipe NON riuscita\n"); 
		exit(4); 
	}

	/* ciclo di creazione delle Q + 1 pipe */
	for (k = 0; k < Q + 1; k++)
	{
		if (pipe(piped[k]) < 0)
		{
			printf("Errore: creazione della pipe di indice %d NON riuscita\n", k); 
			exit(5); 
		}
	}

	/* ciclo di creazione dei processi figli */
	for (q = 0; q < Q; q++)
	{ 
		if ((pid = fork()) < 0)
		{
			printf("Errore: fork del processo figlio di indice %d non riuscita\n", q); 
			exit(6); 
		}

		if (pid == 0)
		{ 
			/* CODICE DEI FIGLI */

			/* dal momento che il normale ritorno dei figli sara' strettamente minore di 255 per specifica, decidiamo che in caso di errore questi ritornino esattamente -1 (ossia 255), cosi'
				da poter distinguere i casi di errore da quelli di successo */
			
			printf("DEBUG-Sono il processo figlio di indice %d e con pid = %d e sto per cercare le occorrenze del carattere %s nel file %s\n", q, getpid(), argv[q + 3], argv[1]); 

			/* inizializziamo CZ al valore del carattere associato al processo Pq */
			CZ = argv[q + 3][0]; 

			/* chisuura dei lati delle pipe non usati */
			for (k = 0; k < Q + 1; k++)
			{ 
				/* chiusura dei lati di lettura di tutte le pipe, tranne quella di indice q */
				if (k != q)
				{ 
					close(piped[k][0]);
				}

				/* chisura dei lati di scrittura di tutte le pipe, trenne quello della pipe di indice q + 1 */
				if (k != q + 1) 
				{ 
					close(piped[k][1]); 
				}
			}

			/* apertura del file */
			if ((fdr = open(argv[1], O_RDONLY)) < 0)
			{
				printf("Errore: il processo figlio di indice %d NON e' riuscito ad aprire il file %s - file NON leggibile oppure NON esistente\n", q, argv[1]); 
				exit(-1); 
			}

			/* ciclo di lettura da file, carattere per carattere con conteggio delle occorrenze di CZ ad ogni linea e SINCRONIZZAZIONE con gli altri processi */
			occ = 0; 	/* inizializziamo il contatore di occorenze a 0 */
			while (read(fdr, &c, 1) > 0)
			{ 
				if (c == '\n')
				{
					/* ci troviamo alla fine della line corrente, dunque ci mettiamo in attesa di ricevere il byte di controllo dal processo precedente per poter stampare */
					nr = read(piped[q][0], &ctrl, 1); 

					/* controllo della lettura da pipe */
					if (nr != 1) 
					{
						printf("Errore: il processo figlio di indice %d non e' riuscito a leggere correttamente il carattere di controllo dal pipe ring\n", q); 
						exit(-1); 
					}
				
					/* stampa richiesta dal testo */
					printf("Il figlio con indice %d e con pid = %d ha letto %d volte il carattere '%c' dalla linea corrente\n", q, getpid(), occ, CZ); 

					/* invio del messaggio di sincronizzazione al processo successivo */
					nw = write(piped[q + 1][1], &ctrl, 1); 

					/* controllo della scrittura su pipe */
					if (nw != 1)
					{ 
						printf("Errore: il processo figlio di indice %d non e' riuscito a scrivere correttamente il carattere di controllo sul pipe ring\n", q); 
						exit(-1); 	
					}

					/* dal momento che iniziamo una nuova riga andiamo ad AZZERARE il numero di occorrenze, andando pero' prima a salvarci questo valore dentro la variabile ritorno 
						cosi' che se siamo sull'ultima riga poi possiamo tornare al padre questo valore */ 
					ritorno = occ; 
					occ = 0; 
				}
				else if (c == CZ)
				{
					occ++; 
				}
			}

			exit(ritorno); 
		}
	}

	/* CODICE DEL PADRE */

	/* il processo padre legge dalla pipe di indice Q e scrive su quella di indice 0, facendo a tutti gli effetti parte del pipe ring, dunque tutti gli atlri lati possono essere chiusi */
	for(k = 0; k < Q + 1; k++)
	{ 
		if (k != Q)
		{
			close(piped[k][0]);
		}

		if (k != 0)
		{
			close(piped[k][1]); 
		}
	}

	/* 
		ciclo di sincronizzazione coi processi figli che itera sul numero di righe del file passato per parametro. Si noti che al momento della stampa della riga, row verra' incrementato di 1,
		questo perche' nella specifica viene chiesto che la numerazione delle linee sia fatta partire da 1. 
		
		Si noti che dal momento che il padre in questo programma fa parte del ciclo di sincronizzazione MA e' anche colui che deve deve DARE IL VIA al ciclo, prima di sincronizzarsi in attesa
		del figlio PQ-1 per la prima volta, dovra' mandare il primo messaggio al figlio 0. Si noti che abbiamo gia' controllato che L >= 0, quindi questa prima stampa avra' sicuramente senso 
	*/

	for (row = 0; row < L; row++)
	{ 
		/* stampa richiesta dal testo */ 
		printf("Linea %d del file %s:\n", row + 1, argv[1]);

		/* invio a P0 del via libera per la scrittura */
		nw = write(piped[0][1], &ctrl, 1); 

		/* controllo che la scrittura abbia successo */
		if(nw != 1)
		{ 
			printf("Errore: il padre ha scritto un numero errato di byte dal pipe ring: %d\n", nw);
			/* exit(8); 	commentiamo questo ritorno per lo stesso motivo del (7) */
			break; 
		}

		/* attesa che il processo PQ-1 dia il via libera alla stampa della prossima riga */
		nr = read(piped[Q][0], &ctrl, 1); 

		/* controllo che la lettura abbia avuto successo */
		if (nr != 1)
		{ 
			printf("Errore: il padre ha letto un numero errato di byte dal pipe ring: %d\n", nr); 
			/* exit(7);    decidiamo di non uscire perche' se no NON andremmo ad attendere i figli */
			break; 
		}

	}
	
	/* ciclo di attesa dei processi figli con recupero e stampa del loro valore di ritorno */
	for (q = 0; q < Q; q++)
	{ 
		if ((pid = wait(&status)) < 0)
		{ 
			printf("Errore: wait di uno dei figli NON riuscita\n"); 
			exit(9); 
		}

		if ((status & 0xFF) != 0)
		{ 
			printf("Il processo figlio con pid = %d e' terminato in modo anomalo\n", pid); 
		}
		else 
		{ 
			ritorno = (int)((status >> 8) & 0xFF); 
			if (ritorno == 255)
			{ 
				printf("Il processo figlio con pid = %d e' incorso in qualche problema, infatti ha ritornato %d\n", pid, ritorno); 
			}
			else 
			{
				printf("Il processo figlio con pid = %d ha ritornato il valore %d\n", pid, ritorno); 
			}
		}
	}
	
	exit(0); 
}
