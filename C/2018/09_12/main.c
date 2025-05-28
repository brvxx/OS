/* programma C main.c che risolve la parte di C dell'esame totale tenutosi il 12 settembre 2018 */

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
	int pid; 		/* identificatore dei processi figli creati con la fork e attesi con la wait */
	int N; 			/* numero di file passati o analogamente di processi figlio creati */
	int i, k; 		/* indici per i cicli */ 
	pipe_t *piped_f; 	/* array dinamico per le N pipe di comunicazione tra padre e figli */
	pipe_t *piped_n; 	/* array dinamico per le N pipe di comunicazione tra padre e nipoti */
	long int trans; 	/* numero di trasformazioni effettuate dai figli e dai nipoti */ 
	char c; 		/* singolo carattere letto dai figli/nipoti */ 
	int fd; 		/* file descriptor per l'apertura dei file da parte dei figli e dei nipoti */
	int nr; 		/* variabile per il controllo dei caratteri letti da pipe */
	int status; 		/* variabile di stato per la wait */
	int ritorno; 		/* valore ritornato dai figli e nipoti */ 
	/*------------------------------------------------*/
	
	/* controllo lasco numero di parametri passato */ 
	if (argc < 3) 
	{ 
		printf("Errore: numero di parametri passato errato - Attesi almeno 2 (2 file) - Passati %d\n", argc - 1); 
		exit(1); 
	} 

	N = argc - 1; 	/* inizializzazione di N (numero di processi figlio) come numero totale di parametri meno 1, ossia il nome dell'eseguibile stesso */

	/* allocazione dinamica della memoria per i due array di pipe */ 
	piped_f = (pipe_t *)malloc(N * sizeof(pipe_t)); 
	piped_n = (pipe_t *)malloc(N * sizeof(pipe_t)); 
	if ((piped_f == NULL) || (piped_n == NULL))
	{ 
		printf("Errore: allocazione dinamica della memoria per gli array di pipe non riuscita\n");
		exit(2); 
	} 

	/* ciclo di creazione delle N pipe di comunicazione padre-figli e le N per comunicazione padre-nipoti */ 
	for (i = 0; i < N; i++)
	{ 
		if (pipe(piped_f[i]) < 0) 
		{ 
			printf("Errore: creazione della pipe di indice %d per la comunicazione padre-figlio NON riuscita\n", i); 
			exit(3); 
		} 
	
		if (pipe(piped_n[i]) < 0) 
		{ 
			printf("Errore: creazione della pipe di indice %d per la comunicazione padre-nipote NON riuscita\n", i); 
			exit(4); 
		} 
	}

	/* ciclo di creazione dei processi figli */
	for (i = 0; i < N; i++) 
	{ 
		if ((pid = fork()) < 0) 
		{ 
			printf("Errore: creazione del processo figlio di indice %d NON riuscita con la fork\n", i);
			exit(5); 
		}
		
		if (pid == 0) 
		{ 
			/* CODICE DEI FIGLI */ 

			printf("DEBUG-Sono il processo figlio di indice %d e pid = %d e sto per cercare tutte le occorrenze di caratteri numerici " 
				"nel file %s per poi trasformarle in degli spazio\n", i, getpid(), argv[i + 1]); 

			/* decidiamo di ritornare il valore -1 in caso in cui i figli incorrano in qualche tipo di errore, in modo tale che possa essere
			distinto dai normali valori tornati da questo (0, 1, 2) */ 

			/* chisura dei lati delle pipe che non servono al processo figlio Pi, all'interno delle N pipe di comunicazione col padre. 
			Attenzione che non e' possibile chiudere le pipe relative alla comunicazioen nipote-padre, se no il nipote generato dal figlio
			non avra' poi modo di comunicare col padre */ 

			for (k = 0; k < N; k++)
			{
				/* chisura dei lati di lettura delle pipe padre-figli */
				close(piped_f[k][0]); 

				/* chiusura dei lati dei scrittura delle pipe padre-figli TRANNE quella di indice i che servira' al processo Pi */
				if (k != i)
				{ 
				 	close(piped_f[k][1]); 
				}
			} 
			
			/* creazione del processo nipote */ 
			if ((pid = fork()) < 0) 
			{ 
				printf("Errore: creazione con fork del processo nipote da parte del figlio %d NON riuscita\n", i); 
				exit(-1); 
			}

			if (pid == 0) 
			{ 
				/* CODICE DEI NIPOTI */ 

				printf("DEBUG-Sono il processo nipote, figlio del processo figlio di indice %d e con pid = %d, e sto per cercare " 
					"nel file %s tutte le occorrenze dei caratteri alfabetici minuscoli, per poi renderli maiuscoli\n", i, getpid(), argv[i + 1]);
				
				/* per lo stesso motivo riportato nel codice dei figli decidiamo di ritornare il valore -1 (255) in caso in cui i nipoti 
				incorrano in qualche tipo di errore */ 

				/* chisura dei lati di tutte le pipe non usate dai nipoti. I nipoti infatti si comporteranno solo da scrittori per le pipe con 
				il loro stesso indice tra quelle per la comunicazione padre-nipoti */ 
				for (k = 0; k < N; k++)
				{ 
					close(piped_f[k][0]); 
					close(piped_f[k][1]);
					close(piped_n[k][0]); 

					if (k != i) 
					{ 
						close(piped_n[k][1]); 
					} 
				} 

				/* tentiamo l'apertura del file associato al nipote, sia in lettura che scrittura */ 
				if ((fd = open(argv[i + 1], O_RDWR)) < 0) 
				{ 
					printf("Errore: il processo nipote di indice %d non e' riuscito ad aprire il file %s (file non esiste oppure non e' "
						"leggibile o scrivibile\n", i, argv[i + 1]); 
					exit(-1); 
				} 

				/* ciclo di lettura del file, carattere per carattere, con trasformazione del carattere come da specifica */ 
				trans = 0L; 	/* inizializziamo a 0 il numero di trasformazioni effettuate */
				while (read(fd, &c, 1) > 0) 
				{ 
					if (islower(c))
					{
						/* trovato carattere numerico */ 

						/* riportiamo indietro il puntatore, cosi' che punti a questo carattere e possa essere sostituito */ 
						lseek(fd, -1L, SEEK_CUR); 
						
						/* trasformiamo il carattere minuscolo trovato nel corrispettivo maiuscolo */
						c = toupper(c); 

						/* andiamo ad effettuare come da specifica una stampa di un carattere spazio */ 
						write(fd, &c, 1); 

						trans++; 
					} 
				} 

				/* comunichiamo al padre attraverso la i-esima pipe padre-nipoti il numero totale di trasformazioni effettuate */ 
				write(piped_n[i][1], &trans, sizeof(trans)); 

				/* in base al numero di trasformazioni effettuate nel nipote, determiniamo che valore tornare al figlio */ 
				ritorno = (int) (trans / 256); 
				
				exit(ritorno); 
			}

			/* ritorno al codice dei figli */

			/* solo a questo punto possiamo chiudere i lati delle pipe per la comunicazione padre-nipoti */ 
			for (k = 0; k < N; k++)
			{ 
				close(piped_n[k][0]);
				close(piped_n[k][1]); 
			} 


			/* tentiamo l'apertura del file associato al processo i. Apertura sia in lettura che scrittura, cosi' da poter leggere le i caratteri
			ed eventualmente sostituirli */ 
			if ((fd = open(argv[i + 1], O_RDWR)) < 0) 
			{
				 
				printf("Errore: il processo figlio di indice %d non e' riuscito ad aprire il file %s (file non esiste oppure non e' "
					"leggibile o scrivibile\n", i, argv[i + 1]); 
				exit(-1); 
			} 

			/* ciclo di lettura del file, carattere per carattere, con trasformazione del carattere come da specifica */ 
			trans = 0L; 	/* inizializziamo a 0 il numero di trasformazioni effettuate */
			while (read(fd, &c, 1) > 0) 
			{ 
				if (isdigit(c))
				{
					/* trovato carattere numerico */ 

					/* riportiamo indietro il puntatore, cosi' che punti a questo carattere e possa essere sostituito */ 
					lseek(fd, -1L, SEEK_CUR); 

					/* andiamo ad effettuare come da specifica una stampa di un carattere spazio */ 
					write(fd, " ", 1); 

					trans++; 
				} 
			} 

			/* comunichiamo al padre attraverso la i-esima pipe padre-nipoti il numero totale di trasformazioni effettuate */ 
			write(piped_f[i][1], &trans, sizeof(trans)); 
			
			/* attesa del processo nipote generato con lettura del valore tornato */ 
			ritorno = -1; 	/* preinizializziamo il valore di ritorno a quello di errore */ 

			if ((pid = wait(&status)) < 0) 
			{ 
				printf("Errore: wait del nipote di indice %d fallita\n", i); 
			}
			else if (WIFEXITED(status))
			{
				printf("Il processo nipote con pid = %d ha ritornato il valore %d "
					"(se 255 e' incorso in qualche problema!)\n", pid, WEXITSTATUS(status));
				ritorno = (int) (trans / 256); 
			}
			else
			{
				printf("Il processo nipote con pid = %d e' terminato in modo anomalo\n", pid);
			} 
			
			exit(ritorno); 
		}
	} /* fine for */
	
	/* CODICE DEL PADRE */

	/* effettuiamo la chiusura dei lati di scrittura di TUTTE le pipe, sia quelle per la comunicazione padre-figli che padre-nipoti */ 

	for (i = 0; i < N; i++)
	{
		close(piped_f[i][1]); 
		close(piped_n[i][1]); 
	} 
	
	/* ciclo di lettura delle informazioni stampate sulle pipe in ordine secondo quello dei file */
	for (i = 0; i < N; i++) 
	{
		/* lettura del numero di trasformazioni effettuate dal processo figlio Pi */ 
		nr = read(piped_f[i][0], &trans, sizeof(trans));

		if (nr == sizeof(trans)) 	/* controllo che sia stato letto un numero corretto di byte dalla pipe */ 
		{
			printf("Il processo figlio di indice %d ha operato %ld trasformazioni di caratteri numerici in caratteri spazio " 
				"sul file %s\n", i, trans, argv[i + 1]); 
		}
		else 
		{
			printf("Errore: lettura del numero di trasformazioni effettuate dal figlio P%d non riuscito\n", i);
		}
		
		/* lettura numero di trasformazioni effettuate dal processo nipote PPi */
		nr = read(piped_n[i][0], &trans, sizeof(trans)); 

		if (nr == sizeof(trans)) 	/* controllo che sia stato letto un numero corretto di byte dalla pipe */ 
		{
			printf("Il processo nipote di indice %d ha operato %ld trasformazioni di caratteri alfabetici minusoli nei corrispondenti caratteri maiuscoli " 
				"sul file %s\n", i, trans, argv[i + 1]); 
		}
		else 
		{
			printf("Errore: lettura del numero di trasformazioni effettuate dal nipote PP%d non riuscito\n", i);
		}

		/* nota che in questo caso il controllo sul numero di byte letto e' molto importante. Infatti in tutti i casi in cui l'invio delle informazioni dipende da
		un'apertura di file dentro i figli/nipoti che non e' scontato abbia successo, c'e' bisogno di controllare che si sia riusciti a leggere correttamente da pipe 
		one evitare di andare a stampare dei valori di variabili NON inizializzate */
	} 

	/* attesa dei processi figli con recupero e stampa dei valori tornati */ 
	for (i = 0; i < N; i++) 
	{ 
		if ((pid = wait(&status)) < 0)
		{ 
			printf("Errore: wait del padre non riuscita\n");
			exit(6); 
		} 

		if (WIFEXITED(status))
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
