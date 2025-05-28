/* Programma C main.c che risolve la parte C della prova di esame totale tenutasi il 25 gennaio 2023 */

#include <stdlib.h> 
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/wait.h> 
#include <string.h> 
#include <ctype.h>
#include <signal.h> 

/* Definizione dei bit dei permessi (in ottale) per gli eventuali file creati */
#define PERM 0644 	

/* Definizione del TIPO pipe_t che rappresenta un array di due interi, che useremo per la gestione dei lati delle varie pipe create */
typedef int pipe_t[2];

int main (int argc, char **argv)
{
	/*---------------- Variabili locali -----------------*/
	int pid; 			/* identificatore dei processi figli creati con fork e attesi con wait */
	int N; 				/* numero file passati per i figli e dunque numero dei processi figli */
	int n; 				/* indice dei processi figli */
	int k; 				/* indice generico per i cicli */
	int fdw; 			/* file descriptor realtivo all'apertura in scrittura del file usato dal padre */
	int fdr; 			/* file descriptor relativo all'apertura in lettura dei vari file associati ai figli */
	char chs[2]; 		/* buffer di 2 caratteri usato per la comunicazione dei caratteri letti dai figli al padre */
	pipe_t *piped; 		/* array dinamico di pipe per la comunicazione figli-padre */
	int finito; 		/* variabile di controllo utilizzata dal padre per regolare il suo ciclo di recuper delle informazioni dai figli: 
							- finito == 0 significa il esiste ancora una pipe di un qualche figlio da cui recuperare dei caratteri 
							- finito == 1 significa che il processo padre ha effettivamente recueperato tutti i caratteri da tutti i figli */
	int status; 		/* variabile di stato per la wait */
	int ritorno; 		/* valore ritornato dai processi figli */
	int nr, nw; 		/* variabili di controllo per lettura/scrittura da/su pipe */
	/*---------------------------------------------------*/
	
	/* Controllo lasco numero di parametri */
	if (argc < 4)
	{ 
		printf("Errore: numero di parametri passato a %s errato - Attesi almeno 3 [filename1 filename2 ... filepadre] - Passati: %d\n", argv[0], argc - 1); 
		exit(1);
	}

	/* Calcolo del numero di file passati per i processi figli */
	N = argc - 2; 

	/* Allocazione dinamica della memoria per l'array di pipe */
	if ((piped = (pipe_t *)malloc(N * sizeof(pipe_t))) == NULL)
	{ 
		printf("Errore: allocazione della memoria per l'array di pipe non riuscita\n");
		exit(2); 
	}

	/* Creazione delle N pipe per la comunicazione figli-padre */
	for (k = 0; k < N; k++)
	{ 
		if (pipe(piped[k]) < 0)
		{ 
			printf("Errore: creazione della pipe di indice %d non riuscita\n", k);
			exit(3);
		}
	}

	/* Come richiesto dalla specifica il processo padre apre il file a lui associato in scrittura prima della creazione dei figli */
	if ((fdw = open(argv[argc - 1], O_WRONLY)) < 0)
	{ 
		printf("Errore: apertura in scrittura del file %s da parte del padre non riuscita\n", argv[argc - 1]); 
		exit(4);
	}

	/* Ciclo di creazione dei figli */
	for (n = 0; n < N; n++)
	{ 
		if ((pid = fork()) < 0)
		{ 
			printf("Errore: fork di creazione del processo figlio con indice %d non riuscita\n", n); 
			exit(5); 
		}

		if (pid == 0)
		{ 
			/* CODICE DEI FIGLI */

			/* Si decide che i vari procssi figli nel caso incorrano in qualche errrore ritornino al padre il valore -1 (255 senza segno), in modo tale che la terminazione errata possa essere
				distinta dai casi normali, in cui viene ritornato il numero totale di caratteri letti, che per specifica risulta essere strettamente minore di 255 */
			
			printf("DEBUG-Sono il processo figlio di indice %d e con pid = %d e sto per leggere il file %s\n", n, getpid(), argv[n + 1]); 

			/* Ogni processo figlio Pn si limita a scrivere sul lato di scrittura della pipe di indice n, di conseguenza tutti gli altri lati non utilizzati possono essere chiusi */
			for (k = 0; k < N; k++)
			{ 
				close(piped[k][0]);

				if (k != n)
				{
					close(piped[k][1]); 
				}
			}

			/* Apertura del file associato al figlio Pn in lettura */
			if ((fdr = open(argv[n + 1], O_RDONLY)) < 0)
			{ 
				printf("Errore: apertura del file %s da parte del figlio di indice %d non riuscita - file non leggibile oppure inesistente\n", argv[n + 1], n); 
				exit(-1); 
			}
			
			/* Ciclo di lettura del file associato al figlio 2 CARATTERI ALLA VOLTA */
			
			/* Inizializzazione del valore di ritorno a 0, questo valore ad ogni lettura verra' incrementato del numero di caratteri letti, cosi' che alla fine della lettura rappresenti 
				il conteggio totale di crattere letti da file */
			ritorno = 0; 
			while ((nr = read(fdr, chs, 2)) > 0) 
			{ 
				/* Scrittura dei byte letti sulla pipe piped[n] cosi' che possano essere letti dal padre. Si noti che non ci viene assciurata che la lunghezza del file sia pari, quindi 
					sara' possibile che alla fine avvenga una lettura di 1 singolo carattere. Alla luce di cio' sara' necessario andare a scrivere sulla pipe di comunicazione SOLO nr 
					byte e non 2, cosi' che nel caso venga letto 1 singolo carattere, venga contestualmente spedito al padre un solo carattere */
				nw = write(piped[n][1], chs, nr); 

				/* Controllo byte scritti */
				if (nw != nr)
				{ 
					printf("Errore: il processo figlio di indice %d ha letto %d caratteri da file ma ne ha scritti sulla pipe solo %d\n", n, nr, nw);
					exit(-1); 
				}

				/* Aggiornamento contatore di caratteri letti */
				ritorno += nr; 
			}

			exit(ritorno); 
		}
	}

	/* CODICE DEL PADRE */

	/* Il processo padre si comporta come lettore per tutti i processi figli, senza scrivere su nessuna pipe. E' dunque possibile andare a chiudere tutti i lati di scrittura delle pipe */
	for (k = 0; k < N; k++)
	{ 
		close(piped[k][1]); 
	}

	/* Ciclo di recupero dei vari caratteri spediti dai figli secondo l'ordine dei file. Ad ongi iterazione viene dunque letta una coppia di caratteri da ogni figlio a seconda dell'ordine 
		dei file, e il ciclo deve andare avanti finche' ci sia ancora una pipe contenente dei caratteri */

	/* Inizializzazione di finito a 0, si suppone dunque che ci sia un qualche figlio di cui si debbano leggere caratteri, dunque si entra nel ciclo */
	finito = 0; 
	while (!finito)
	{ 
		/* Si suppone adesso che tutti i figli siano terminati. Qualora il padre riesca a leggere dei caratteri da una delle pipe il valore verra' riportato a 0 */
		finito = 1; 

		for (k = 0; k < N; k++)
		{ 
			/* Lettura di due caratteri dalla pipe del processo k-esimo. Analogamente a quanto detto per i figli, dal momento che i file potrebbero avere lunghezza in caratteri dispari, e'
				dunque possibile che l'ultima lettura da parte di un processo sia di 1 solo carattere. Per questo motivo anche nel padre la stampa su file sara' di nr caratteri e non 2 */
			nr = read(piped[k][0], chs, 2); 

			if (nr > 0)		/* Se sono stati letti caratteri da pipe */
			{ 
				/* Scrittura dei nr byte letti sul file aperto dal padre in scirttura */
				nw = write(fdw, chs, nr); 

				/* Controllo sulla scirttura */
				if (nw != nr)
				{ 
					printf("Errore: il processo padre ha scritto sul file %s un numero di byte (%d) differente da quelli letti dalla pipe del figlio di indice %d (%d)\n", argv[argc - 1], nw, k, nr); 
					/* exit(6); 	Si decide di non terminare, affinche' questo possa attendere tutti i figli */
				}

				/* Si riporta a 0 il valore di finito perche' si ha trovato una pipe con ancora dei caratteri da leggere */
				finito = 0; 
			}

			/* Altrimenti se sono stati letti 0 byte significa che il processo corrente e' terminato, quindi non altera lo stato di finito */
		}
	}

	/* Ciclo di attesa dei processi figli con recupero e stampa del valore tornato */
	for (n = 0; n < N; n++)
	{
		if ((pid = wait(&status)) < 0)
		{ 
			printf("Errore: wait di attesa di uno dei figli non riuscita\n");
			exit(7); 
		}

		if ((status & 0xFF) != 0)
		{ 
			printf("Il processo figlio con pid = %d e' terminato in modo anomalo\n", pid);
		}
		else 
		{ 
			ritorno = (int)((status >> 8)  & 0xFF); 
			printf("Il processo figlio con pid = %d ha ritornato il valore %d (se 255 e' incorso in qualche probelma durante l'esecuzione)\n", pid, ritorno); 
		}
	}
	exit(0); 
}
