/* programma C main.c che risolve la parte C della seconda prova in itinere tenutasi il 8 giugno 2022 */

#include <stdlib.h> 
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/wait.h> 
#include <string.h> 
#include <ctype.h>
#include <signal.h> 

/* definizione della costante MSGSIZE che rappresenta la lunghezza delle righe di TUTTI i file passati tenendo in conto anche del terminatore di linea */
#define MSGSIZE 6	

/* definizione del TIPO pipe_t che rappresenta un array di due interi, che useremo per la gestione dei lati delle varie pipe create */
typedef int pipe_t[2];

int main (int argc, char **argv)
{
	/*---------------- Variabili locali -----------------*/
	int pid; 				/* identificatore dei processi figli creati con fork() e attesi con wait() */
	int N; 					/* numero di file passati e analogamente di processi figli che verranno creati */
	int Npipe; 				/* numero di pipe da creare */
	int n; 					/* indice dei processo figli */
	int k; 					/* indice generico per i cicli */
	pipe_t *piped; 			/* array dinamico per gestire le pipe che verranno create per la comunicazione tra fratelli */
	char linea[MSGSIZE];	/* buffer di memoria per memorizzare le linee lette da file */
	char buffer[MSGSIZE]; 	/* buffer di memoria per memorizzare le stringhe inviate dal processo P0 */
	int fdr;				/* file descriptor associato all'apertura dei file nei vari figli */
	int status; 			/* variabile di stato per la wait */
	int ritorno; 			/* valore ritornato dai processi figli */
	/*---------------------------------------------------*/

	if (argc < 4)
	{ 
		printf("Errore: numero di parametri passati a %s errato - Attesi almeno 3 [filename1 filename2 filename3] - Passati: %d\n", argv[0], argc - 1); 
		exit(1); 
	}
	
	/* Inizializzazione di N come argc - 1, dove quel valore sottratto rappresenta il nome dell'esegubile */
	N = argc - 1; 

	/* Inizializzazione di Npipe, numero di pipe da creare come N - 1. Questo perche' si ha un particolare schema di comunicazione in cui e' il processo P0 che comunica ai restanti 
		N - 1 processi suoi fratelli. Quindi per questa comunicazione sono necessarie solo N - 1. Si noti poi che sebbene i processi figli dovranno creare dei nipoti, con questi non 
		dovranno comunicare direttamente, ma semplicemente attendere il loro valore di ritorno */
	Npipe = N - 1; 

	/* Allocazione dinamica della memoria per l'array di N - 1 pipe */
	if ((piped = (pipe_t *)malloc(Npipe * sizeof(pipe_t))) == NULL)
	{
		printf("Errore: allocazione della memoria con malloc per l'array di pipe fallita\n"); 
		exit(2); 
	}
	
	/* Creazione delle pipe */
	for (k = 0; k < Npipe; k++)
	{ 
		if (pipe(piped[k]) < 0)
		{
			printf("Errore: creaazione della pipe di indice %d fallita\n", k);
			exit(3);
		}
	}

	/* Ciclo di generazione dei figli */
	for (n = 0; n < N; n++)
	{ 
		if ((pid = fork()) < 0)
		{ 
			printf("Errore: fork di creazione del processo figlio di indice %d fallita\n", n);
			exit(4); 
		}

		if (pid == 0)
		{ 
			/* CODICE DEI FIGLI */

			/* Si decide che in caso i processi figli incorrano in qualche tipo di errore nell'esecuzione, questi ritornino il valore -1 al padre (ossia 255 senza segno) in modo tale che 
				si possa distinguere dal valore normale di ritorno, ossia l'indice d'ordine del figlio, che supponiamo non arrivare mai a valori cosi' elevati */
			
			printf("DEBUG-Sono il processo figlio di indice %d e con pid = %d e sono associato al file %s\n", n, getpid(), argv[n + 1]);
			
			/* Chiusura dei lati delle pipe NON utilizzati dai processi figli. Qui il codice si divide per i figli, infatti: 
				- il processo P0 scrive su tutte le pipe, quindi andra' a chiudere solo i lati di lettura di tutte queste 
				- i processi Pn (con n != 0) si limitano a leggere dalla pipe di indice n - 1. Questo perche' l'indicizzazione di questi processi parte da 1 (perche' abbiamo escluso P0), 
				  ma l'indicizzazione delle pipe parte necessariamente da 0, dunqe per associare una pipe ad ogni figlio si sottrae 1 dal proprio indice */
			if (n == 0)
			{ 
				/* chiusure di P0 */
				for (k = 0; k < Npipe; k++)
				{ 
					close(piped[k][0]); 
				}
			}
			else 
			{
				/* chisura del generico Pn */
				for (k = 0; k < Npipe; k++)
				{ 
					close(piped[k][1]);

					if (k != (n - 1))
					{
						close(piped[k][0]); 
					}
				}
			}

			/* A prescindere dall'indice del figlio, questi dovranno tutti aprire il lettura il proprio file */
			if ((fdr = open(argv[n + 1], O_RDONLY)) < 0)
			{
				printf("Errore: apertura in lettura del file %s da parte del figlio di indice %d fallita - file non leggibile oppure inesistente\n", argv[n + 1], n); 
				exit(-1); 
			}

			/* Da qui il poi il codice dei figli si divide in base all'indice. Infatti il processo P0 si comportera' da produttore di informazioni, mentre i restanti figli da consumatori */
			if (n == 0)
			{ 
				/* CODICE DI P0 */
				
				/* sleep(2); per verificare che se un figlio terminasse perche' associato ad un file inesistente, allora P0 riceverebbe il SIGPIPE */

				/* Dal momento che il processo P0 invia dei messaggi potenzialmente a dei figli che potrebbero essere terminati a causa del file associato inesistente, e' preferibile cautelarsi
					e andare ad ignorare in P0 il SIGPIPE, che ne causerebbe l'arresto anomalo */
				signal(SIGPIPE, SIG_IGN);
				

				/* Ciclo di lettura delle linee da file con inoltro di tali ai processi fratelli */
				while (read(fdr, &linea, MSGSIZE) > 0)
				{ 
					/* Si noti che la specifica assicura che le linee siano di lunghezza MSGSIZE, dunque invece di leggere carattere per carattere si puo' direttamente usufruire di questo valore */

					/* Trasformazione della linea in STRINGA, andando a sostituire il terminatore di linea con quello di stringa */
					linea[MSGSIZE - 1] = '\0'; 

					/* Ciclo di invio della stringa corrente ai processi fratelli */
					for (k = 0; k < Npipe; k++)
					{ 
						write(piped[k][1], linea, MSGSIZE); 
					}
				}
					
			}
			else 
			{ 
				/* CODICE DI Pn (con n != 0) */

				/* Ciclo di lettura dei nomi inviati dal processo P0 sulla pipe */
				while (read(piped[n - 1][0], buffer, MSGSIZE) > 0)
				{ 
					/* A questo punto la variabile buffer contiene la STRINGA di un nome di file inviato da P0 che deve essere confrontato con TUTTI i nomi di file conentuti nel file 
						associato a Pn, dunque si deve instaurare un ulteriore ciclo che vada a leggere questi nomi dal file */
					while (read(fdr, linea, MSGSIZE) > 0)
					{ 
						/* Trasformazione della riga in STRINGA */
						linea[MSGSIZE - 1] = '\0'; 

						/* Creazione del processo nipote che eseguira' il diff per il confronto tra questi due file */
						if ((pid = fork()) < 0) 
						{ 
							printf("Errore: fork di creazione di un processo nipote da parte del figlio di indice %d fallita\n", n); 
							exit(-1); 
						}
						
						if (pid == 0)
						{ 
							/* CODICE DEI NIPOTI */
							/* Analogamente a quanto detto per i processi figli, se i nipoti dovessero incorrere in qualche errore, si decide di fargli tornare il valore -1 (255), in modo tale 
								che possa essere distinto dai casi di corretto funzionamento, ossia i valori tornati dal diff, molto minori di 255 */

							/* Chiusura del lato di lettura della pipe di comunicazione con P0, che non verra' usato */
							close(piped[n - 1][0]); 

							/* Dal momento che sara' il processo figlio a gestire il risultato della diff tramite il valore di ritorno del nipote e' possibile andre a scartare le eventuali 
								stampe di output o di errore del comando diff. Per farlo basta andare a ridirezionare lo stdout e stderr verso /dev/null */
							close(1); 
							open("/dev/null", O_WRONLY); 

							close(2); 
							open("/dev/null", O_WRONLY); 

							/* Esecuzione del comando diff su buffer (nome del file mandato da P0) e linea (nome del file letto nel file associato da Pn). Per l'esecuzione verra' usata la primtiva 
								execlp dal momento che diff e' un comando di UNIX, il cui percorso si trovera' dentro la variabile PATH, usata dalla versione della exec con 'p' */
							execlp("diff", "diff", linea, buffer, (char *)0); 

							/* Se si arriva ad eseguire questa parte di codice significa che c'e' stato un qualche errore con la exec. Dal momento pero' che si ha ridirezionato sia lo stdout che 
								stderr non sara' possibile effettuare delle stampe di notifica */
							exit(-1); 
						}

						/* Attesa del nipote con recupero e stampa del valore tornato */
						if ((pid = wait(&status)) < 0)
						{ 
							printf("Errore: wait di un nipote da parte del figlio di indice %d fallita\n", n); 
							exit(-1); 
						}
						if ((status & 0xFF) != 0)
						{ 
							printf("Il processo nipote con pid = %d e' terminato in modo anomalo\n", pid); 
							exit(-1); 
						}
						else 
						{ 
							ritorno = (int)((status >> 8) & 0xFF); 

							switch (ritorno)
							{
								case 0:
								{
									printf("I file %s e %s sono uguali\n", buffer, linea); 
									break; 
								}

								case 1: 
								{ 
									printf("I file %s e %s sono diversi\n", buffer, linea);
									break;
								}
								
								default:
								{
									printf("Uno dei due file tra %s e %s non esiste, oppure non esistono entrambi\n", buffer, linea);
								}
							}
						}
					}

					/* Arrivati a questo punto il processo Pn ha confrontato il PRIMO nome inviato da P0 con TUTTI i nomi presenti nel suo file associato. Adesso per far si' che anche 
						i prossimi nomi inviati da P0 possano essere nuovamente confrontati con tutti i nomi del file dobbiamo riportare il file pointer all'inizio per permettre una nuova 
						lettura di questo file */
					lseek(fdr, 0L, SEEK_SET);
				}
			}

			exit(n); 
		}
	}

	/* CODICE DEL PADRE */

	/* Il processo padre non e' compreso nella comunicazione, dunque non usa nessuna pipe. Si possono dunque chiudere tutte le pipe */
	for (k = 0; k < Npipe; k++)
	{ 
		close(piped[k][0]);
		close(piped[k][1]); 
	}

	/* Ciclo di attesa dei figli con recupero e stampa dei valori tornati */
	for (k = 0; k < N; k++)
	{ 
		if ((pid = wait(&status)) < 0)
		{ 
			printf("Errore: wait di uno dei figli da parte del padre fallita\n"); 
			exit(5); 
		}
		if ((status & 0xFF) != 0) 
		{ 
			printf("Il figlio con pid = %d e' terminato in modo anomalo\n", pid); 
		}
		else 
		{ 
			ritorno = (int)((status >> 8) & 0xFF); 
			printf("Il processo figlio con pid = %d ha ritoranto il valore %d\n", pid, ritorno);
		}
	}
	exit(0); 
}
