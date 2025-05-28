/* Programma C main.c che risolve la parte C della prova di esame totale tenutasi il 12 giugno 2024 */

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
	int N; 				/* numero di file passati per parametro */
	int n; 				/* indice dei processi figli */
	int k; 				/* indice generico per i cicli */
	pipe_t *piped; 		/* array dinamico di pipe per la comunicazione dei processi in coppia */
	char linea[250]; 	/* buffer di memoria per contenere la linea corrente letta (pari o dispari) dai processi figli. Viene scelta come lunghezza 
							250 caratteri perche' viene specificato nel testo che la lunghezza delle righe (compreso il \n) non superi questo numero di caratteri */
	char buffer[250]; 	/* buffer di memoria usato dal secondo processo di ogni coppia per leggere la riga spedita dal primo processo della coppia */
	int fcreato; 		/* file descriptor associato al file creato da ogni coppia */
	char *name; 		/* array dinamico di char che conterra' il nome del file creato, ottenuto dalla concatenazione del nome del file associato ad una coppia di processi 
							e l'estensione '.max' */
	int fdr; 			/* file descriptor associato all'apertura in lettura del file associato ai vari figli */
	int len_buffer; 	/* lunghezza della linea del primo processo di ogni coppia, letta dal rispettivo secondo processo della stessa coppia */
	int nro; 			/* lunghezza massima calcolata da ogni processo per le righe di propria competenza */
	int row; 			/* contatore linee del file letto dai processi figli */
	int status; 		/* variabile di stato per la wait */
	int ritorno; 		/* valore ritornato dai processi figli */
	int nr; 			/* variabile di controllo per la lettura da pipe */
	/*---------------------------------------------------*/
	
	/* Controllo lasco numero di parametri */
	if (argc < 2)
	{ 
		printf("Errore: numero di parametri passati a %s errato - Atteso almeno 1 [filename1 ... ] - Passati: %d\n", argv[0], argc - 1);
		exit(1);
	}

	/* Calcolo del numero di file passati */
	N = argc - 1; 

	/* Allocazione dinamica della memoria per l'array di pipe. Si noti che dal momento che la comunicazione tra processi avviene solamente tra processi accoppiati, e le coppie 
		sono esattamente N, allora sara' necessario creare un array di sole N pipe */
	if ((piped = (pipe_t *)malloc(N * sizeof(pipe_t))) == NULL)
	{ 
		printf("Errore: allocazione della memoria per l'array di pipe non riuscita\n"); 
		exit(2); 
	}

	/* Ciclo di creazione delle N pipe */
	for (k = 0; k < N; k++)
	{ 
		if (pipe(piped[k]) < 0)
		{ 
			printf("Errore: creazione della pipe di indice %d non riuscita\n", k);
			exit(3);
		}
	}

	/* Ciclo di creazione dei processi figli. Si noti che questi sono 2N e non solo N */
	for (n = 0; n < 2*N; n++)
	{
		if ((pid = fork()) < 0)
		{ 
			printf("Errore: fork di creazione del processo figlio di indice %d non riuscita\n", n); 
			exit(4); 
		}

		if (pid == 0)
		{ 
			/* CODICE DEI FIGLI */

			/* Si decide che nel caso in cui un processo figlio incorra in qualche tipo di errore ritorni al padre il valore -1 (255 senza segno), in modo tale che possa essere distinto dal 
				normale valore di ritorno, che corrisponde alla lunghezza massima trovata nella lettura delle linee, che per specifica e' sicuramente minore di 250 */
			
			if (n < N)
			{ 
				/* Codice del primo processo della coppia Cn */

				printf("DEBUG-Sono il processo figlio di indice %d e con pid = %d e sono il primo processo della coppia C%d, associata al file %s\n", n, getpid(), n, argv[n + 1]); 

				/* Il processo Pn si limita a scrivere le proprie linee dispari sulla pipe n, tutti gli altri lati delle pipe non usate verranno quindi chiusi */
				for (k = 0; k < N; k++)
				{ 
					close(piped[k][0]); 

					if (k != n)
					{ 
						close(piped[k][1]);
					}
				}

				/* Tentativo di apertura del file associato alla coppia */
				if ((fdr = open(argv[n + 1], O_RDONLY)) < 0)
				{ 
					printf("Errore: il primo processo della coppia C%d non e' riuscito ad aprire il file %s - file non leggibile o inesistente\n", n, argv[n + 1]); 
					exit(-1);
				}

				/* Ciclo di lettura del file associato al processo, carattere per carattere con invio delle linee DISPARI al secondo processo della coppia */
				k = 0; 		/* Inizializzazione a 0 dell'indice del buffer della riga letta */
				row = 1; 	/* Inizializzazione a 1 del contatore di riga. Si noti infatti che la specifica richieda che venga inizializzato a 1 */
				nro = -1; 	/* Inizializziamo la lunghezza massima ad un valore negativo, cosi' che venga aggiornato alla prima riga trovata per poi essere confrontato via via 
								tra le varie righe */
				while (read(fdr, &(linea[k]), sizeof(char)) > 0)
				{ 
					if (linea[k] == '\n')
					{ 
						/* Fine della riga corrente */ 

						if (row % 2 == 1)
						{ 
							/* Trovata una riga di indice dispari, si procede dunque con l'invio al secondo processo della coppia */
							
							/* Invio della lunghezza in caratteri della riga, ottenuta incrementando k di 1, cosi' che contenga anche il terminatore di riga */
							k++; 
							write(piped[n][1], &k, sizeof(int)); 

							/* Invio della riga corrente */
							write(piped[n][1], linea, k * sizeof(char)); 

							/* Eventuale aggiornamento della lunghezza massima */
							if (k > nro)
							{ 
								nro = k; 
							}
						}
						
						/* A prescindere dal fatto che la linea sia pari o dispari, giunti alla fine di una linea si fa il Reset di k a 0 per la riga successiva e
							si incrementa il contatore della riga corrente */
						k = 0; 
						row++; 
					}
					else
					{ 
						/* Se non si e' alla fine di una riga allora ci si limita ad incrementare l'indice di linea */
						k++;
					}
				}
			}
			else 
			{ 
				/* Codice del secondo processo della coppia Cn */

				printf("DEBUG-Sono il processo figlio di indice %d e con pid = %d e sono il secondo processo della coppia C%d, associata al file %s\n", n, getpid(), n - N, argv[n - N + 1]); 

				/* Come da specifica per prima cosa il secondo processo della coppia Cn tenta la creazione del file: */

				/* Allocazione dinamica della memoria per l'array di caratteri che conterra' la concatenazione del nome del file associato alla coppia + '.max' 
					La lunghezza totale di questo array corrispondera' alla lunghezza del nome del file associato alla coppia + 5 (l'estensione .max e il terminatore)*/
				if ((name = (char *)malloc((strlen(argv[n - N + 1]) + 5) * sizeof(char))) == NULL)
				{ 
					printf("Errore: allocazione dinamica dell'array di char per contenere il nome del file creato dalla coppia C%d non riuscita\n", n - N); 
					exit(-1); 
				}

				/* Copia del nome di argv[n - N + 1] dentro il buffer name */
				strcpy(name, argv[n - N + 1]); 

				/* Concatenazione con l'estensione */
				strcat(name, ".max"); 

				/* Creazione effettiva del file */
				if ((fcreato = creat(name, PERM)) < 0)
				{ 
					printf("Errore: creazione del file %s da parte della coppia C%d non riuscita\n", name, n - N); 
					exit(-1); 
				}

				/* Il processo Pn+N si limita a leggere le linee dispari inviate dal primo figlio della coppia dalla pipe n, tutti gli altri lati delle pipe non usate verranno quindi chiusi */
				for (k = 0; k < N; k++)
				{ 
					close(piped[k][1]); 

					if (k != n - N)
					{ 
						close(piped[k][0]);
					}
				}

				/* Tentativo di apertura del file associato alla coppia */
				if ((fdr = open(argv[n - N + 1], O_RDONLY)) < 0)
				{ 
					printf("Errore: il secondo processo della coppia C%d non e' riuscito ad aprire il file %s - file non leggibile o inesistente\n", n, argv[n - N + 1]); 
					exit(-1);
				}

				/* Ciclo di lettura del file associato al processo */
				k = 0; 		/* Inizializzazione a 0 dell'indice del buffer della riga letta */
				row = 1; 	/* Inizializzazione a 1 del contatore di riga. Si noti infatti che la specifica richieda che venga inizializzato a 1 */
				nro = -1; 	/* Inizializziamo la lunghezza massima ad un valore negativo, cosi' che venga aggiornato alla prima riga trovata per poi essere confrontato via via 
								tra le varie righe */
				while (read(fdr, &(linea[k]), sizeof(char)) > 0)
				{ 
					if (linea[k] == '\n')
					{ 
						/* Fine della riga corrente */ 

						if (row % 2 == 0)
						{ 
							/* Trovata una riga di indice PARI, si procede dunque con la lettura della riga passata dal primo processo della coppia */
							
							/* Incremento di k affinche' tenga in conto anche della lunghezza del terminatore */
							k++; 

							/* Lettura della lunghezza della riga inviata dal processo accoppiato, con relativo controllo */
							nr = read(piped[n - N][0], &len_buffer, sizeof(int));  

							if (nr != sizeof(int))
							{ 
								printf("Errore: lettura lunghezza da pipe da parte del secondo processo di C%d non riuscita\n", n - N); 
								exit(-1); 
							}

							/* Lettura della linea con controllo */
							nr = read(piped[n - N][0], buffer, len_buffer * sizeof(char)); 

							if (nr != len_buffer * sizeof(char))
							{ 
								printf("Errore: lettura riga da pipe da parte del secondo processo di C%d non riuscita\n", n - N); 
								exit(-1); 	
							}

							/* Confronto tra le lunghezza della riga corrente PARI del secondo figlio della coppia, e quella corrente DISPARI trovata dal primo. Solo 
								quella con lunghezza massima verra' stampata sul file creato dalla coppia */
							if (k > len_buffer)
							{ 
								write(fcreato, linea, k * sizeof(char)); 
							}
							else 
							{ 
								write(fcreato, buffer, len_buffer * sizeof(char)); 
							}

							/* Eventuale aggiornamento di nro */
							if (k > nro)
							{ 
								nro = k; 
							}
						}

						/* A prescindere dal fatto che la linea sia pari o dispari, giunti alla fine di una linea si fa il Reset di k a 0 per la riga successiva e
							si incrementa il contatore della riga corrente */
						k = 0; 
						row++; 
					}
					else
					{ 
						/* Se non si e' alla fine di una riga allora ci si limita ad incrementare l'indice di linea */
						k++;
					}
				}
			}

			exit(nro); 
		}
	}

	/* CODICE DEL PADRE */

	/* Il padre in questa soluzione e' escluso dallo schema di comunicazione, quindi si limita a creare le pipe ma poi ne chiude TUTTI i lati */
	for (k = 0; k < N; k++)
	{ 
		close(piped[k][0]); 
		close(piped[k][1]); 
	}

	/* Ciclo di attesa dei processi figli con recupero e stampa del valore tornato */
	for (k = 0; k < 2*N; k++)
	{ 
		if ((pid = wait(&status)) < 0)
		{ 
			printf("Errore: wait di uno dei figli da parte del padre non riuscuita\n");
			exit(5); 
		}

		if (WIFEXITED(status))
		{ 
			ritorno = WEXITSTATUS(status); 
			printf("Il processo con pid = %d ha ritornato il valore %d (se 255 e' incorso in qualche problema durante l'esecuzione)\n", pid, ritorno); 
		}
		else 
		{ 
			printf("Il processo con pid = %d e' terminato in modo anomalo\n", pid); 
		}
	}
	exit(0); 
}
