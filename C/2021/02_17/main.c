/* programma C main.c che risolve la parte C della prova totale di esame tenutasi nel 17 febbraio 2021 */

#include <stdlib.h> 
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/wait.h> 
#include <string.h> 
#include <ctype.h> 

/* definizione del TIPO pipe_t che rappresenta un array di due interi, che useremo per la gestione dei lati delle varie pipe create */
typedef int pipe_t[2];

int main (int argc, char **argv)
{
	/*--------------- Variabili locali ---------------*/
		int pid; 			/* identificatore dei processi figli generati con fork e attesi con wait */
		int Q; 				/* numero di file passati e analogamente di figli creati */
		int q, k; 			/* indici per i cicli */
		pipe_t *piped; 		/* array dinamico di Q pipe per la comunicazione/sincronizzazione dei processi figli: 
							attraverso queste pipe i processi figli implementano uno schema di comunicazione a pipe RING, dove il primo processo comunica al secondo, il secondo 
							al terzo e via dicendo... fino ad arrivare all'ultimo processo di indice Q - 1 che invia di nuovo al PRIMO PROCESSO. Dunque il generico processo Pq: 
							- legge dalla pipe di indice q 
							- scrive sulla pipe di indice (q + 1) % Q. Il %Q serve per far si' che l'ultimo processo torni a scrivere sulla pipe 0 e non sulla pipe inesistente di indice Q */
		int fdr; 			/* file descriptor usato per le aperture dei file nei vari figli */
		int cnt; 			/* numero di occorrenze di caratteri numerici nella singola linea */
		char linea[256]; 	/* buffer di caratteri che useremo per leggere le righe dei vari file, per poi stamparle */
		char ctrl = 'S'; 	/* carattere di controllo che si passano i vari processi per sincronizzarsi. Inizialmente spedito al processo P0 dal padre. Nota che non era necessario
							inizializzare il valore di questa variabile, perche' tanto non verra' effettivamente considerato da nessun processo */
		int nr, nw; 		/* variabili di controllo per le letture/scritture sul pipe ring */
		int status; 		/* variabile di stato per la wait */
		int ritorno; 		/* valore ritornato dai vari processi figli */
	/*------------------------------------------------*/
	
	/* controllo lasco numero di parametri passato */
	if (argc < 3)
	{
		printf("Errore: numero di parametri passato errato - Attesi almeno 2 (2 nomi di file) - Passati %d\n", argc - 1);
		exit(1);  
	}

	Q = argc - 1; 	/* inizializziamo il valore di Q come numero di file passati, ottenuto sottraendo dal numero totale di parametri 1, ossia il nome dell'eseguibile stesso */

	/* allocazione dinamica della memoria per l'array di pipe */
	if ((piped = (pipe_t *)malloc(Q * sizeof(pipe_t))) == NULL)
	{
		printf("Errore: allocazione della memoria con malloc per l'array di pipe non riuscita\n"); 
		exit(2);
	}

	/* ciclo di creazione delle Q pipe */
	for (k = 0; k < Q; k++)
	{ 
		if (pipe(piped[k]) < 0) 
		{
			printf("Errore: creazione della pipe di indice %d non riuscita\n", k); 
			exit(3); 
		}
	}

	/* ciclo di creazione dei processi figli */
	for (q = 0; q < Q; q++)
	{
		if ((pid = fork()) < 0) 
		{ 
			printf("Errore: creazione con fork del processo figlio di indice %d NON riuscita\n", q); 
			exit(4); 
		}

		if(pid == 0) 
		{ 
			/* CODICE DEI FIGLI */
			
			/* qualora un processo figlio incorresse in qualche tipo di errore, decidiamo di ritornare il valore -1 (255), che sara' possibile distinguere da 
				il normale valore di ritorno dei processi figli, perche' il teso specifica che il numero di occorrenze per riga sia STRETTAMENTE minore di 255 */

			printf("DEBUG-Sono il processo figlio di indice %d e con pid = %d e sto per contare le occorrenze dei caratteri nuemerici in ogni riga "
					"del file %s\n", q, getpid(), argv[q + 1]); 
			
			/* abbiamo detto che ogni figli Pq legge dalla pipe di indice q e scrive su quella di indice (q + 1) % Q, dunque tutti i lati che non servono possiamo chiuderli */
			for (k = 0; k < Q; k++)
			{ 
				if (k != q)
				{ 
					close(piped[k][0]); 
				}

				if (k != ((q + 1) % Q))
				{ 
					close(piped[k][1]); 
				}
			}

			/* tentiamo l'apertura del file associato al processo di indice q */
			if ((fdr = open(argv[q + 1], O_RDONLY)) < 0)
			{ 
				printf("Errore: apertura del file %s da parte del figlio di indice %d non riuscita - file non esistente o non leggibile\n", argv[q + 1], q); 
				exit(-1); 
			}

			/* ciclo di lettura carattere per carattere del file appena aperto, con sincronizzazione via pipe ring coi processi fratelli */
			k = 0; 		/* inizializziamo a 0 l'indice di scrittura del buffer */
			cnt = 0; 	/* inizializziamo analgoamente a 0 anche il contatore di caratteri numerici */	
			while (read(fdr, &(linea[k]), 1) > 0)
			{ 
				if (linea[k] == '\n')	/* ci troviamo alla fine di una riga, allora prima di stampare dobbiamo attendere il messaggio dal processo precedente */
				{ 
					/* trasformiamo la linea trovata in una stringa, cosi' che possa essere stampata come tale tramite la printf. Per farlo sovrascriviamo il \n, cosi' da non avere
					problemi con gli "a capo" nella printf */
					linea[k] = '\0'; 

					/* attesa del via libera da parte del processo precedente */
					nr = read(piped[q][0], &ctrl, 1); 

					/* controllo che sia stato letto correttamente, cosa molto utile, in quanto in caso in cui il processo precedente avesse avuto un qualche problema col 
						file associato, la cosa comporterebbe una mancata scrittura sulla pipe e dunque nr = 0 */
					if (nr != 1) 
					{ 
						printf("Errore: lettura da pipe ring del carattere di controllo da parte del processo di indice %d non riuscita - no. di byte letti: %d\n", q, nr); 
						exit(-1); 
						/* cosi' facendo non andiamo a scrivere al processo successivo, che quindi terminera', innestando una terminazione a catena di tutti i processi figli */
					}

					/* stampa richiesta dal testo */
					printf("Il figlio con indice %d e pid = %d ha letto %d caratteri numerici nella linea: '%s'\n",  q, getpid(), cnt, linea); 

					/* invio del carattere di controllo al processo successivo con controllo sul corretto invio */
					nw = write(piped[(q + 1) % Q][1], &ctrl, 1); 

					if (nw != 1) 
					{
						printf("Errore: scrittura su pipe ring del carattere di controllo da parte del processo di indice %d non riuscita - no. di byte scritti: %d\n", q, nw); 
                        exit(-1);  
					}

					/* reinizializziamo i vari parametri associati alla linea perche' passiamo a quella successiva */
					k = 0; 
					ritorno = cnt; 		/* ci salviamo il numero di occorrenze della linea attuale, cosi' che se siamo sull'ultima riga non lo perdiamo e lo possiamo passare al padre*/
					cnt = 0; 
				} 
				else	/* caso in cui siamo su un carattere normale */
				{ 
					if (isdigit(linea[k]))
					{
						cnt++;
					}

					k++;
				}
			}
			
			exit(ritorno); 
		}
	}

	/* CODICE DEL PADRE */

	/* il processo padre concretamente deve scrivere solo sulla pipe 0 per dare il via alla innescare il ciclo dei figli, pero' della prima pipe tiene aperto anche il lato di lettura.
		questo perche' quando un processo ha stampato l'ultima riga ovviamente invia il messaggio di controllo al successivo per permettere anche a questo la stampa, pero' nel caso del
		processo di indice Q - 1, questo tenterebbe di scrivere sulla pipe 0 al processo P0, che oramai sara' gia' terminato, dunque tentando la scrittura su una pipe senza lettore 
		riceverebbe il SIGPIPE e terminerebbe in modo anomalo. 
		Allora per evitare la cosa facciamo si' che il padre diventi un lettore fittizio per questa pipe in modo tale che il processo PQ-1 possa inviare il messaggio senza essere killato*/
	
	for (k = 1; k < Q; k++)		/* k dunque parte da 1, perchè la pipe 0 non va toccata */
	{ 
		close(piped[k][0]); 
		close(piped[k][1]);
	}

	/* invio del carattere di controllo che da il via al ciclo dei figli */
	nw = write(piped[0][1], &ctrl, 1); 

	/* controllo che sia stato possibile inviare*/
	if (nw != 1) 
	{ 
		printf("Errore: scrittura da parte del padre sul pipe ring NON riuscita - no. di byte scritti %d\n", nw);
		/* exit(5)    non effettuiamo l'uscita per far si' che il padre possa attendere i figli */ 
	}

	/* a questo punto il lato scrittura della pipe 0 e' inutile e possiamo dunque chiuderlo */
	close(piped[0][1]); 

	/* attesa dei Q processi figli con recupero del valore tornato e stampa */
	for (q = 0; q < Q; q++)
	{ 
		if ((pid = wait(&status)) < 0)
		{
			printf("Errore: wait del padre NON riuscita\n");
			exit(6); 
		}

		if (WIFEXITED(status))
		{
			ritorno = WEXITSTATUS(status); 
			printf("Il processo figlio con pid = %d e' terminato e ha ritornato il valore %d (se 255 e' incorso in qualche problema durante l'esecuzione)\n", pid, ritorno); 
		}
		else 
		{	
			printf("Il processo figlio con pid = %d e' terminato in modo anomalo\n", pid); 
		}
	}
	exit(0); 
}
