/* programma C main.c che risolve la parte di C della prova di esame totale tenutasi il 11 luglio 2018 (versione con calcolo del massimo da parte del padre) */ 

#include <stdlib.h> 
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/wait.h> 
#include <string.h> 
#include <ctype.h> 

/* definizione del TIPO pipe_t che rappresenta un array di due interi, che useremo per la gestione dei lati delle varie pipe create */
typedef int pipe_t[2];

/* andiamo a dichiare qualche variabile GLOBALE. Si tratta di variabili che dal momento che servono sia al main che alle 
altre funzioni, tanto vale che vengano dichiarate a livello globale, cosi' che tutte le funzioni all'interno del file possano
accedervi */

/*--------------- Variabili globali ----------------*/ 		
int *finito; 		/* array dinamico di N elementi che indica lo stato di ogni figlio. La sua semantica prevede che: 
			- se finito[i] == 0, allora il processo figlio i-esimo non sia terminato e sia dunque in attesa del messaggio di stampa del padre 
			- se finito[i] == 1, allora il processo figlio i-esimo siamo certi sia terminato. L'accertamento di tale avvenimento e' dato dal fatto 
			  che la read dalla sua pipe ha tornato precedentemente 0, che implica che lo scrittore (il figlio stesso) sia appunto terminato */ 
int N; 			/* numero di file passati, o analogamente di processi figli */ 
/*--------------------------------------------------*/


/* definzione della funzione che controlla i valori dell'array finito e ritorna 1 se TUTTI i processi figli sono terminati, 0 altrimenti */ 
int all_ended ()
{ 
	/* nota che non accetta parametri poiche' l'array lo abbiamo definito come globale */ 
	for (int i = 0; i < N; i++)
	{ 
		if (!finito[i])
		{ 
			return 0; 
		} 
	}
	
	/* se non si e' ritornato 0 per un qualche processo non finito, allora la funzione ritorna 1 */ 
	return 1; 
}


int main (int argc, char **argv)
{
	/*--------------- Variabili locali ---------------*/
	int pid; 		/* identificatore dei processi figli creati con la fork e attesi con la wait */ 
	int i, j; 		/* indici per i cicli */
	char CZ; 		/* carattere target da cercare nei vari file */ 
	pipe_t *piped_fp; 	/* array dinamico di pipe che permettono la comunicazione DA figli A padre, in questo specifico verso */ 
	pipe_t *piped_pf; 	/* array dinamico di pipe che permettono la sincronizzazione tra il padre e i vari figli. Attraverso queste pipe infatti 
				il processo padre spedira' dei singoli caratteri per notificare i figli se debbano stampare o meno */
	int fdr; 		/* file descriptor relativo all'apertura in lettura dei file da parte dei figli */
	char ch; 		/* variabile "buffer" per salvare i vari caratteri letti da file dai figli */
	char ch_ctrl; 		/* carattere di controllo spedito dal padre per notificare i figli di come comportarsi con la stampa. Questo varra': 
				- 'S' se il padre vuole che il figlio corrente stampi 
				- 'N' se il padre vuole che il figlio corrente NON stampi nulla */
	int occ; 		/* contatore di occorrenze di CZ. Servira' come valore di ritorno dei figli */
	long int pos; 		/* posizione corrente nella lettura da file dei processi figli. Analogamente la variabile verra' usata dal padre per memorizzare 
				temporaneamente le posizioni inviate dai vari figli */ 
	long int max_pos; 	/* posizione massima all'interno di un insieme di posizioni spedite dai figli */ 
	int max_idx; 		/* indice del processo figlio al quale si attribuisce la posizione massima dell'occorrenza */
	int nr, nw; 		/* variabili di controllo per la lettura e scrittura dalle pipe */ 
	int status; 		/* variabile di stato per la wait */ 
	int ritorno; 		/* variabile per salvare i valori tornati dai vari figli */
	/*------------------------------------------------*/
	
	/* controllo lasco numero di parametri passati */ 
	if (argc < 4)
	{
	 	printf("Errore: numero di parametri passato errato - Attesi almeno 3 (1 carattere e 2 file) - Passati: %d\n", argc - 1); 
		exit(1); 
	} 

	N = argc - 2; 	/* inizializziamo N al numero di file, che otteniamo da argc (parametri totali) sottraendo 2 (nome eseguibile e carattere target) */

	/* controllo che il primo parametro sia un singolo carattere */ 
	if (strlen(argv[1]) != 1) 
	{ 
		printf("Errore: il parametro %s non corrisponde ad un singolo carattere\n", argv[1]); 
		exit(2); 
	} 

	CZ = argv[1][0]; 	/* estraiamo il singolo carattere dalla rispettiva stringa dei parametri */ 

	/* allochiamo la memoria per gli array dinamici di pipe */
	piped_fp = (pipe_t *)malloc(N * sizeof(pipe_t)); 
	piped_pf = (pipe_t *)malloc(N * sizeof(pipe_t)); 

	if ((piped_fp == NULL) || (piped_pf == NULL))
	{ 
		printf("Errore: allocazione della memoria degli array di pipe con malloc NON riuscita\n"); 
		exit(3); 
	} 

	/* creazione delle N pipe per la comunicazione tra figli e padre e delle N per la sincronizzazione tra padre e figli */
	for (i = 0; i < N; i++)
	{ 
		if (pipe(piped_fp[i]) < 0)
		{ 	
			printf("Errore: creazione della pipe di indice %d figli-padre NON riuscita\n", i); 
			exit(4); 
		} 
		if (pipe(piped_pf[i]) < 0) 
		{ 
			printf("Errore: creazione della pipe di indice %d padre-figli NON riuscita\n", i); 
			exit(5); 
		} 
	} 

	/* allocazione della memoria per l'array "finito" */ 
	if ((finito = (int *)malloc(N * sizeof(int))) == NULL) 
	{ 	
		printf("Errore: allocazione della memoria per l'array finito NON riuscita\n"); 
		exit(6); 
	} 

	/* inizializziamo l'array a soli 0, supponendo dunque che nessun processo sia finito, mediante una memset */
	memset(finito, 0, N * sizeof(int)); 

	/* ciclo di creazione dei processi figli */
	for (i = 0; i < N; i++) 
	{ 
		if ((pid = fork()) < 0) 
		{ 	
			printf("Errore: creazione del figlio %d con fork NON riuscita\n", i); 
			exit(7);
		} 

		if (pid == 0) 
		{
		 	/* CODICE PROCESSI FIGLI */ 

			/* nel caso che il processo figlio incorra in qualche tipo di errore, ritorneremo il valore -1 (255) */

			printf("DEBUG-Sono il processo figlio di indice %d e pid = %d e sto per cercare le occorrezne del carattere '%c' "
				"nel file %s\n", i, getpid(), CZ, argv[i + 2]); 

			/* chidiamo tutti i lati delle pipe che non vengono usate dal processo Pi: 
			ogni processo di indice i SCRIVE solo dalla pipe piped_fp[i], e legge solo dalla pipe piped_pf[i] */ 
			for (j = 0; j < N; j++) 
			{ 
				close(piped_fp[j][0]); 
				close(piped_pf[j][1]); 

				if (j != i) 
				{ 
					close(piped_fp[j][1]); 
					close(piped_pf[j][0]); 
				} 
			} 

			/* tentiamo l'apertura del file associato al processo Pi, che si trovera in posizione i + 2 di argv */ 
			if ((fdr = open(argv[i + 2], O_RDONLY)) < 0) 
			{ 
				printf("Errore: il file %s non esiste oppure non e' leggibile\n", argv[i + 2]); 
				exit(-1); 
			} 

			/* inizializziamo il valore di pos a 0L, cosi' facendo stiamo imponendo che il primo carattere si trovi in posizione 0, esattamente nello stesso 
			con cui lavora la funzione lseek */
			pos = 0L; 
			occ = 0; 
			
			/* ciclo di lettura da file carattere per carattere con ricerca delle occorrenze del carattere CZ */ 
			while (read(fdr, &ch, 1) > 0) 
			{ 
				if (ch == CZ) 
				{ 	
					occ++; 

					/* invio al padre della posizione della occorrenza trovata */ 
					nw = write(piped_fp[i][1], &pos, sizeof(pos)); 

					/* controllo sul numero di byte inviati nella pipe */ 
					if (nw != sizeof(pos))
					{ 
						printf("Errore: il figlio di indice %d non e' riuscito a scrivere correttamente sulla pipe\n", i); 
						exit(-1); 
					} 

					/* lettura del carattere inviato dal padre */
					nr = read(piped_pf[i][0], &ch_ctrl, 1); 

					/* controllo che sia stato letto correttamente un singolo carattere */ 
					if (nr != 1)
					{ 
						printf("Errore: il figlio di indice %d non e' riuscito a leggere correttamente dalla pipe il messaggio del padre\n", i);
						exit(-1);
					} 

					/* controllo del valore del carattere di controllo spedito dal padre, e corrispettiva azione associata */
					if (ch_ctrl == 'S')
					{ 	
						/* effettuiamo la stampa di tutte le informazioni richieste nella specifica */
						printf("Il processo figlio di indice %d e pid = %d ha trovato l'occorrenza no. %d del carattere '%c' nel file %s "
							"nella posizione massima rispetto agli altri figli, ossia %ld\n", i, getpid(), occ, CZ, argv[i + 2], pos); 
					} 
				} 

				pos++;	/* si noti che NON va messo l'else perche' si continua a tenere traccia della posizione
					globale nel fie anche dopo aver trovato un'occorrenza del carattere CZ */					
			} 
			
			exit(occ); 	/* come da specifica i figli ritornano al padre il numero totale di occorrenze trovate di CZ nel file associato */
		}
	} /* fine for */ 

	/* CODICE DEL PADRE */ 

	/* chiusura di tutti i lati delle pipe non usati dal padre, ossia tutti i lati di scrittura delle pipe di piped_fp, e tutti quelli di lettura delle pipe di piped_pf */
	for (i = 0; i < N; i++)
	{ 
		close(piped_fp[i][1]); 
		close(piped_pf[i][0]); 
	} 

	/* ciclo di recupero ordinato delle posizioni spedite dai figli. Si va avanti a recuperare le posizioni e a determinare quella massima, fintanto che TUTTI i processi 
	non sono terminati */ 
	while(!all_ended()) 
	{
		max_pos = -1L; 	/* inizializziamo la posizione massima ad un valore basso che sicuramente verra' superato anche dal minore dei valori possibili tornati dai figli*/

		/* ciclo di recupero delle posizioni delle occorrenze di CZ nei figli, con aggiornamento del loro stato tramite finito */ 
		for (i = 0; i < N; i++) 
		{ 
			nr = read(piped_fp[i][0], &pos, sizeof(pos)); 

			if (nr == 0) 
			{ 
				/* allora il processo figlio di indice i e' terminato */ 
				finito[i] = 1; 
			} 
			else if (nr == sizeof(pos)) 
			{ 
				/* il figli i-esimo ha inviato una posizione, quindi non e' terminato e si trova in attesa del messaggio dal padre */ 
				
				/* controllo se si ha trovato la posizione massima */ 
				if (pos > max_pos) 
				{ 
					max_pos = pos; 
					max_idx = i;
				}
			}
			else 	/* numero di byte letti dalla pipe errato */ 
			{ 
				printf("Errore: il processo padre ha letto un numero errato di byte dalla pipe col figlio %d\n", i); 
			} 
		} 

		/* ciclo di invio ai vari figli dell'informazione sulla stampa */ 
		for (i = 0; i < N; i++) 
		{ 
			if (i == max_idx) 
			{ 
				ch_ctrl = 'S'; 
			} 
			else 	/* figlio che NON ha trovato la posizione massima nell'occorrenza corrente */ 
			{ 
				ch_ctrl = 'N'; 
			} 

			if (!finito[i])
			{ 	
				/* invio all'i-esimo figlio il carattere di controllo */ 
				nw = write(piped_pf[i][1], &ch_ctrl, 1);

				/* controllo del corretto invio del messaggio */ 
				if (nw != 1) 
				{ 
					printf("Errore: il processo padre non e' riuscito a inviare correttamente al figlio %d il messaggio di controllo\n", i); 
				} 
			} 
		}
	}

	/* attesa dei vari figli da parte del padre con recuper del valore tornato e stampa di questo */ 
	for (i = 0; i < N; i++) 
	{ 
		if ((pid = wait(&status)) < 0) 
		{ 
			printf("Errore: attesa di uno dei figli da parte del padre con la wait non riuscita\n");
			exit(8); 
		} 

		if(WIFEXITED(status))
		{ 	
			ritorno = WEXITSTATUS(status); 
			printf("Il processo figlio con pid = %d ha ritornato il valore %d (se 255 e' incorso in qualche problema!)\n", pid, ritorno);
		} 
		else
		{
			printf("Il processo figlio con pid = %d e' terminato in modo anomalo\n", pid);
		}
	}

	exit(0); 
}
