/* programma main2.c che e' sostanzialmente una rivisitazione di main.c dell'esame del 17 febbraio 2021 che utilizza un differenti protocollo di sincronizzazione tra i processi,
che sostanzialmente ad evitare che il processo PQ-1 riceva il SIGPIPE.

Infatti nel programma main.c quello che si fa per ogni processo e' andare ad iterare sulla lettura carattere per carattere da file e qualora si finisca la riga corrente, ci si mette 
in attesa del carattere di controllo (atto alla sincronizzazione tra processi fratelli) dal figlio di indice precedente. Ma cosi' inevitabilmente l'ultimo processo, letta l'ultima riga 
del file, prova a scrivere un messaggio INUTILE al processo P0 quando questo potenzialmente potrebbe gia' essere terminato. 

Dunque la grossa modifica che comporta questo nuovo codice sta nel fatto che l'iterazione che si ha in OGNI figlio sara' sulla lettura del carattere di controllo. Quindi, ogni figlio, 
prima ancora di mettersi a contare le occorrenze numeriche della riga corrente si mette in attesa del byte di controllo; Solo quando questo verra' ricevuto, dara' il via all'esecuzione 
del conteggio di occorrenze nella linea, la stampa e infine il messaggio di controllo al processo precedente.

Ma come fa cio' ad impedire l'accdimento del SIGPIPE dell'ultimo processo?? semplice... il modo in cui terminera' il ciclo di lettura NON sara' piu' dettato da una read da file che ritorna
0, ma da una read da pipe che ritorna 0 a causa della terminazione del processo a monte. Infatti, quando il processo P0 avra' esaurito la lettura da file fara' un BREAK dal ciclo e finira' 
per terminare. Cosi' facendo il processo successivo, si trovera' ad attenedere un messaggio da una pipe SENZA SCRITTORE, dunque la read tornera' 0 e il ciclo terminera' anche per questo 
processo. E via dicendo fino all'ultimo processo, che dunque si trovera' a terminare l'iterazione senza mandare NESSUN messaggio superfulo a P0. 

Nota dunque che questa implementazione NON richiedera' da parte del padre di andare a lasciare aperto il lato di lettura della pipe0 

Nota poi che anche in questa versione il problema del SIGPIPE per il caso di file inenistenti e' inevitabile e quindi andiamo anche qui ad ignorare il SIGPIPE 

NB: noi qui stiamo dando per scontato che i file associtai ai processi abbiano TUTTI la stessa lunghezza in linee, e questo e' fondamentale per la riuscita del programma, infatti la terminazione 
del ciclio di OGNI processo e' dettata dal fatto che il solo processo P0 a monte della catena termini la sua lettura. Qualora i file associati agli altri processi avessero piu' righe, queste 
verrebbero IGNORATE */

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
		int finito; 		/* variabile utilizzata per discriminare la fine della lettura da file */
		int nw; 			/* variabile di controllo per la scrittura su pipe. Il controllo sulla lettura e' implicito nella condizione del while */
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

	/* ingnoriamo il segnale SIGPIPE (caso in cui vengono passati dei file non esistenti)*/
	signal(SIGPIPE, SIG_IGN); 

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

			/* ciclo di sincronizzazione tra processi figli, con conteggio e stampa della riga corrente */
			while (read(piped[q][0], &ctrl, 1) > 0)		/* qualora venga effettuata questa read allora si ha il via libera per procedere al conteggio dei caratteri */
			{ 
				finito = 1; 	/* ipotizziamo che il file sia finito, per poi andare a TENTARE la lettura da file e se questa riesce allora andremo ad aggiornare la 
									variabile a 0 */
				cnt = 0; 		/* inizializziamo a 0 il contatore di occorrenze, perche' siamo su una nuova riga */
				k = 0; 			/* inizializziamo a 0 l'indice del buffer per poter scrivere dal suo inizio */
				while (read(fdr, &(linea[k]), 1) > 0)
				{ 
					finito = 0; 	/* file NON e' terminato */

					if (linea[k] == '\n')		/* lettura riga terminata */
					{ 
						/* trasformiamo il buffer in una stringa, rimuovendo il terminatore di riga \n */
						linea[k] = '\0';

						break; 
					}

					if (isdigit(linea[k]))
					{
						cnt++; 	/* se abbiamo trovato un carattere numerico aumentiamo il numero di occorrenze*/
					}

					k++; 	/* incremento dell'indice di scrittura nella linea affinche' non si sovrascrivano aratteri*/
				}

				if (finito)
				{
					break; 		/* se si ha finito il file non c'e' bisogno di fare NULLA, semplicemente si esce dal ciclo di sincronizzazione 
									e si termina il processo, cosi' implicitamente anche tutti i successivi termineranno perche' via via le varie 
									read da pipe torneranno 0 e non un valore > 0 */
				}

				/* stampa richiesta dal testo */
				printf("Il figlio con indice %d e pid = %d ha letto %d caratteri numerici nella linea: '%s'\n",  q, getpid(), cnt, linea); 

				/* invio del messaggio di controllo al figlio successivo */
				nw = write(piped[(q + 1) % Q][1], &ctrl, 1); 

				/* controllo che l'invio abbia avuto successo */
				if (nw != 1) 
				{ 
					printf("Errore: scrittura su pipe ring del carattere di controllo da parte del processo di indice %d non riuscita - no. di byte scritti: %d\n", q, nw); 
                    exit(-1);  
				}				
				
				/* salviamo il valore del conteggio nel caso si trattasse dell'ultima riga del file */
				ritorno = cnt; 
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
	
	for (k = 0; k < Q; k++)		/* k dunque parte da 1, perchè la pipe 0 non va toccata */
	{ 
		close(piped[k][0]); 
		
		if (k != 0)
		{
			close(piped[k][1]);
		}
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
