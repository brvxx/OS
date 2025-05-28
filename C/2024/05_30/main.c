/* programma C main.c che risolve la parte C della seconda prova in itinere del 30 maggio 2024 */

#include <stdlib.h> 
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/wait.h> 
#include <string.h> 
#include <ctype.h> 

/* definizione dei bit dei permessi associati al file che creeremo secondo la semantica di UNIX */
#define PERM 0644 	

/* definizione del TIPO pipe_t che rappresenta un array di due interi, che useremo per la gestione dei lati delle varie pipe create */
typedef int pipe_t[2];

int main (int argc, char **argv)
{
	/*---------------- Variabili locali -----------------*/
	int pid; 			/* identificatore dei processi figli creati con fork ed attesi con wait */
	int N; 				/* numero di processi figli che creeremo */
	int n; 				/* indice processi figli */
	int k; 				/* indice generico per i cicli */
	pipe_t *pipes; 		/* array dinamico di N pipe che permettono la comunicazione DAI figli AL padre */
	pipe_t p; 			/* singola pipe per la comunicazione tra figlio e il suo nipote */
	char pidn[11]; 		/* stringa che conterra' il pid del nipote dei singoli processi Pn. Verra' usata come arogmento del comando grep in modo tale che i processi figli 
							riescano a filtrare nell'output del ps SOLO la riga del relativo processo nipote. 
							Si e' scelta una lunghezza di 11 caratteri, dal momento che il pid e' un int e il valore massimo rappresentabile con 1 int e' di 10 cifre, poi 
							dal momento che si tratta di una stringa, si deve comunque tenere in conto del byte finale del terminatore */
	int outfile; 		/* file descriptor per l'aperture in scrittura del file creato dal padre */
	char buffer[250]; 	/* buffer statico di memoria per la lettura delle varie linee passate dai figli */
	int status; 		/* variabile di stato per la wait */
	int ritorno; 		/* valore ritornato dai processi figli */
	int nw; 			/* variabile di controllo dei byte scritti su file */
	/*---------------------------------------------------*/
	
	/* controllo stretto numero di parametri */
	if (argc != 3)
	{ 
		printf("Errore: numero di parametri passato a %s errato - Attesi 2 [numpos, nomefile] - Passati: %d\n", argv[0], argc - 1); 
		exit(1); 
	}

	/* controlliamo che N sia effettivamente un numero intero strettamente positivo */
	N = atoi(argv[1]); 
	if (N <= 0)
	{
		printf("Errore: il paramtro %s NON e' un numero intero strettamente positivo\n", argv[1]); 
		exit(2); 
	}

	/* creazone del file passato per parametro in caso di NON esistenza, oppure sovrascrittura in caso di esistenza e poi successiva apertura in scrittura.
		Come da specifica del testo andiamo a fare questa creazione prima di qualsiasi altra azione nel padre */
	if ((outfile = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, PERM)) < 0)
	{ 
		printf("Errore: creazione del file %s da parte del padre NON riuscita\n", argv[2]); 
		exit(5); 
	}

	
	/* allocazione dinamica della memoria per l'array di pipe */
	if ((pipes = (pipe_t *)malloc(N * sizeof(pipe_t))) == NULL)
	{
		printf("Errore: allocazione della memoria per l'array di pipe NON riuscita\n");
		exit(3); 
	}

	/* ciclo di creazione delle N pipe per la comunicazione figli-padre */
	for (n = 0; n < N; n++)
	{ 
		if (pipe(pipes[n]) < 0)
		{
			printf("Errore: creazione della pipe di indice %d NON riuscita\n", n); 
			exit(4); 
		}
	}

	/* ciclo di creazione degli N processi figli */
	for (n = 0; n < N; n++)
	{ 
		if ((pid = fork()) < 0)
		{
			printf("Errroe: fork() di creazione del processo figlio di indice %d NON riuscita\n", n); 
			exit(6);
		}

		if (pid == 0)
		{ 
			/* CODICE DEI FIGLI */

			/* decidiamo che in caso in cui il processo figlio Pn incorra in un qualche errore ritorni -1 (255), cosi' da essere distinto dai valori di ritorno 
				possibili del comando grep */
			
			printf("DEBUG-Sono il processo figlio di indice %d e con pid = %d e sto per tramutarmi nel comando grep\n", n, getpid()); 

			/* chiusura dei lati delle pipe non usati dal processo figlio Pn. Ogni processo di indice n infatti scrive solo sulla pipe di indice n */
			for (k = 0; k < N; k++)
			{
				close(pipes[k][0]); 

				if (k != n)
				{ 
					close(pipes[k][1]); 
				}
			}

			/* creazione della pipe di comunicazione col nipote PPn */
			if (pipe(p) < 0)
			{ 
				printf("Errore: creazione della pipe di comunicazione nipote-figlio non riuscita per il figlio di indice %d\n", n); 
				exit(-1); 
			}

			/* creazione del processo nipote PPn */

			if ((pid = fork()) < 0)
			{
				printf("Errore: fork() di creazione del processo nipote da parte del figlio di indice %d NON riuscita\n", n);
				exit(-1);
			}

			if (pid == 0)
			{ 
				/* CODICE DEI NIPOTI */

				/* analogamente a quanto detto per i figli, in caso di errore torneremo -1 (255) per omogeneita', sebbene questo valore viene di fatto perso 
					dal momento che i figli NON attendono i nipoti */

				printf("DEBUG-Sono il processo nipote di indice %d e con pid = %d e sto per eseguire il comando ps\n", n, getpid()); 

				/* chiusura del lato della pipe per la comunicazione tra Pn-padre */
				close (pipes[n][1]); 

				/* 
					implementazione del piping dei comandi, infatti il processo nipote che si tramuta nel ps NON dovra' scrivere a video ma dovra' scrivere
					sul lato di scrittura della pipe p, in modo tale che poi Pn possa usare il lato di lettura della medesima pipe come stdin ed implementare 
					il piping dei comandi tra ps e grep: ps | grep "PID_NIPOTE"
				*/
				close(1); 	/* chisura dello stdout */
				dup(p[1]); 	/* copia del riferimento al lato di scrittura di p nello stdout (appena stato chiuso)*/

				/* 
					a questo punto e' possibile andare a chiudere ENTRAMBI i lati di p:
					- p[0] perche' lato di lettura, dunque non usato da PPn
					- p[1] perche' il suo riferimento ora si trova come stdout 
				*/
				close(p[0]); 
				close(p[1]); 

				/* esecuzione del comando ps mediante la execlp. Usiamo la versione con la 'p' perche' stiamo andando ad eseguire un comando di sistema, il cui percorso 
					si trova nella variabile PATH */
				execlp("ps", "ps", (char *) 0); 

				/* se si arriva a questo punto del codice allora e' perche' la exec e' fallita, dunque riportiamo una stampa di notifica tramite la perror (non printf
					dal momento che abbiamo ridirezionato lo stdout) */
				perror("exelcp del ps da parte di uno dei nipoti NON riuscita"); 
				exit(-1);
			}

			/* ritorno al codice dei figli */

			/* implementazione del piping dei comandi anche dal lato del figlio, che esegue il SECONDO comando del piping, dunque RIDIREZIONA LO STDIN */
			close(0); 
			dup(p[0]); 

			/* il figlio pero' necessita di passare le informazioni anche al processo padre, dunque e' come se dovesse implementare un ulteriore ridirezione, sta volta 
			 	pero' dello stdout, in modo tale che punti al lato di scrittura della pipe di indice n */
			close(1); 
			dup(pipes[n][1]); 

			/* a questo punto possiamo chiudere tutti i lati delle pipe che non servono piu' */
			close(p[0]);
			close(p[1]); 
			close(pipes[n][1]); 

			/* trasformazione in stringa del pid del nipote creato, che si trova nella variabile pid */
			sprintf(pidn, "%d", pid); 

			/* esecuzione del comando grep della stringa che corrisponde al pid del nipote mediante l'execlp, per lo stesso ragionamento fatto per il nipote */
			execlp("grep", "grep", pidn, (char *) 0); 

			/* se si arriva a questa parte di codice allora il grep e' fallito, andiamo dunque a notificare l'errore su stderr con perror (stdout ridirezionato)*/
			perror("execlp del grep da parte di uno dei figli NON riuscita");
			exit(-1); 
		}
	}

	/* CODICE DEL PADRE */
	
	/* chisura dei lati di scrittura di tutte le N pipe dell'array pipes */
	for (n = 0; n < N; n++)
	{ 
		close(pipes[n][1]); 
	}

	/* a questo punto il processo padre riceve SECONDO L'ORDINE di creazione dei figli le varie righe inviate da questi nelle pipe */ 
	for (n = 0; n < N; n++)
	{ 
		k = 0; 		/* indice k usato come indice dei caratteri del buffer, dunque viene inizializzato a 0 cosi' che la scrittura dei caratteri sul buffer letti dalla pipe di 
						ogni figlio inizi dal byte 0 del buffer */
		while (read(pipes[n][0], &(buffer[k]), sizeof(char)) > 0)
		{ 
			if (buffer[k] == '\n')
			{ 
				/* 
					arrivati a questo punto il buffer contiene la linea letta dal figlio Pn CON TANTO DI TERMINATORE, dunque dobbiamo andarla a stampare come sul file creato mediante
					una write. Si noti che per la write si necessita del numero di byte da scrivere, ma quello lo abbiamo grazie a k, che corrisponde a questo valore -1 (infatti e' 
					partito da 0 e non da 1)
				*/
				k++; 	/* incremento di k in modo tale che rappresenti il nuemero totale di byte della riga (incluso \n che permettera' di andare a capo nel file) */

				/* scrittura della riga del figlio Pn su file */
				nw = write(outfile, buffer, k * sizeof(char));
				
				/* controllo numero di caratteri scritti */
				if (nw != k * sizeof(char))
				{ 
					printf("Errore: numero di caratteri scritti dal padre sul file per la riga relativa al processo di indice %d errato - scritti: %d\n", n, nw); 
					/*exit(7); 		evitiamo di terminare il padre cosi' che possa attendere i figli */
				}

				/* a questo punto possiamo anche uscire dal ciclo perche' quello che il padre voleva ottenere dalla pipe l'ha ottenuto. Si noti che se tutto va correttamente si 
					avrebbe comunque esaurito il contenuto della pipe, quindi la prossima read avrebbe tornato 0 e si sarebbe comunque usciti dal ciclo */
				break; 		
			}

			k++; 
		}
	}

	/* ciclo di attesa dei processi figli con recupero e stampa del rispettivo valore di ritorno */
	for (n = 0; n < N; n++)
	{ 
		if ((pid = wait(&status)) < 0)
		{ 
			printf("Errrore: wait di uno dei processi figli da parte del padre NON riuscuta\n"); 
			exit(8); 
		}

		if ((status & 0xFF) != 0)
		{ 
			printf("Il processo figlio con pid = %d e' terminato in modo anaomalo\n", pid); 
		}
		else
		{ 
			ritorno = (int)((status >> 8) & 0xFF); 

			if (ritorno == 0)
			{ 
				printf("Il processo figlio con pid = %d ha ritornato %d, dunque il comando grep e' stato eseguito con succeesso e ha trovato correttamente la riga "
						"corrispondente al processo nipote\n", pid, ritorno);
			}
			else 
			{ 
				printf("Il processo figlio con pid = %d che si e' tramutato nel comando grep con la execlp ha avuto qualche problema - valore tornato: %d\n", pid, ritorno); 
			}
		}
	}
	exit(0); 
}
