/* Programma C main.c che risolve la parte C della prova di esame totale tenutasi il 9 giugno 2021 */

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
	int pid; 			/* identificatore dei processi figli creati con la fork e attesi con wait */
	int N; 				/* numero di file passati, che analogamente corrisponde al numero di processi figli che creeremo */
	int n; 				/* indice processi figli */	
	int k; 				/* indice generico per i cicli */
	int fcreato; 		/* file descriptor relativo al file crato dal padre */
	int fdr; 			/* file descriptor relativo all'aperutura del file associato ad ogni figlio */
	pipe_t p; 			/* pipe di comunicazione tra padre e processo figlio speciale */
	pipe_t *piped; 		/* array dinamico di pipe per la comunicazione tra i figli e il padre */
	char linea[200]; 	/* buffer di caratteri per memorizzare le linee lette sia dal padre che i figli */
	int len; 			/* lunghezza dei file, ottenuta tramite il processo speciale */
	int len_cur; 		/* lunghezza in byte della linea corrente letta dal padre */
	int status; 		/* variabile di stato per la wait */
	int ritorno; 		/* valore ritornato dai processi figli */
	int nr; 			/* variabile di controllo per lettura da pipe */
	/*---------------------------------------------------*/
	
	/* Controllo lasco numero di parametri passati */
	if (argc < 3)
	{ 
		printf("Errore: numero di parametri passati a %s errato - Attesi almeno 2 [filename1 filename2 ... ] - Passati: %d\n", argv[0], argc - 1);
		exit(1); 
	}

	/*	Inizializzazione della variabile N come numero totale di file, ottenuta da argc - 1, dove il valore che sottraiamo corrisponde al nome dell'eseguibile stesso */
	N = argc - 1; 

	/* Creazione del file da parte del padre */
	if ((fcreato = creat("/tmp/EdduHogea", PERM)) < 0)
	{ 
		printf("Errore: creazione del file /tmp/EdduHogea da parte del padre non riuscita\n"); 
		exit(2); 
	}

	/* Creazione della pipe p per la comunicazione col figlio speciale */
	if (pipe(p) < 0)
	{ 
		printf("Errore: Creazione della pipe di comunicazione tra padre e figlio speciale non riuscita\n");
		exit(3);
	}

	/* Creazione del figlio speciale */
	if ((pid = fork()) < 0) 
	{ 
		printf("Errore: fork di creazione del figlio speciale non riuscita\n");
		exit(4); 
	}

	if (pid == 0)
	{ 
		/* CODICE FIGLIO SPECIALE */

		/* Si decide che per i processi figli, qualunque essi siano, si vada a ritoranre al processo padre il valore -1 (ossia 255 senza segno) per poterlo distinguere dai valori di ritorno 
			normali. Nel caso di questo figlio speciale, il valore di ritorno sarebbe quello del comando wc, che ritorna valori piccoli, dunque facilmente distinguibili da quello di errore */

		printf("DEBUG-Sono il processo figlio speciale con pid = %d e sto per ricavare la lunghezza in linee dei vari file passati, contando le linee di %s "
				"col comando wc -l\n", getpid(), argv[1]); 
		
		/* Per facilitare il recupero del numero di linee di cui e' composto il file F1, si va ad utilizzare la versione filtro del comando wc, quella che NON stampa anche il nome del file, che 
			sarebbe del tutto inutile. Per farlo necessariamente bisogna andare ad implementare la ridirezione dello stdin verso il file di cui si vogliono leggere le linee, nel nostro caso F1 */
		
		close(0); 
		if (open(argv[1], O_RDONLY) < 0)
		{ 
			printf("Errore: apertura del file %s da parte del figlio speciale non riuscita\n", argv[1]); 
			exit(-1); 
		}

		/* Implementazione del piping dei comandi nei confronti del processo padre, per far si' che questo riceva su pipe p l'output del comando wc */
		close(1); 
		dup(p[1]); 

		/* Chisura di entrambi i lati di p. E' ora possibile chiuderli entrambi dal momento che quello di lettura non serve, mentre il riferimento a quello di scrittura e' ora nello stdout */
		close(p[0]); 
		close(p[1]); 

		/* Esecuzione del comando wc -l mediante la primitiva execlp. Si utilizza la versione con p, dal momento che wc e' un comando di sistema e dunque il suo percorso si trova nella variaibile 
			PATH, utilizzata dalla primtiiva con p */
		execlp("wc", "wc", "-l", (char *)0); 

		/* Se si arriva ad eseguire questa parte di codice significa che la execlp non ha funzionato, quindi si da' una notifica dell'errore con la perro su stderr (stdout ridirezionato)
			per poi terminare il figlio con il valore di errore (-1) */
		perror("Problemi con l'execlp del wc da parte del figlio speciale"); 
		exit(-1); 
	}

	/* Codice del padre */

	/* Chisura lato di scrittura di p */
	close(p[1]); 

	/* Ciclo di lettura dalla pipe p carattere per carattere con salvataggio dei caratteri dentro al buffer linea */

	/* Inizializzazione a 0 dell'indice di scrittura sul buffer */
	k = 0; 		
	while (read(p[0], &(linea[k]), 1) > 0)
	{ 
		k++; 
	}

	/* Controllo che la lettura abbia avuto successo. Si controlla dunque che k != 0, dal momento che se il wc ha avuto successo sicuramente avra' scritto qualche carattere sulla pipe */
	if (k != 0)
	{ 
		/* A questo punto la variabile k contiene esattamente il numero di caratteri letti dalla pipe, e la si puo usare per andare a trasformare il buffer in una stringa, sostituendo il \n
		con il \0, andando a scrivere il linea[k - 1], che corrisponde alla posizione del terminatore */
		linea[k - 1] = '\0'; 

		/* Trasformazione della stringa linea (che contiene l'output del wc) nel corrispettivo valore numerico */
		len = atoi(linea); 
	}
	else
	{ 
		printf("Errore: il processo padre non e' riuscito ad ottenere la lunghezza in linee del file - lettura dalla pipe di comunicazione col figlio speciale non riuscita\n");
		exit(5);
	}
	
	/* Chisura lato lettura di p */
	close(p[0]); 

	/* Stampa di debug del numero di righe trovate */
	printf("DEBUG-Lunghezza dei file passati: %d\n", len);

	/* Attesa del figlio speciale con stampa del valore ritornato ed eventuale terminazione del padre in caso di suo arresto anomalo */
	if ((pid = wait(&status)) < 0)
	{ 
		printf("Errore: attesa del processo figlio speciale non riuscita\n");
		exit(6); 
	}
	if (WIFEXITED(status))
	{ 
		printf("Il processo figlio speciale con pid = %d, che ha eseguito il comando wc -l ha ritornato il valore %d\n", pid, WEXITSTATUS(status));
	}
	else 
	{ 
		printf("Il processo figlio speciale con pid = %d e' terminato in modo anomalo\n", pid);
		exit(6); 
	}

	/* Allocazione dinamica della memoria per l'array di pipe di comunicazione normale tra padre e figli */
	if ((piped = (pipe_t *)malloc(N * sizeof(pipe_t))) == NULL)
	{ 
		printf("Errore: allocazione della memoria per l'array di pipe non riuscita\n"); 
		exit(6); 
	}

	/* Ciclo di creazione delle pipe */
	for (k = 0; k < N; k++)
	{ 
		if (pipe(piped[k]) < 0)
		{ 
			printf("Errore: creazione della pipe di indice %d non riuscita\n", k);
			exit(7);
		}
	}

	/* Ciclo di creazione dei processi figli */
	for (n = 0; n < N; n++)
	{ 
		if ((pid = fork()) < 0)
		{ 
			printf("Errore: fork di creazione del processo figlio di indice %d non riuscita\n", n);
			exit(8); 
		}

		if (pid == 0)
		{ 
			/* CODICE DEI FIGLI */

			/* Cosi' come fatto per il figlio speciale, si decide che in caso di errore i figli ritornino -1, affinche' possa essere distinto dai casi normali, dove il ritorno rappresenta il 
				numero di caratteri dell'ultima riga del file associato, che per specifica e' non maggiore di 200 caratteri (inferiore di 255) */
			
			printf("DEBUG-Sono il processo figlio di indice %d e con pid = %d e sto per leggere le righe del file %s\n", n, getpid(), argv[n + 1]);

			/* Ogni processo figlio Pn si limitaa scrivere sulla pipe di indice n, tutti gli altri lati possono dunque essere chiusi */
			for (k = 0; k < N; k++)
			{ 
				close(piped[k][0]); 

				if (k != n)
				{
					close(piped[k][1]);
				}
			}

			/* Tentativo di apertura del file associato al processo Pn */
			if ((fdr = open(argv[n + 1], O_RDONLY)) < 0)
			{ 
				printf("Errore: apertura del file %s da parte del processo figlio di indice %d non riuscita - file non leggibile o non esistente\n", argv[n + 1], n); 
				exit(-1);
			}

			/* Ciclo di lettura del file carattere per carattere con spedizione delle righe al padre */
		
			/* Inizializzazione a 0 dell'indice del buffer delle linee */
			k = 0; 

			/* Inizializzazione a 0 della variabile di ritorno, cosi' che nel caso in cui i file siano vuoti viene ritornato 0 e NON un valore non inizializzato */
			ritorno = 0; 
			while (read(fdr, &(linea[k]), sizeof(char)) > 0)
			{ 
				if (linea[k] == '\n')
				{ 
					/* Invio al padre della lunghezza della linea appena trovata come k + 1 (perche' si tiene in conto per specifica anche del terminatore) */
					k++; 
					write(piped[n][1], &k, sizeof(int)); 

					/* Invio al padre della linea */
					write(piped[n][1], linea, k * sizeof(char)); 

					/* Reset della variabile indice per la riga successiva con salvataggio del suo valore nel caso ci trovassimo sull'ultima riga */
					ritorno = k; 
					k = 0;
				}
				else
				{ 
					/* Incremento dell'indice solo se non ci si trova sul \n se no per ogni nuova riga si andrebbe a scrivere sul buffer linea a partire dal byte 1 e non 0 */
					k++; 
				}
			} 

			exit(ritorno); 

		}
	}

	/* CODICE DEL PADRE */

	/* IL processo padre si comporta come consumatore per tutti i figli, dunque si vanno a chiudere tutti i lati di scrittura delle pipe */
	for (k = 0; k < N; k++)
	{ 
		close(piped[k][1]); 
	}

	/* Ciclo di recupero delle linee rispettando l'ordine delle linee e quello dei file. Avendo ricavato la lunghezza in righe per tutti i file (che si suppone abbiano la stessa lunghezza in 
		righe per spercifica) e' possibile andare ad instaurare un for che faccia un numero di iterazioni pari al numero di righe dei file, nel nostro caso len */
	for (k = 0; k < len; k++)
	{ 
		/* Ciclo di recupero della k-esima riga relativa ad OGNI file secondo l'ordine di questi */
		for (n = 0; n < N; n++)
		{ 
			/* Lettura della lunghezza della riga k-esima inviata dal processo Pn */
			nr = read(piped[n][0], &len_cur, sizeof(int)); 

			/* Controllo sulla lettura */
			if (nr != sizeof(int))
			{ 
				printf("Errore: lettura da pipe della lunghezza in caratteri della riga %d del file %s da parte del padre non riuscita\n", k + 1, argv[n + 1]); 
				/* exit(9)		Si decide di non terminare il padre, affinche' possa attendere tutti i figli */

				/* Si passa all'iterazione successiva dal momento che la corrente e' priva di significato */
				continue; 
			}

			/* Lettura della riga */
			nr = read(piped[n][0], linea, len_cur * sizeof(char));

			/* Controllo sulla lettura */
			if (nr != len_cur * sizeof(char))
			{ 
				printf("Errore: lettura da pipe della della riga %d del file %s da parte del padre non riuscita\n", k + 1, argv[n + 1]); 
				/* exit(10)		Si decide di non terminare il padre, affinche' possa attendere tutti i figli */

				continue;
			}	

			/* Scrittura della linea ottenuta sul file creato */
			write(fcreato, linea, len_cur * sizeof(char)); 
		}
	}

	/* Ciclo di attesa dei processi figli con recupero e stampa del valore tornato */
	for (n = 0; n < N; n++)
	{ 
		if ((pid = wait(&status)) < 0)
		{ 
			printf("Errore: wait di attesa di uno dei figli non riuscita\n");
			exit(11);
		}

		if (WIFEXITED(status))
		{ 
			ritorno = WEXITSTATUS(status);
			printf("Il processo figlio con pid = %d ha ritornato il valore: %d (se 255 e' incorso in qualche errore durante la sua esecuzione)\n", pid, ritorno);
		}
		else
		{
			printf("Il processo figlio con pid = %d e' terminato in modo anomalo\n", pid); 
		}
	}

	exit(0); 
}
