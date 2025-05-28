/* programma C main.c che risolve la parte C della prova di esame totale tenutasi nel 09 settembre 2015. 
	In questa versione: 
	- useremo la comunicazione di caratteri via pipe per la sincronizzazione tra padre e figli
	- la terminazione della lettura dalla pipe per i processi che hanno sempre fornito caratteri corretti avvera' sempre via pipe 
	- tratteremo la terminazione dei figli che hanno fornito un carattere sbagliato tramite il segnale SIGKILL (come da specifica)
	- tratteremo il SIGPIPE nel padre per far si' che questo non vada a terminare in modo anomalo il padre nel caso in cui uno o piu'
		figli abbiano associato un file inensistente o non leggibile 
	- terremo pure in conto del caso in cui sia il file "guida" del padre a non esistere/non essere leggibile. Infatti il padre non 
		terminera' lasciando i figli bloccati in attesa, ma prima mandera' il messaggio di terminazione a tutti i figli */

#include <stdlib.h> 
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/wait.h> 
#include <string.h> 
#include <signal.h>
#include <ctype.h> 

/* definizione del TIPO pipe_t che rappresenta un array di due interi, che useremo per la gestione dei lati delle varie pipe create */
typedef int pipe_t[2];

/* definiamo la funzione handler del segnale SIGPIPE, infatti tratteremo questo segnale in modo tale che non causi la terminazione del padre ma effettui una sola 
	stampa di DEBUG */
void handler(int signo)
{ 
		printf("DEBUG-Il processo padre (pid = %d) ha ricevuto il SIGPIPE (no. %d) perche' uno dei figli ha avuto problemi col proprio file\n", getpid(), signo); 
}

int main (int argc, char **argv)
{
	/*--------------- Variabili locali ---------------*/
	int *pid; 			/* array dinamico che conterra' i pid degli N processi figli creati dal padre. Servira' per l'invio dei SIGKILL e per il 
							recupero dell'indice dei figli e dunque anche il file per la stampa finale del padre */
	int *confronto; 	/* array dinamico inizializzato a tutti 1, che tiene traccia dei figli che hanno rispettato il confronto di tutti i caratteri col padre*/
	int N; 				/* numero di processi figli creati */
	int i, k; 			/* indici dei cicli */
	pipe_t *piped_fp; 	/* array dinamico di N pipe per la comunicazione dei caratteri letti dai processi figli al padre */
	pipe_t *piped_pf; 	/* array dinamico di N pipe per la comunicazione del carattere di controllo DAL padre ai figli */
	char c; 			/* carattere letto dal file del padre da confrontare con quelli mandati dai figli */
	char cf; 			/* carattere letto dai vari figli */
	char token; 		/* carattere di controllo per la sincronizzazione */
	int fdr; 			/* file descriptor legato all'apertura dei file nei vari figli */
	int read_f; 		/* variabile di controllo che verifica che i figli siano effettivamente stati confrontati con il padre */
	int pidFigio; 		/* identificatore dei processi figlio attesi con la wait */
	int status;  		/* variabile di stato per la wait */
	int ritorno; 		/* valore ritornato dai processi figlio */
	int nr, nw; 		/* variabili di controllo per il numero di byte letti/scritti sulle pipe */
	/*------------------------------------------------*/
	
	/* controllo lasco numero di parametri passati */
	if (argc < 4)
	{ 
		printf("Errore: numero di parametri passato errato - Attesi almeno 3 [file1 file2 file_target] - Passati: %d\n", argc - 1); 
		exit(1); 
	}

	/* inizializzazione di N come argc - 2, infatti al numero totale di parametri andiamo a togliere il nome del programma stesso e il nome del file del padre */
	N = argc - 2; 

	/* trattamento del segnale SIGPIPE. Questo infatti potrebbe essere ricevuto dal padre causando una terminazione anomala e dunque un "abbandono" dei figli 
		qualora uno di questi figli abbia associato un file NON esistente o NON leggibile. Infatti il tentativo di invio del token ad un processo oramai terminato 
		provocherebbe il SIGPIPE, dunque decidiamo di trattarlo con un apposita stampa di debug */
	signal(SIGPIPE, handler);
	
	/* allocazione dinamica della memoria per l'array di pid */
	if ((pid = (int *)malloc(N * sizeof(int))) == NULL)
	{ 
		printf("Errore: allocazione memoria con malloc per l'array di pid NON riuscita\n"); 
		exit(2); 
	}

	/* allocazione dinamica della memoria per l'array di controllo */
	if ((confronto = (int *)malloc(N * sizeof(int))) == NULL)
	{
		printf("Errore: allocazione memoria con malloc per l'array di contrllo NON riuscita\n"); 
		exit(3);
	}

	/* inizializziamo i valori di questo array a 1, in modo tale che inizialmente TUTTI i processi figli ricevano il carattere che dia il via libera alla lettura e passaggio del carattere al padre*/
	for (i = 0; i < N; i++)
	{ 
		confronto[i] = 1; 
	}

	/* allocazione dinamica della memroia per gli array di pipe, quello per la comunicazione nel verso padre-figli e quello figli-padre, infatti si ricordi che le pipe sono UNIDIREZIONALI */
	piped_fp = (pipe_t *)malloc(N * sizeof(pipe_t)); 
	piped_pf = (pipe_t *)malloc(N * sizeof(pipe_t));
	if (piped_fp == NULL || piped_pf == NULL)
	{ 
		printf("Errore: allocazione memoria con malloc per gli array di pipe NON riuscita\n"); 
		exit(4); 
	}

	/* creazione delle pipe */
	for (i = 0; i < N; i++)
	{ 
		if (pipe(piped_fp[i]) < 0)
		{ 
			printf("Errore: creazione della pipe per la comunicazione figli-padre di indice %d NON riuscita\n", i);
			exit(5); 
		}

		if (pipe(piped_pf[i]) < 0)
		{
			printf("Errore: creazione della pipe per la comunicazione padre-figli di indice %d NON riuscita\n", i); 
			exit(6); 
		}
	}

	/* ciclo di creazione dei processi figli */
	for (i = 0; i < N; i++)
	{ 
		if ((pid[i] = fork()) < 0)
		{ 
			printf("Errore: fork per la creazione dei figlio di indice %d NON riuscita\n", i); 
			exit(7); 
		}

		if (pid[i] == 0)
		{
			/* CODICE DEI FIGLI */
		
			/* decidiamo che nel caso i processi figli incorrano in qualche tipo di errore si ritorni -1 (255), che sara' dunque dal normale di valore di ritorno, ossia 0 */

			printf("DEBUG-Sono il processo figlio di indice %d e con pid = %d e sto per leggere i caratteri del file %s\n", i, getpid(), argv[i + 1]); 
			
			/* chiudiamo i lati delle pipe che non vengono usati dal figlio Pi. 
				Ogni figlio Pi LEGGE il carattere di controllo inviato dalla pipe di indice i di quelle per la comunicazione padre-figli 
				e SCRIVE il carattere letto da file sulla i-esima pipe di comunicazione figli-padre. Tutti gli altri lati delle pipe 
				potranno dunque essere chiusi */

			for (k = 0; k < N; k++)
			{ 
				close(piped_fp[k][0]); 
				close(piped_pf[k][1]); 

				if (k != i)
				{ 
					close(piped_fp[k][1]); 
					close(piped_pf[k][0]); 	
				}
			}

			/* tentiamo l'apertura del file associato al figlio di indice i */
			if ((fdr = open(argv[i + 1], O_RDONLY)) < 0)
			{ 
				printf("Errore: apertura del file %s da parte del processo figlio di indice %d NON riuscita - file non leggibile o non esistente\n", argv[i + 1], i); 
				exit(-1); 
			}

			/* ciclo di lettura del file amministrato dal controllo del padre */
			read_f = 0;/* andiamo a supporre che il controllo non avvenga, caso in cui il file del padre non si apre e dunque ricevano il 't' senza aver effettivamente spedito nulla al padre */

			while (read(piped_pf[i][0], &token, 1) > 0) 
			{ 
				
				/* il figlio ha ricevuto un messaggio dal padre, effettuiamo il controllo del token */
				if (token == 'v') 
				{
					/* il padre ha dato al figlio il via libera di leggere il carattere corrente dal file. Si noti che il figlio si affida completamente al 
						padre per quanto riguarda le dimensioni del padre, questo perche' per specifica TUTTI i file hanno la stessa dimensione */

					read_f = 1; /* se si arriva almeno una volta qui dentro allora il file del padre funziona */

					read(fdr, &cf, 1); 

					/* comunichiamo al padre il carattere letto */
					write(piped_fp[i][1], &cf, 1); 
				}
				else if (token == 't')
				{ 
					/* padre ha richiesto la terminazione del figlio a causa del raggiungimento della fine del file oppure per un qualche altro problema */
					break; 	/* uscendo dal ciclo il figlio va a terminare in modo normale, infatti 't' viene mandaato solo ai processi che hanno rispettato tutte le corrispondenze
								coi caratteri letti dal padre dal suo file associato */
				}
			}

			/* sottigliezza: nel caso in cui il file del padre non esista i processi figli ricevono il 't' ma ritornano 0, quando in realta' non dovrebbero, dal momento che non hanno
			effettivamente verificato che il file sia lo stesso. Pero ovviare alla cosa basta guardare il valoe di read_f, se questo e' 0 allora il file del padre non esisteva e dunque 
			i figli possono tornare -1 */
			if (!read_f)
			{
				exit(-1); 
			}

			exit(0); 
		}
	}

	/* CODICE DEL PADRE */

	/* chiudiamo i vari lati non usati delle pipe. Il padre infatti SCRIVE su tutte le pipe di piped_pf e legge da TUTTE le pipe di piped_fp */
	for(i = 0; i < N; i++)
	{ 
		close(piped_fp[i][1]); 
		close(piped_pf[i][0]); 
	}

	/* tentiamo l'apertura del file associato al padre. A questo punto andiamo nel caso in cui il padre NON riesca ad aprire questo file ci sara' bisogno che tutti i figli ESCANO 
		dal ciclo di sincronizzazione se no tutto andrebbe in deadlock. Dunque facciamo si' che il padre vada a mandare il carattere 't' per indurre la loro terminazione. Decidiamo 
		di non inviare un segnale perche' a tutti gli effetti possiamo vedere questo caso come una terminazione normale*/
	if ((fdr = open(argv[argc - 1], O_RDONLY)) < 0)
	{ 
		printf("Errore: apertura del file %s da parte del padre NON riuscita - file NON leggibile o NON esistente\n", argv[argc - 1]); 
		/* exit(8); 	decidiamo di non uscire per permettere al padre di attendere i figli creati che stiamo per terminare */

		/* invio del carattere di terminazione dei figli */
		for (i = 0; i < N; i++)
		{ 
			token = 't'; 
			nw = write(piped_pf[i][1], &token, 1);

			if (nw != 1)	
			{
				/* questo significa che anche il figlio di indice i ha avuto un problema col proprio file e dunque sia gia' terminato. Allora il padre riceve il SIGPIPE (che abbiam trattato),
					e quindi NON riesce a scrivere sulla pipe */
				printf("Errore: scrittura del padre sulla pipe del figlio di indice %d NON riuscita\n", i); 
				/* exit(9); 	per lo stesso motivo di prima decidiamo di NON terminare il processo padre */
			}
		}
	}
	else
	{ 
		/* caso in cui il processo padre e' effettivamente riuscito ad aprire il file e puo' dunque iniziare il ciclo di lettura */
		token = 'v'; 	/* da ora fino alla fine della lettura inviera' solo dei via libera, infatti a tutti i processi che non rispettano il confronto smettera' direttamente di 
							inviare il controllo, lasciandoli in attesa per poi andarli a termianare con il SIGKILL alla fine della lettura */

		while (read(fdr, &c, 1) > 0)
		{ 
			for (i = 0; i < N; i++)		/* iteriamo sui figli, controllando contestualmente il carattere letto da loro nella stessa posizione e confrontando corrisponda con quello del padre*/
			{ 
				if(confronto[i])
				{
					/* se il confronto per ora e' tutto corretto andiamo ad inviare il via libera */ 
					nw = write(piped_pf[i][1], &token, 1);

					/* anche in questo caso controlliamo che la scrittura sia riuscita, infatti come prima se il processo Pi ha avuto problemi con proprio file terminando, noi stiamo 
						per ottenere un SIGPIPE e quindi la scrittura fallira'. Decidiamo pero' anche di non passare direttamente all'iterazione successiva, tanto tramite il controllo
						della read necessariamente fallira', quindi tanto vale che si faccia arrivare il codice a quel punto, dove effettivamente verra' aggiornato il valore di confronto[i] 
						*/
					if (nw != 1) 
					{ 
						printf("Errore: scrittura del padre sulla pipe del figlio di indice %d NON riuscita\n", i);
					}

					nr = read(piped_fp[i][0], &cf, 1); 
					if (nr == 1) 
					{ 
						if (c != cf)
						{ 
							/* caso in cui il carattere letto dal figlio e' DIVERSO da quello del padre, allora dobbiamo smettere di inviargli il via libera, aggiornando il suo elemento del 
								array confronto */
							confronto[i] = 0; 
						}

						/* nel caso in cui c == cf non bisogna fare nulla, infatti i valori di confronto li abbiamo preinizializzati a 1 */
					}
					else
					{
						printf("Errore: lettura del padre dalla pipe del figlio di indice %d NON riuscita\n", i); 
						
						confronto[i] = 0;	 /* effettivamente e' vero che non si e' neanche fatto il confronto, ma cosi' facendo evitiamo ad ogni iterazione di andare a provare a comunicare con
												un figlio terminato. Nota che impostare a 0 il valore di confronto fa si' che poi si andra' ad inviare un SIGKILL ad un processo inesistente, ma la 
												cosa non crea dei problemi. Semplicmente la kill tornera' -1, che possiamo andare dunque a spiegare */
					}
				}
			}
		}

		/* arrivati a questo punto il padre ha terminato la lettura del suo file e puo' quindi andare a: 
			- terminare correttamente i processi che hanno sempre letto caratteri giusti con il token 't'
			- terminare in modo anomalo gli altri processi con il SIGKILL 
		   per discriminare i due casi bastera' utilizzare i valori dell'array confronto */
		
		token = 't'; 

		for (i = 0; i < N; i++)
		{ 
			if (confronto[i])
			{
				write(piped_pf[i][1], &token, 1); 
			}
			else
			{
				printf("stiamo per killare il processo di pid = %d\n", pid[i]); 
				int ret = kill(pid[i], SIGKILL); 
				printf("la kill ha tornato %d\n", ret); 
				if (ret == -1) 
				{
					/* ci troviamo nel caso in cui siamo andato a mandare una SIGKILL ad un processo gia' terminato */
					printf("DEBUG-Inviato un SIGKILL al processo figlio di indice %d e pid = %d che era gia' terminato\n", i, pid[i]); 
				}
			}
		}

	}

	/* ciclo di attesa dei processi figli, con recupero dei loro valori di ritorno e stampa di quanto richiesto dal testo */
	for (i = 0; i < N; i++)
	{ 
		if((pidFigio = wait(&status)) < 0)
		{ 
			printf("Errore: wait di attesa di uno dei figli NON riuscita\n"); 
			exit(10); 
		}

		if (WIFEXITED(status))
		{ 
			/* recuperiamo l'indice del figlio associato a questo pid, cosi' da poter associarli correttamente il file come da specifica */
			int idx; 	/* variabile in cui salviamo l'indice in questione */
			for (k = 0; k < N; k++)
			{
				if (pid[k] == pidFigio)
				{
					idx = k; 
					break; 
				}
			}

			ritorno = WEXITSTATUS(status); 
			/* distinguiamo il caso di effettiva riuscita del processo da quello di errore */
			if (ritorno == 0)
			{ 
				printf("Il processo figlio di indice %d e pid = %d ha ritornato il valore %d ed ha dunque verificato che il file %s "
						"fosse identico al file %s\n", idx, pidFigio, ritorno, argv[idx + 1], argv[argc - 1]); 
			}
			else
			{ 
				printf("Il processo figlio con pid = %d ha ritornato %d, dunque ci sono stati dei problemi\n", pidFigio, ritorno); 
			}
		}
		else 
		{
			printf("Il processo figlio con pid %d e' terminato in modo anomalo\n", pidFigio); 
		}
	}
	exit(0); 
}
