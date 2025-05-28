/* Progrogramma main.c che risolve la parte C della prova di esame totale tenutasi il 19 gennaio 2022 */

#include <stdlib.h> 
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/wait.h> 
#include <string.h> 
#include <ctype.h> 

/* Definizione dei bit dei permessi (in ottale) per gli eventuali file creati */
#define PERM 0644 	

/* Definizione del TIPO pipe_t che rappresenta un array di due interi, che useremo per la gestione dei lati delle varie pipe create */
typedef int pipe_t[2];

int main (int argc, char **argv)
{
	/*---------------- Variabili locali -----------------*/
	int pid; 			/* identificatore dei processi figli creati con fork() e attesi con la wait */
	int N; 				/* numero di file passati per parametro */
	int C; 				/* lunghezza blocchi di cui sono costituiti i vari file passati per parametro. 
							Si tratta di un numero intero strettamente positivo e dispari */
	int i; 				/* indice dei processi figli */
	pipe_t *piped; 		/* array dinamico di pipe per la comunicazione dei processi figli accoppiati */
	int k; 				/* indice generico per i cicli */
	int fdr; 			/* file descriptor per i file letti dai processi figli */
	int fcreato; 		/* file descriptor per il file creato da ogni coppia di processi figli */
	char *name;			/* array dinamico di caratteri atto al contenimento del nome per il nuovo file creato.
							Si parla di array dinamico perche' verra' allocata solo la memoria strettamente necessaria 
							al contenimento del vecchio nome + quella per l'estensione '.mescolato' */
	char *b; 			/* array dinamico di char di lunghezza C. Servira' per contenere i vari blocchi letti/scritti dai figli */
	int nro_tot; 		/* numero totale di blocchi di cui e' composto un certo file */
	int nro; 			/* numero di blocchi di lunghezza C che dovra' leggere dalla propria meta' file */
	int status; 		/* variabile di stato per la wait */
	int ritorno; 		/* valore ritornato dai processi figli */
	int nr, nw; 		/* variabili di controllo per lettura/scrittura da/su pipe */
	/*---------------------------------------------------*/
	
	/* Controllo lasco numero di parametri passati */
	if (argc < 3)
	{ 
		printf("Errore: numero di parametri passati a %s errato - Attesi almeno 2 [filename1 numpos_dispari] - passati: %d\n", argv[0], argc - 1); 
		exit(1); 
	}

	/* Inizializzazione del valore di N (numero di file), come numero totale di parmetri, meno il nome dell'eseguibile e C */
	N = argc - 2; 

	/* Controllo che l'ultimo parametro sia un numero intero strettamente positivo e DISPARI */
	C = atoi(argv[argc - 1]); 
	if ((C <= 0) || (C % 2 != 1))
	{ 
		printf("Errore: il parametro %s non e' un numero intero strettamente positivo e dispari\n", argv[argc - 1]); 
		exit(2);
	}

	/* Allocazione dinamica della memoria per il buffer di caratteri, come buffer di C caratteri, ossia la lunghezza del singolo blocco */
	if ((b = (char *)malloc(C * sizeof(char))) == NULL)
	{ 
		printf("Errore: allocazione dinamica della memoria per il buffer dei singoli blocchi non riuscita\n"); 
		exit(3);
	}

	/* Allocazione dinamica della memoria per l'array di pipe. Dal momento che ci sara' bisogno di una pipe per coppia di processi, e le coppie di processi sono esattamente N, allora 
		si effettua la creazione di un array di esattamente N pipe */
	if ((piped = (pipe_t *)malloc(N * sizeof(pipe_t))) == NULL)
	{ 
		printf("Errore: allocazione dinamica della memoria per l'array di pipe non riuscita\n");
		exit(4); 
	}

	/* Creazione delle N pipe per la comunicazione tra coppie di processi */
	for (k = 0; k < N; k++)
	{ 
		if (pipe(piped[k]) < 0)
		{ 
			printf("Errore: creazione della pipe di indice %d non riuscita\n", k); 
			exit(5);
		}
	}

	/* Ciclo di creazione dei processi figli */
	for (i = 0; i < 2*N; i++)
	{
		if ((pid = fork()) < 0)
		{ 
			printf("Errore: fork di creazione del processo figlio di indice %d non riuscita\n", i);
			exit(6); 
		}

		if (pid == 0)
		{ 
			/* CODICE DEI FIGLI */

			/* Si decide che nel caso in cui i processi figli incorrano in un qualche tipo di errore, questi ritornino il valore -1 (ossia 255 senza segno), ipotizzando che il numero totale 
				di blocchi letto da ciascun figlio sia verosimilmente minore di questo valore, in modo tale che si possano dunque distinguere i casi di successo da quelli di errore dei figli */

			if (i < N)
			{ 
				/* Codice del primo figlio della coppia Ci*/

				printf("DEBUG-Sono il processo figlio con pid = %d e con indice %d, dunque primo figlio della coppia di indice %d e associata al file %s\n", getpid(), i, i, argv[i + 1]); 

				/* Chiusura dei lati delle pipe non utilizzati, tenendo in conto che ogni figlio Pi scrive solo dalla pipe di indice i (sulla quale poi andra' a leggere il processo Pi+N) */
				for (k = 0; k < N; k++)
				{ 
					close(piped[k][0]);
					
					if (k != i)
					{ 
						close(piped[k][1]); 
					}
				}
				
				/* Tentativo di apertura del file associato alla coppia Ci */
				if ((fdr = open(argv[i + 1], O_RDONLY)) < 0)
				{ 
					printf("Errore: apertura in lettura del file %s da parte del primo figlio della coppia di indice %d non riuscita - file non leggibile oppure inesistente\n", argv[i + 1], i); 
					exit(-1); 
				}

				/* Calcolo del numero totale di blocchi di cui e' composto il file associato alla coppia Ci. Si noti che per la specifica e' possibile dare per scontato che il risulato della 
					divisione tra la lunghezza totale del file e quella del singolo blocco sia un numero intero. Si noti poi che per trovare la lunghezza totale in byte si andra' ad usare il 
					valore di ritorno della primitiva lseek quando si e' impostata come posizione del cursore quella della fine del file */
				nro_tot = lseek(fdr, 0L, SEEK_END) / C; 
				
				/* Calcolo del numero totale di blocchi da leggere da parte del primo figlio della coppia come meta' di nro_tot */
				nro = nro_tot / 2; 
				
				/* Per effettuare il calcolo di nro c'e' stato bisogno di spostare il file pointer alla fine del file, dunque adesso per effettuare la lettura dei blocchi sara' necessario 
					riportarlo all'inizio */
				lseek(fdr, 0L, SEEK_SET); 

				/* Ciclo di lettura dei blocchi di lunnghezza C dal file aperto, con scrittura sulla pipe i */
				for (k = 0; k < nro; k++)
				{ 
					/* Lettura di un intero blocco da file */
					read(fdr, b, C);
					
					/* Scrittura del blocco sulla pipe per comunicarlo al secondo figlio della coppia */
					nw = write(piped[i][1], b, C); 
					
					/* Controllo sulla scrittura su pipe */
					if (nw != C)
					{ 
						printf("Errore: scrittura su pipe di un blocco da parte del primo processo della coppia C%d non riuscita\n", i); 
						exit(-1); 
					}
				}

				/* Fine codice primo figlio della coppia */
			}
			else
			{ 
				/* Codice del secondo figlio della coppia Ci */
				
				printf("DEBUG-Sono il processo figlio con pid = %d e con indice %d, dunque secondo figlio della coppia di indice %d e associata al file %s\n", getpid(), i, i - N, argv[i - N + 1]);
				
				/* Creazione del nome del nuovo file mescolato come concatenazione della stringa del nome associato alla coppia Ci e l'esetnsione '.mescolato' */

				/* Allocazione dinamica dell'array di char che conterra' la concatenazione delle stringhe. La dimensione e' data dalla lunghezza della stringa del file associato al processo 
					+ esattamente 11 caratteri, 10 dei quali per l'estensione e 1 per il terminatore */
				if ((name = (char *)malloc((strlen(argv[i - N + 1]) + 11) * sizeof(char))) == NULL)
				{ 
					printf("Errore: allocazione dinamica della memoria per il buffer che conterra' il nome del file creato dalla coppia C%d non riuscita\n", i - N); 
					exit(-1);
				}

				/* Copia del nome associato al processo dentro al buffer name */
				strcpy(name, argv[i - N + 1]); 

				/* Concatenazione con la stringa di estensione */
				strcat(name, ".mescolato"); 

				/* Tentativo di creazione del file */
				if ((fcreato = creat(name, PERM)) < 0)
				{ 
					printf("Errore: creazione del file %s da parte del secondo processo della coppia C%d non riuscita\n", name, i - N);
					exit(-1); 
				}		
				
				/* Chisura dei lati delle pipe non utilizzati tenendo in conto che il processo di indice Pi+N deve solo leggere dalla pipe di indice i, sulla quale scrive il primo 
					processo della coppia */
				for (k = 0; k < N; k++)
				{ 
					close(piped[k][1]); 

					if (k != i - N) 
					{ 
						close(piped[k][0]); 
					}
				}

				/* Tentativo di apertura del file associato alla coppia Ci */
				if ((fdr = open(argv[i - N + 1], O_RDONLY)) < 0)
				{ 
					printf("Errore: apertura in lettura del file %s da parte del secondo figlio della coppia di indice %d non riuscita - file non leggibile oppure inesistente\n", argv[i - N +  1], i - N); 
					exit(-1); 
				}

				/* Calcolo del numero totale di blocchi di cui e' composto il file associato alla coppia Ci. Vale lo stesso ragionamento fatto per il primo figlio della coppia */
				nro_tot = lseek(fdr, 0L, SEEK_END) / C; 
				
				/* Calcolo del numero totale di blocchi da leggere da parte del secondo figlio della coppia come meta' di nro_tot */
				nro = nro_tot / 2; 

				/* Spostamento del file pointer NON all'inizio del file ma a meta', esattamente da dove deve iniziare a leggere i propri blocchi il secondo figlio della coppia */
				lseek(fdr, (long)nro * C, SEEK_SET);
				
				/* Ciclo di scrittura dei vari blocchi della coppia mescolati sul file appena creato */
				for (k = 0; k < nro; k++)
				{ 
					/* Lettura di un intero blocco da file */
					read(fdr, b, C);

					/* Scrittura del blocco del SECONDO processo della coppia sul file creato */
					write(fcreato, b, C); 

					/* lettura del blocco inviato dall'altro processo della coppia Ci sulla pipe i */
					nr = read(piped[i - N][0], b, C); 

					/* Controllo del numero di caratteri letti */
					if (nr != C)
					{ 
						printf("Errore: lettura da pipe di un blocco da parte del secondo processo della coppia C%d non riuscita\n", i - N);
						exit(-1); 
					}

					/* Scrittura del blocco del PRIMO processo della coppia sul file creato */
					write(fcreato, b, C); 
				}

				/* Fine codice secondo figlio della coppia */
			}

			/* Come da specifica, ogni processo figlio torna il numero di blocchi letti dal file associato alla coppia */
			exit(nro);
		}
	}

	/* CODICE DEL PADRE */

	/* Il processo padre non rientra in nessun modo nello schema di comunicazione tra i figli, e' dunque possibile andare a chiudere i lati di tutte le pipe */
	for (k = 0; k < N; k++)
	{ 
		close(piped[k][0]); 
		close(piped[k][1]);
	}

	/* Ciclo di attesa dei processi figli con recupero e stampa dei valori tornati */
	for (k = 0; k < 2*N; k++)
	{ 
		if ((pid = wait(&status)) < 0)
		{ 
			printf("Errore: wait() di uno dei processi figli da parte del padre non riuscita\n");
			exit(7);
		}

		if(WIFEXITED(status))
		{ 
			ritorno = WEXITSTATUS(status); 
			printf("Il processo figlio con pid = %d ha ritornato il valore: %d (se 255 allora e' incorso in qualche problema durante l'esecuzione)\n", pid, ritorno);
		}
		else
		{ 
			printf("Il processo figlio con pid = %d e' terminato in modo anomalo\n", pid);
		}
	}

	exit(0); 
}
