/* programma C che risolve la parte C della prova d'esame totale tenutasi il 12 luglio 2017 */

#include <stdlib.h> 
#include <unistd.h>
#include <stdio.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <time.h> 	/* per la generazione del seme della rand */

/* definizione del TIPO pipe_t che rappresenta un array di due interi, che useremo per la gestione dei lati delle varie pipe create */
typedef int pipe_t[2];

/* definzione del TIPO tipo_s che rappresenta una particolare struttura dati che verra' mandata dai processi figli al padre, per ogni riga 
letta dal file associato */
typedef struct { 
	int pid_n; 
	int no_linea; 
	char linea[250]; 
} tipo_s; 

/* definizione della funzione mia_random che permette di calcolare un numero randomico compreso tra 1 e n (valore passato alla funzione) */ 
int mia_random (int n) 
{ 
	int val = rand() % n; 	/* da questa operazione otterremo un valore compreso tra 0 e n - 1 */
	val++; 			/* cosi' facendo trasliamo il range a [1, n] */
	return val; 
}

int main (int argc, char **argv)
{
	/*--------------- Variabili locali ---------------*/
	int pid; 		/* identificatore dei processi figli creati con la fork e attesi con la wait */
	int N; 			/* numero di file passati o analogamente di processi figli */ 
	int i, j; 		/* indici per i cicli */
	pipe_t *piped; 		/* array dinamico di pipe per la comunicazione tra padre e figli */ 
	pipe_t p; 		/* singola pipe per la comunicazione di ogni coppia figlio-nipote */
	tipo_s msg; 		/* struttura usata da ogni figlio per spedire al padre le singole righe del file */ 
	int rows; 		/* contatore di righe lette da pipe da parte di ogni figlio */
	int X; 			/* lunghezza dei file espressa in numero di linee, nonche' parametro passato per ogni file */ 
	int r; 			/* numero randomico generato da ogni processo nipote */ 
	char opt[5]; 		/* stringa che corrispondera' al campo di opzione del comando head eseguito dai nipoti. Ipotizziamo che 
				   3 caratteri siano sufficienti per salvare il numero di righe da leggere, infatti gli altri 2 seviranno 
				   per il '-' delle opzioni e per il terminatore della stringa */
	int finito; 		/* variabile booleana per determinare quando il padre deve finire il ciclo di recupero dei dati spediti dai figli, questa varra':
				- 0 qualora ci siano ancora delle strutture da leggere per qualche figlio 
				- 1 qualora tutte le strutture di tutti i figli siano state lette */
	int status; 		/* variabile di stato per la wait */ 
	int ritorno; 		/* variabile per salvare il valore di ritorno dei figli */ 
	/*------------------------------------------------*/
	
	/* controllo lasco numero di parametri */ 
	if (argc < 3) 
	{ 
		printf("Errore: numero di parametri passato errato - Attesi almeno 2 - passati %d\n", argc - 1); 
		exit(1);
	} 
		
	/* controllo che il numero totale (2N) di parametri passati sia pari */ 
	if (((argc - 1) % 2) != 0)
	{ 
		printf("Errore: numero di parametri passato errato - Atteso un numero pari di parametri (coppie di file e rispettivo numero di righe)\n"); 
		exit(2); 
	} 

	N = (argc - 1)/2; 	/* inizializziamo il valore di N, ossia di file passati / figli da creare */ 
	
	/* controllo che i numeri passati (da specifica si suppone che siano delle stringhe rappresentanti degli interi) siano strettamente positivi */
	for (i = 0; i < N; i++) 
	{ 
		if (argv[(i * 2) + 2][0] == '-') 
		{ 
			printf("Errore: %s non rappresenta un numero positivo\n", argv[(i * 2) + 2]); 
			exit(3); 
		} 
		
		/* controlliamo che non sia il valore nullo. Nota che non andiamo a verificare solo che la prima cifra sia 0, facendo si' che casi come 
		004 vegano contati come validi. Usiamo dunque la atoi che ignora gli 0 iniziali senza salvarci da nessuna parte il ritorno */
		if (atoi(argv[(i * 2) + 2]) == 0) 
		{
			printf("Errore: %s e' il valore nullo e quindi non valido\n", argv[(i * 2) + 2]); 
			exit(3); 
		} 
	} 

	/* allochiamo la memoria per l'array piped che conterra' le N pipe per la comunicazione padre-figli */
	if ((piped = (pipe_t *)malloc(N * sizeof(pipe_t))) == NULL) 
	{ 
		printf("Errore: non e' stato prossibile allocare memoria per l'array di pipe con la malloc\n"); 
		exit(3); 
	}
	
	/* creaiamo le N pipe */ 
	for (i = 0; i < N; i++) 
	{ 
		if (pipe(piped[i]) < 0) 
		{ 
			printf("Errore: creazione della pipe di indice %d non riuscita\n", i); 
			exit(4); 
		} 
	} 

	/* ciclo di generazione dei figli */ 
	for (i = 0; i < N; i++)
	{
		/* creazione dei figli */ 
		if ((pid = fork()) < 0) 
		{ 
			printf("Errore: fork di creazione del figlio di indice %d non riuscita\n", i); 
			exit(5); 
		} 

		if (pid == 0)
		{
			/* CODICE DEI FIGLI */ 

			/* in caso di errore nei processi figli o nei nipoti decidiamo di ritornare il valore -1 (255) */ 

			printf("DEBUG-Sono il processo figlio di indice %d e pid = %d e sto per creare un processo nipote\n", i, getpid()); 

			/* chiudiamo: 
			- i lati di lettura di tutte le N pipe create dal padre 
			- i lati di scrittura di tutte le N pipe create dal padre TRANNE quello associata al processo
			che non servono per alla comunicazione tra Pi e il padre */
			for (j = 0; j < N; j++) 
			{ 
				close(piped[j][0]); 
				
				if (j != i)
				{ 
					close(piped[j][1]); 
				} 
			}
			
			/* creazione della pipe p per comunicare col nipote */ 
			if (pipe(p) < 0) 
			{
			 	printf("Errore: crezione della pipe figlio-nipote non riuscita\n"); 
				exit(-1); 
			} 

			/* creazione del processo nipote PPi */ 
			if ((pid = fork()) < 0) 
			{
			 	printf("Errore: creazione del nipote con fork non riuscita per il figlio di indice %d\n", i); 
				exit(-1); 
			} 
			 
			if (pid == 0) 
			{
				/* CODICE DEI NIPOTI */ 

				/* come riportato sopra, i nipoti ritornano -1 in caso di errore */ 
				printf("DEBUG-Sono il processo nipote associato al figlio di indice %d e ho pid = %d, e sto per selezionare delle "
					"linee dal file %s che ha %s linee totali\n", i, getpid(), argv[(i * 2) + 1], argv[(i * 2) + 2]); 

				/* come da specifica del testo per prima cosa andiamo ad inizializzare il seme per la rand */ 
				srand(time(NULL)); 

				/* chidiamo il lato di scrittura lasciato aperto dal figlio Pi per la comunicazione col padre */ 
				close(piped[i][1]); 
				
				/* implementiamo il piping dei comandi nei confronti del figlio Pi cosi' che il risultato del comando head possa finire sulla 
				pipe p. Dunque chiudiamo lo standard output e facciamo la dup del lato di scrittura di p */ 
				close(1); 
				dup(p[1]); 

				/* a questo punto si possono chiudere entrambi i lati di p, poiche' quello di lettura non serve al nipote e quello di scrittura 
				si trova in corrispodenza dello standard output */ 
				close(p[0]); 
				close(p[1]); 
				
				/* recuperiamo il numero di righe del file associato al processo (parametro) per poter calcolare il numero random da usare come 
				opzione del comando head */ 
				X = atoi(argv[(i * 2) + 2]);

				/* generiamo il valore random tramitel la funzione mia_random */ 
				r = mia_random(X); 

				/* trasformiamo il valore r nella stringa che corrisponde all'opazione del numero di righe per l'head mediante la funzione 
				sprintf che si comporta come la printf ma su un buffer, che nel nostro caso sara' opt */ 
				sprintf(opt, "-%d", r); 

				/* eseguiamo il comando head con l'opzione -r sul file associato al nipote mediante l'execlp */ 
				execlp("head", "head", opt, argv[(i * 2) + 1], (char *) 0); 
				
				/* se si arriva a questo punto del codice allora c'e' stato un errore con la exec, lo notifichiamo con la perror sullo stderr, 
				dal momento che lo stdout e' stato ridirezionato */ 
				perror("Errore: esecuzione del comando head con execlp fallita"); 
				exit(-1); 
			}
			
			/* ritorno al codice del figlio */ 

			/* chiusura del lato di scrittura della pipe p */ 
			close(p[1]);
			
			msg.pid_n = pid; 	/* inizializziamo il valore del pid del nipote nella struttura che passeremo al padre */ 
			rows = 0; 		/* inizializziamo il contatore delle linee a 0 */ 
			j = 0; 			/* inizializziamo l'indice del buffer per la linea letta a 0, per poter scrivere dall'inizio del buffer */

			/* lettura dalla pipe p carattere per carattere */ 
			while (read(p[0], &(msg.linea[j]), 1) > 0) 
			{ 
				if (msg.linea[j] == '\n') 	/* siamo arrivati alla fine di una linea, allora inviamo la struttura al padre */ 
				{ 
					/* prepariamo la struttura per essere mandata al padre, inizializzando l'indice della linea e trasformando questa in una 
					stringa, aggiungendo il terminatore */ 

					rows++; 
					msg.no_linea = rows; 

					j++; 
					msg.linea[j] = '\0'; 

					/* invio della struttura al padre */ 
					write(piped[i][1], &msg, sizeof(msg)); 

					/* azzeriamo l'indice j, cosi' che si possa tornare a scrivere la prossima riga dall'inizio del buffer */ 
					j = 0; 
				} 
				else 
				{
				 	j++; 
				} 
			} 

			/* attesa da parte del figlio del processo nipote */ 
			if ((pid = wait(&status)) < 0)
			{ 
				printf("Errore: attesa del figlio di indice %d del nipote con la wait non riuscita\n", i); 
				exit(-1); 
			} 
			
			ritorno = -1; 	/* pre-inizializziamo il ritoro a -1 cosi' da avere una sola exit alla fine del codice del figlio */
			if (WIFEXITED(status))
			{ 
				printf("DEBUG-Il processo nipote con pid = %d ha ritornato il valore %d (se 255 problemi!)\n", pid, WEXITSTATUS(status)); 
				ritorno = rows; /* il numero totale di linee lette dal figlio corrisponde implicitamente al valore randomico trovato dal nipote */ 
			} 
			else
			{
			 	printf("il processo nipote con pid = %d e' terminato in modo anomalo\n", pid); 
			}

			exit(ritorno); 
		}
	} /* fine for */

	/* CODICE DEL PADRE */ 

	/* chiudiamo il lato di scrittura di tutte le N pipe, infatti il padre si comporta da lettore per tutti i figli */ 
	for (i = 0; i < N; i++)
	{ 
		close(piped[i][1]); 
	} 

	/* ciclo di recupero dei dati spediti dai figli */ 
	finito = 0;  /* partiamo supponendo che ci siano delle strutture dal leggere */ 

	while (!finito) 
	{ 
		finito = 1; 	/* ad ogni iterazione supponiamo che non ci sia piu' nulla da leggere, ovviamente se si trova qualcosa in corrispondenza di qualche 
				figlio, il valore verra' portato a 0, cosi' che il ciclo while possa continuare */ 
		for (i = 0; i < N; i++)
		{ 
			if (read(piped[i][0], &msg, sizeof(msg)) > 0)
			{ 	
				/* stampiamo solo se siamo riusciti a leggere dalla pipe */ 
				printf("il nipote con pid = %d ha letto dal file %s la linea %d con il seguente contenuto: \n" 
					"%s", msg.pid_n, argv[(i * 2) + 1], msg.no_linea, msg.linea); 	/* non e' necessario inserire il terminatore, perche' ogni linea
													contiene gia' sicuramente il terminatore */ 
				finito = 0; 	/* trovata una pipe con ancora dei dati */ 
			} 

			/* se la read ha tornato 0, allora significa che su quella pipe non c'e' piu' nulla da leggere e che il figlio associato e' terminato */
		} 
	} 
	
	/* attesa dei figli con recupero dei valori tornati */ 
	for (i = 0; i < N; i++) 
	{ 
		if ((pid = wait(&status)) < 0) 
		{ 
			printf("Errore: attesa del padre di uno dei figli con wait non riuscita\n"); 
			exit(6); 
		} 

		if (WIFEXITED(status))
		{ 	
			ritorno = WEXITSTATUS(status); 
			printf("Il processo figlio con pid = %d ha ritornato il valore %d (se 255 ci sono stati problemi nel figlio o nel nipote, "
				"se 0 il nipote ha avuto problemi nell'eseguire l'head)\n", pid, ritorno); 
			/* infatti se ritorno, che altro non e' che il valore tornato dal figlio, ossia il numero totale di righe lette dalla pipe, e quindi 
			il numero generato in modo randomico dal nipote vale 0, quando doveva stare tra 1 e Xh che corrispondeva alla lunghezza in linee del file,
			sicuramente l'head e' incappato in qualche errore */ 
		} 
	} 

	exit(0); 
}
