/* programma C main.c che risolve la parte di C della prova di esame totale tenutasi nel 15 maggio 2015 */ 

/* breve descrizione del programma: 
il programma accetta un numero variabile N (con N >= 2) di parametri, che rappresentano nomi di file. Il processo padre dovra' andare a creare esattamente N processi figli, ognuno dei quali associato al corrispondente file mediante l'indice d'ordine. Ogni processo figlio a sua volta dara' vita ad un singolo processo nipote, il qualeavra' associato lo stesso file del padre. Il processo nipote conta il numero di rige del file e riporta tale informazione al processo figlio tramite una pipe, a sua volta il processo figlio dovra' ricevere questa informazione, convertirla in un intero e comunicarla a sua volta al padre. Il padre ha invece il compito di sommare tutti questi valori inviati dai figli per poi sommarli e stampare il risultato a video. Al termine dell'esecuzione il padre attende i figli andando a stampare il loro pid e il valore ritornato, che corrisponde al valore tornato dal nipote ai figli stessi. */

#include <stdlib.h> 
#include <unistd.h> 
#include <stdio.h> 
#include <fcntl.h>
#include <sys/wait.h>

/* Definizione del TIPO pipe_t come array di due interi, che servira' alla gestione delle varie pipe di comunicazione tra i processi */
typedef int pipe_t[2]; 

int main (int argc, char **argv)
{ 
	/*--------------- Variabili locali ------------------*/ 
	int N; 			/* numero di file e quindi anche di processi figli */ 
	int i, j; 		/* indici per i cicli */ 
	int pid; 		/* identificatore dei processi usato dalla fork e la wait */ 
	pipe_t *piped; 		/* array dinamico di pipe che permette la comunicazione tra padre e gli N figli. Questo array conterra' 
				esattametne N pipe, e verranno associate ai processi figli in base al rispettivo indice d'ordine */
	pipe_t p; 		/* singola pipe per la comunicazione tra processo nipote e rispettivo figlio */
	char buf[11]; 		/* buffer che permettera' di leggere i caratteri della stringa che rappresentano il numero di linee stampato dal figlio.
				Lo facciamo lungo  11 perche' nel testo viene specificato che la lunghezza dei file sia sempre rappresentabile tramite 
				un intero di C, quindi un numero a 32 bit, il cui valore massimo si aggira intorno al miliardo. Quindi per poter rappresentare 
				questo valore saranno necessarie AL MASSIMO 10 cifre (caratteri), quindi andiamo a fare il nostro buffer lungo 10 (per il numero
				stesso) + 1 (per il terminatore della stringa) */
	int fdr; 		/* file descriptor per l'apertura dei file da parte dei nipoti */ 
	int len; 		/* variabile utilizzata per salvare la lunghezza in linee dei singoli file */ 
	long int len_sum = 0L; 	/* variabile utilizzata per sommare tutte le lunghezze dei vari file */ 
	int status; 		/* variabile di stato per la wait */
	int ritorno; 		/* valore ritornato dai processi */ 
	/*---------------------------------------------------*/


	/* controllo lasco numero di parametri */ 
	if (argc < 3) 
	{ 
		printf("Errore: numero di parametri passato errato - Attesi almeno 2 (due nomi di file) - Passati %d\n", argc - 1); 
		exit(1); 
	} 
		
	N = argc - 1; 	/* numero di file si ottiene togliendo al numero totale di parametri 1, corrispondente al nome dell'eseguibile */

	/* allocazione array dinamico per contenere le N pipe di comunicazione tra padre e figli */ 
	if ((piped = (pipe_t *)malloc(N * sizeof(pipe_t))) == NULL)
	{ 
		printf("Errore: non e' stato possibile allocare la memoria per l'array di pipe\n"); 
		exit(2); 
	}

	/* creazione delle N pipe */ 
	for (i = 0; i < N; i++) 
	{
		if (pipe(piped[i]) < 0)
		{
			printf("Errore: non e' stato possibile creare la pipe %d\n", i); 
			exit(3); 
		} 
	} 

	/* ciclo di generazione dei process figli */ 
	for (i = 0; i < N; i++) 
	{ 
		/* creazione dell'i-esimo processo figlio */ 
		if ((pid = fork()) < 0)
		{ 
			printf("Errore: non e' stato possibile creare il processo %d con la fork\n", i);
			exit(4); 
		} 

		if (pid == 0) 
		{
			/* CODICE DEI FIGLI */ 
	
			/* per i valori di errore che ritorna il figlio al processo padre possiamo andare ad utilizzare dei valori 
			negativi decrescenti (-1, -2, ...) in quanto corrispondono ad interi positivi grandi e non verranno confusi 
			coi valori normali di ritorno, che corrisponderanno all'esito del comando wc del processi nipote, il quale 
			si trovera' al massimo a tornare dei valori positivi PICCOLI in caso di errore */

			printf("DEBUG-Sono il processo figlio di indice %d con pid = %d e sto per creare un processo nipote "
				"che vada a contare le righe del file %s\n", i, getpid(), argv[i + 1]); 
			
			/* chiusura dei lati delle pipe non usati dal singolo processo figlio di indice i: 
			- chriusura del lato di lettura di TUTTE le pipe poiche' nei confronti dei padre ogni figlio si comporta da scrittore 
			- chiusura del lato di scrittura di TUTTE le pipe tranne in quella di indice i che verra' usata dal figlio per comunicare col padre */
			for (j = 0; j < N; j++)
			{ 
				close(piped[j][0]); 

				if (j != i) 
				{ 
					close(piped[1][j]);
				}
			} 

			/* creazione della pipe per la comunicazione tra nipote e figlio */ 
			if (pipe(p) < 0)
			{ 
				printf("Errore: il figlio di indice %d non e' riuscito a creare la pipe per comunicare col nipote\n", i);
				exit(-1);
			} 

			/* creazione del processo nipote */
			if ((pid = fork()) < 0)
			{ 
				printf("Errore: il figlio di indice %d non e' riuscito a creare il suo processo nipote\n", i);
				exit(-2);
			}

			if (pid == 0)
			{ 
				/* CODICE DEI NIPOTI */ 

				/* per quanto riguarda i valori di ritorno possiamo fare lo stesso ragionamento fatto per i processi figli */ 

				printf("DEBUG-Sono il processo nipote con pid = %d, associato al processo figlio di indice %d e "
					"sto per eseguire il comando wc -l sul file %s per contare le sue righe\n", getpid(), i, argv[i + 1]); 
				
				/* chiudiamo i lati delle pipe non usati da ogni processo nipote: 
				- il lato di scrittura della pipe utilizzata dal rispettivo processo figlio
				- il lato di lettura delle pipe p creata per poter comunicare col figlio associato */

				close(piped[i][1]); 
				close(p[0]); 


				/* a questo punto conviene pensare di andare ad utilizzare wc in versione comando filtro, questo perche' cosi' 
				non andra' a stampare sulla pipe anche il nome del file del quale si devono contare le righe, dunque il processo 
				figlio potra' andare direttamente a leggere il numero di righe senza dover preoccuparsi di fermare la lettura prima 
				di leggere anche il nome del file stampato dal comando normale */ 

				/* per poter utilizzare il comando wc in modalita' filtro sul file associato, sara' dunque necessario andare a ridirezionare
				il suo standard input, in modo tale che questo vada a leggere dal file piuttosto che da tastiera */ 
				close(0); 
				
				/* tentiamo l'apertura del file come stdin */ 
				if ((fdr = open(argv[i + 1], O_RDONLY)) < 0) 
				{ 
					printf("Errore: il file %s non esiste oppure non e' leggibile\n", argv[i + 1]); 
					exit(-3); 
				} 

				/* andiamo a ridirezionare pure lo stdout, in modo tale che il comando vada a scrivere sulla pipe creata dal figlio */ 
				close(1); 
				dup(p[1]); 	/* creiamo una copia del riferimento al lato di scirttura di p con la dup, che finira' nel posto 1 della TFA */

				/* a questo punto possiamo andare a chiudere anche p[1], perche' il riferimento al lato scrittura di p e' stato duplicato e 
				salvato nell'elemento di indice 1 della TFA */ 
				close(p[1]); 

				/* esecuzione del comando wc -l. Usiamo la execl in versione p perche' si tratta di un comando di sistema */
				execlp("wc", "wc", "-l", (char *) 0); 

				/* se si arriva ad eseguire questa parte di codice allora ci sara' stato un qualche errore nella exec. Nota che non possiamo 
				andare a stampare un errore su stdout perche' questo e' stato ridirezionato, usiamo dunque la funzione perror */ 
				perror("Errore: non e' stato possibile eseguire il comando wc con l'exec"); 
				exit(-4); 

			} 
			
			/* ritorno al codice dei figli */ 

			/* chisura del lato scrittura della pipe p */
			close(p[1]);
			
			/* lettura dalla pipe carattere per carattere */ 
			j = 0; 	/* indice per poter scrivere i caratteri letti da pipe direttamente dentro al buffer */
			while (read(p[0], &(buf[j]), 1) > 0) 
			{ 	
				j++; 
			} 

			/* a questo punto buf, che e' un array di char, conterra' la riga stampata dal nipote tramite l'esecuzione di wc. 
			per poterla trasformare in stringa ci sara' bisogno che l'utlimo carattere letto (\n) venga trasformato nel terminatore, 
			cosi' che questo buffer possa essere tratta come un stringa contenente delle cifre, affinche' si possa traddure in un effettivo 
			intero tramite la funzione atoi. 
			Tutto questo ha senso solo nella misura in cui il figlio sia riuscito a leggere qualcosa dalla pipe, dunque che il nipote abbia 
			effettivamente stampato qualcosa col wc, quindi faremo un controllo su j (diverso da 0 se si e' letto qualcosa) */ 
			
			if (j) 
			{
				buf[j - 1] = '\0'; 

				len = atoi(buf);
			}
			else
			{
				len = 0; 
			}

			/* invio della lunghezza trovata al padre */ 
			write(piped[i][1], &len, sizeof(int)); 

			/* attesa del processo nipote creato con recupero del suo valore */ 

			if ((pid = wait(&status)) < 0) 
			{ 
				printf("Errore: il figlio di indice %d non e' riuscito ad attendere il nipote con la wait\n", i); 
				exit(-4); 
			}

			if (WIFEXITED(status))
			{ 
				ritorno = WEXITSTATUS(status); 
				printf("DEBUG-Il processo nipote con pid = %d ha ritornato il valore %d\n", pid, ritorno); 
			} 
			else
			{ 
				printf("il processo nipote con pid = %d e' terminato in modo anomalo\n", pid); 
			} 

			exit(ritorno);
		} 
	} /* fine for */ 

	/* CODICE PROCESSO PADRE */ 

	/* chisura del lato di scrittura di TUTTE le N pipe */ 
	for (i = 0; i < N; i++)
	{ 
		close(piped[i][1]);
	} 

	/* lettura in ordine dei file dei valori ritornati dai figli con incremento del conto totale di linee */ 
	for (i = 0; i < N; i++)
	{
		read(piped[i][0], &len, sizeof(int)); 
		printf("DEBUG-il processo figlio di indice %d ha trovato %d linee nel file %s\n", i, len, argv[i + 1]);

		len_sum += (long int)len; 
	}
	
	/* stampa del numero totale di righe trovate negli N file */
	printf("Il numero totale di righe trovate nei file passati e': %ld\n", len_sum); 

	/* attesa degli N figli con recupero e stampa dei valori tornati */ 
	for (i = 0; i < N; i ++) 
	{ 
		if ((pid = wait(&status)) < 0)
		{ 
			printf("Errore: non e' stato possibile attendere uno dei processi figli con la wait\n");
			exit(5); 
		}

		if (WIFEXITED(status))
		{ 	
			ritorno = WEXITSTATUS(status); 
			printf("Il processo figlio con pid = %d e' terminato ritornando il valore %d (se elevato problemi)\n", pid, ritorno); 
		}
		else
		{ 
			printf("Il processo figlio con pid = %d e' terminato in modo anomalo\n", pid); 
		} 
	}

	exit(0); 
} 

