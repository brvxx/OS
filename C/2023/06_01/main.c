/* programma C main.c che risolve la parte C della seconda prova in itinere tenutasi nel 1 giugno 2023 */

#include <stdlib.h> 
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/wait.h> 
#include <string.h> 
#include <ctype.h> 

/* Definizione del TIPO pipe_t che rappresenta un array di due interi, che useremo per la gestione dei lati delle varie pipe create */
typedef int pipe_t[2];

/* Definizione del TIPO Strut che rappresenta una particolare struttura con 3 campi che viene utilizzata per il passaggio di informazioni tra figli e padre */
typedef struct { 
	int pidn; 				/* Identitificatore del processo nipote associato al figlio */
	char last_line[250];	/* buffer di caratteri che conterra' l'ultima linea rovesciata letta dal figlio dalla pipe col nipote */ 		
	int len; 				/* lunghezza dell'ultiam riga, che COMPRENDE ANCHE IL TERMINATORE DI LINEA */
} Strut; 

int main (int argc, char **argv)
{
	/*---------------- Variabili locali -----------------*/
	int pid; 			/* identificatore dei processi figli creati con fork() e attesi con wait() */
	int N; 				/* numero di file passati per parametro e contestualmente anche di processi figli creati */
	int n; 				/* indice per i processi figli (specifica del testo) */
	int k; 				/* indice generico per i cicli */
	pipe_t *piped; 		/* array dinamico che conterra' le pipe per la comunicazione tra figli e padre */
	pipe_t p; 			/* singla pipe per la comunicazione tra figlio e rispettivo nipote */
	char buffer[250]; 	/* buffer di caratteri utilizzato dai figli per leggere le varie righe inviate dai nipoti. 
							Supposto di lunghezza 250 perche' nel testo viene riportato questo valore come massima lunghezza delle righe 
							contando anche terminatore \n e \0 per la stringa */
	Strut S; 			/* struttura inviata dai singoli figli e usata per ricevere questi messaggi da parate del padre */
	int status; 		/* variabile di stato per la wait */
	int ritorno; 		/* valore ritornato dai processi figli */
	int nr; 			/* variabile per controllare il valore tornato dalle read del padre */
	/*---------------------------------------------------*/
	
	/* Controllo lasco numero di parametri passati al programma */
	if (argc < 3)
	{ 
		printf("Errore: numero di parametri passati a %s errato - Attesi almeno 2 [filename1 filename2 ... ] - Passati %d\n", argv[0], argc - 1); 
		exit(1); 
	}

	/* Inizializzazione del valore di N come numero di file passati, togliendo al numero totale di parametri (argc) 1, che corrisponde al nome dell'esegubile stesso */
	N = argc - 1; 

	/* Allocazione dinamica della memoria per l'array di pipe */
	if ((piped = (pipe_t *)malloc(N * sizeof(pipe_t))) == NULL)
	{ 
		printf("Errore: allocazione della memoria per l'array di pipe NON riuscita\n"); 
		exit(2); 
	}

	/* Ciclo  di creazione delle N pipe */
	for (n = 0; n < N; n++)
	{ 
		if (pipe(piped[n]) < 0)
		{ 
			printf("Errore: creazione della pipe di indice %d NON riuscita\n", n); 
			exit(3);
		}
	}

	/* Ciclo di creazione dei processi figli */
	for (n = 0; n < N; n++)
	{ 
		/* Creazione dei singoli figli */
		if ((pid = fork()) < 0)
		{ 
			printf("Errore: fork() di creazione del processo figlio di indice %d NON riuscita\n", n); 
			exit(4); 
		}

		if (pid == 0)
		{ 
			/* CODICE DEI FIGLI */

			/* Si decide che in caso di errore i processi figli e nipoti ritornano -1 (ossia 255 unsigned). Questo per permettere la distinzione con i valori di ritorno normali, che 
				essendo lunghezze di linee di lunghezza massima 250 caratteri, saranno sempre valori inferiori a 255 */
			
			printf("DEBUG-Sono il processo figlio di indice %d e con pid = %d e sto per creare il processo nipote che vada a invertire il file %s\n", n, getpid(), argv[n + 1]); 

			/* Chisura dei lati delle pipe NON usati da Pn tenendo in conto che questo processo scrive solo sul lato di scrittura della pipe di indice n */
			for (k = 0; k < N; k++)
			{ 
				close(piped[k][0]); 

				if (k != n)
				{ 
					close(piped[k][1]); 
				}
			}

			/* Creazione della pipe p per la comunicazione col nipote */
			if (pipe(p) < 0)
			{ 
				printf("Errore: creazione della pipe p da parte del figlio di indice %d NON riuscita\n", n); 
				exit(-1); 
			}

			/* Crezione del processo nipote */
			if ((pid = fork()) < 0)
			{ 
				printf("Errore: fork() di creazione del processo nipote di indice %d non riuscita\n", n); 
				exit(-1); 
			}

			if (pid == 0)
			{
				/* CODICE DEI NIPOTI */

				/* Per quanto riguarda i valori tornati si veda il commento fatto nei figli */

				printf("DEBUG-Sono il processo nipote di indice %d con pid = %d e sto per eseguire il comando rev sul file %s\n", n, getpid(), argv[n + 1]); 

				/* Chiusura del lato della pipe di indice n usata per la comunicazione tra Pn e il padre */
				close(piped[n][1]); 

				/* Implementazione del piping dei comandi col processo Pn tramite la pipe p in modo tale che quello che scriva il comando rev finisca sulla pipe e non a video */
				close(1); 
				dup(p[1]); 

				/* Chisura di ENTRAMBI i lati di p, poiche': 
					- p[0] e' corrisponde al lato di lettura che non viene usato da i nipoti 
					- p[1] perche' oramai il riferimento al lato di scrittura di p si trova come stdout */
				close(p[0]);
				close(p[1]); 

				/* Esecuzione del comando rev sul file associato al nipote PPn mediante la primitiva execlp. Si noti che useremo la versione 'p' dal momento che rev e' un comando 
					di sistema e dunque il suo percorso si trova dentro la variabile PATH */
				execlp("rev", "rev", argv[n + 1], (char *) 0); 

				/* Se si arriva ad eseguire questa parte di codice da parte del nipote significa che la execlp ha avuto qualche problema, dunque possiamo notificare dell'accaduto 
					usando la perror() che scrive su stderr, dal momento che abbiamo ridirezionato lo stdout, quindi non possiamo usare la printf */
				perror("Problemi di esecuzione del rev da parte di un nipote"); 
				exit(-1); 
			}

			/* Ritorno al codice dei figli */

			/* Chiusura del lato di scrittura della pipe p */
			close(p[1]);

			/* Ciclo di lettura dalla pipe p carattere per carattere, con salvataggio delle righe dentro buffer */

			/* Pre-inizializzazione della lunghezza dell'ultima riga dentro S a 0, in modo tale che qualora ci siano problemi con la lettura 
				del file associato al processo e quindi NON si entri nel ciclo successivo, questo valore non rimanga NON inizializzato */
			S.len = 0; 

			/* Inizializzazione di k, usato come indice di buffer, a 0 in modo tale che la scrittura avvegna dalla posizione 0 di tale */
			k = 0; 
			while (read(p[0], &(buffer[k]), sizeof(char)) > 0)
			{ 
				if (buffer[k] == '\n')
				{ 
					/* Si e' arrivati alla fine di una delle righe, dunque salviamo la lunghezza di tale linea direttamente dentro alla struttura S, cosi' che se effettivamente 
						si tratta dell'ultima linea, il campo len della struttra sara' gia' inizializzato */
					
					k++; 		/* Incremento di 1 il valore di k per far si' che tenga in conto anche del \n nella lunghezza della linea */
					S.len = k; 

					/* reset dell'indice del buffer a 0 per la prossima linea */
					k = 0; 
				}
				else
				{ 
					/* Si necessista dell'else onde evitare che per le nuove linee k parti da 1 */
					k++; 
				}
			}

			/* Inizializzazione della struttura da inviare al padre */
			S.pidn = pid; 	/* copia del pid del nipote */

			/* copia della linea contenuta nel buffer nel buffer proprio della struttura. Si noti che nel caso che si abbiano avuto dei problemi nell'apertura del file associato 
				al processo S.len == 0, dunque la memcpy non fara' nulla, giustamente! */
			memcpy(S.last_line, buffer, S.len * sizeof(char)); 

			/* Invio della struttura al padre sulla pipe di indice n */
			write(piped[n][1], &S, sizeof(S)); 

			/* Per correttezza ogni processo figlio attende il nipote creato */
			if ((pid = wait(&status)) < 0)
			{ 
				printf("Errore: wait() del nipote di indice %d NON riuscita\n", n);
				exit(-1); 
			}

			/* pre-inizializzazione del valore di ritorno a 1, se poi il nipote e' terminato volontariamente si tornera' il valore normale richiesto dal testo */
			ritorno = -1; 
			if ((status & 0xFF) != 0)
			{
				printf("Il processo nipote di indice %d e con pid = %d e' terminato in modo anomalo\n", n, pid); 

			}
			else 
			{
				printf("Il processo nipote di indice %d e con pid = %d ha eseguito il rev ed ha ritorato il valore %d\n", n, pid, WEXITSTATUS(status)); 

				/* Come da specifica del testo ogni Pn se tutto e' andato bene ritorna al padre la lunghezza della linea a lui inviata nella struttura senza 
					contare pero' il terminatore di linea. Se il file associato al processo NON esisteva S.len varra' 0 che corrisponde ad una linea vuota, in 
					questo caso non ha senso andare a decrementare di 1 infatti la lunghezza senza terminatore sara' comunque 0 */
				
				if (S.len != 0)
				{
					ritorno = S.len - 1; 
				}	
				else
				{ 
					ritorno = S.len;
				}
			}
			
			exit(ritorno); 
		}
	}

	/* CODICE DEL PADRE */

	/* Il processo padre legge da tutte le pipe ma non scrive su nessuna, dunque si procede con la chiusura dei lati di scrittur di tutte le pipe di piped */
	for (n = 0; n < N; n++)
	{ 
		close(piped[n][1]); 
	}

	/* Ricezione delle strutture inviate dai figli secondo l'ordine dei file, dunque quello dei processi dal momento che i processi sono associati ai file nell'ordine in cui 
		vegnono passati questi ultimi come parametri */
	for (n = 0; n < N; n++)
	{
		nr = read(piped[n][0], &S, sizeof(S)); 

		if (nr == sizeof(S))
		{ 
			/* Trasformazione dell'ultima linea inviata dal processo Pn in una stringa che possa essere stampata con la printf. Come richiesto dal testo verra' mantenuto il fatto 
				che questo buffer sia una linea, dunque il \0 verra' inserito DOPO il \n e non al suo posto */
			S.last_line[S.len] = '\0'; 

			/* Stampa richiesta dal testo */
			printf("Il processo nipote con pid = %d ha letto e invertito l'ultima linea del file %s di %d caratteri totali, ossia: '%s'\n", S.pidn, argv[n + 1], S.len, S.last_line); 
		}
		else 
		{ 
			printf("Errore: il processo padre non e' riuscito a leggere correttamente la struttura inviata dal figlio di indice %d - byte letti: %d\n", n, nr); 
			/* exit(5); 	Si decide di non terminare il padre affinche' possa attendere correttamente i vari figli */
		}
	}

	/* Ciclo di attesa dei processi figli, con recupero e stampa dei rispettivi valori tornati */
	for (n = 0; n < N; n++)
	{ 
		if ((pid = wait(&status)) < 0)
		{ 
			printf("Errore: wait() di un processo figlio NON riuscita\n"); 
			exit(6); 
		}

		if ((status & 0xFF) != 0)
		{ 
			printf("Il processo figlio con pid = %d e' terminato in modo anomalo\n", pid); 
		}
		else 
		{ 
			ritorno = (int)((status >> 8) & 0xFF); 
			printf("Il processo figlio con pid = %d ha ritornato il valore %d (se 255 e' incorso in qualche problema durante l'esecuzione)\n", pid, ritorno); 
		}
	}

	exit(0); 
}
