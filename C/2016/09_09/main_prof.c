/* Soluzione della parte C dell'esame del 9 Settembre 2016. Nota bene: questa soluzione non ha bisogno di usare array dinamici dato che il numero di processi e' noto staticamente e pari al numero di lettere minuscole dell'alfabeto inglese (26 lettere) */
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#define N 26 	/* numero di figli da creare: il valore e' COSTANTE! Definire questa costante serve per poter riutilizzare, senza modifiche, codice scritto con N variabile! */

/* definizione del TIPO pipe_t come array di 2 interi */
typedef int pipe_t[2];

/* definizione del TIPO data in cui il primo campo rappresenta il carattere e il secondo campo il numero di occorrenze di quel carattere trovate nel file */
typedef struct	{
	char ch; 	/* campo v1 del testo */
	long int occ;	/* campo v2 del testo */
		} data;

/* funzione bubblesort che rispetto a quella fornita e' stata adattata alle esigenze del testo e, quindi, ordina l'array di strutture di tipo data passato in base al valore del campo occ delle strutture facenti parte dell'array */
void bubbleSort(data *v, int dim)
{
   int i;
   data tmp;
   int ordinato = 0;
	while(dim > 0 && !ordinato)
	{
		ordinato = 1;
		for(i=0; i < dim-1; i++)
		{
			if(v[i].occ > v[i+1].occ)
			{
				tmp = v[i];
				v[i] = v[i+1];
				v[i+1] = tmp;
				ordinato = 0;
			}
		}
		dim--;
	}
}

int main(int argc, char **argv)
{
   int pid[N]; 				/* array STATICO di process identifier dei processi generati tramite fork: l'array di pid serve perche' il padre deve ricavare il pid dall'indice del processo che avra' generato la specifica struttura */
   char c; 				/* carattere letto dai figli */
   pipe_t pipes[N]; 			/* array STATICO di pipe usate a pipeline da primo figlio, a secondo figlio .... ultimo figlio e poi a padre: ogni processo legge dalla pipe i-1 (a parte il primo figlio) e scrive sulla pipe i */
   int i, j; 				/* indici  per i cicli */
   int fd; 				/* file descriptor del file aperto da ogni figlio in lettura */
   data d[N]; 				/* array STATICO di strutture che i figli si passano nella pipeline, fino al padre */
   char C; 				/* carattere da cercare da parte di ogni figlio */
   long int curr_occ; 			/* conteggio corrente */
   int nr,nw;				/* variabili per salvare valori di ritorno di read e write da/su pipe */
   int pidFiglio, status, ritorno;	/* per valore di ritorno figli */
	
   	/* controllo sul numero di parametri: solo il nome di un file */
	if (argc != 2)
	{
		printf("Numero di parametri errati, dato che argc=%d: ci vuole solo il nome di un file\n", argc);
		exit(1);
	}

	/* creazione delle 26 pipe, usate in pipeline: N.B. uso costante N */
	for (i = 0; i < N; i++)
	{
		if (pipe(pipes[i]) < 0)
		{
			printf("Errore nella creazione delle pipe\n");
			exit(2);
		}
	}

	/* generiamo i 26 figli (N e' una costante definita uguale a 26!) */
	/* N.B. Dato che il padre deve stampare il pid dei figli una volta ricevuto l'array e ordinatolo, si devono memorizzare i pid dei figli! */
	for (i = 0; i < N; i++)
	{
		if ((pid[i] = fork()) < 0)
		{
			printf("Errore nella creazione figlio %d-esimo\n", i);
			exit(3);
		}
		
		if(pid[i] == 0)
		{
			/* codice del figlio Pi */
			/* definiamo a quale carattere e' assolciato questo figlio */
			C = 'a'+i;  	/* ogni figlio deve essere associato ad una lettera dell'alfabeto inglese (minuscola), iniziando dalla lettera 'a' */
			printf("DEBUG-Sono il figlio di indice %d e pid %d e sto per cercare il carattere %c nel file %s\n", i, getpid(), C, argv[1]); 

			/* chiusura dei lati delle pipe non usate nella pipeline */ 
			/* per capire le chiusure, fare riferimento al commento riportato per l'array pipes */
			for (j=0; j < N; j++)
			{
				if (j!=i)
					close (pipes[j][1]);
				if ((i == 0) || (j != i-1))
					close (pipes[j][0]);
			}

			/* ogni figlio deve aprire il file (l'unico che e' stato passato) in sola lettura */
			/* N.B. l'apertura va ripetuta da tutti i figli perche' ogni figlio deve avere il proprio file-pointer per cercare il carattere associato! */
			if((fd = open(argv[1], O_RDONLY)) < 0)
			{
				printf("Errore nella apertura del file %s\n", argv[1]);
				/* dato che i figli devono tornare un carattere in condizioni di non errore, torniamo il valore -1 che sara' interpretato dal padre come 255 e quindi non una codifica ASCII di un carattere! */
				exit(-1);
			}
			
			curr_occ = 0L;	/* inizializziamo il conteggio: N.B. VALORE 0 COME LONG INT! */
			/* N.B. in questo caso ogni figlio NON puo' usare l'elemento i-esimo dell'array dato che, a parte di primo figlio, viene sempre letto TUTTO l'array e quindi le informazioni inserite dell'elemento i-simo verrebbero perse!*/

		  	/* ogni figlio deve leggere il proprio file, un carattere alla volta */
			while (read(fd, &c, 1))	
			{
				/* cerchiamo il carattere */
				if (c == C)
					curr_occ++;	/* trovato il carattere e quindi si incrementa il conteggio */
			}

			/* lettura da pipe dal figlio precedente dell'array di strutture per tutti i figli a parte il primo */
			if (i != 0)
			{
				nr=read(pipes[i-1][0],d,sizeof(d));	/* N.B. d e' un array statico e quindi si puo' usare il sizeof per ricavare la lunghezza dell'array che sara' uguale a N * sizeof(data) */
				/* N.B. In caso di pipeline e' particolarmente importante che la lettura sia andata a buon fine e che quindi la pipeline non si sia 'rotta'; si deve quindi controllare il valore di ritorno della read da pipe! */
				if (nr != sizeof(d)) 
	 			{ 
					printf("Errore in lettura da pipe[%d]\n", i);
					exit(-1);
				}
			}

			/* inseriamo le informazioni del figlio corrente nella posizione giusta e quindi nella posizione i-esima */
			d[i].ch = C;
			d[i].occ = curr_occ;

			/* tutti i figli mandano in avanti (cioe' al figlio successivo, a parte l'ultimo figlio che manda al padre) un array di strutture di dimensione fissa (e costante) */
			nw=write(pipes[i][1],d,sizeof(d));
			/* N.B. In caso di pipeline e' particolarmente importante che anche la scrittura sia andata a buon fine e che quindi la pipeline non si sia 'rotta'; si deve quindi controllare il valore di ritorno della write su pipe! */
			if (nw!=sizeof(d)) 
			{ 
				printf("Errore in scrittura da pipe[%d]\n",i);
				exit(-1);
			}

			/* torniamo l'ultimo carattere letto (che sara' chiaramente uguale per tutti i figli) */
			exit(c);
		}
	} /* fine for */

	/* codice del padre */
	/* chiusura lati delle pipe non usati: tutti meno quello l'ultima in lettura */
	for (i=0; i < N; i++)
	{
		close (pipes[i][1]);
		if (i != N-1) close (pipes[i][0]);
	}

	/* il padre deve leggere l'array di strutture che gli arriva dall'ultimo figlio */
	/* quindi al padre arriva una singola informazione, rappresentata pero' da un array che quindi contiene piu' elementi */
	nr=read(pipes[N-1][0],d,sizeof(d));
	/* N.B. anche il padre deve controllare di avere letto qualcosa! */
	if (nr!=sizeof(d))
	{
		printf("Errore in lettura da pipe[N-1] per il padre\n");
        	/* deciso di commentare questa 
		exit(4); 
		cosi' se i figli incorrono in un qulche problema e quindi la pipeline si rompe, il padre comunque eseguira' l'attesa dei figli */
	}
	else 
	{
		/* ordiniamo l'array ricevuto, secondo il campo occ delle varie strutture */
		bubbleSort(d,N);
		/* padre deve stampare quanto ordinato rispettando la specifica */
		for (i=0; i < N; i++)
		{
			printf("Il figlio di indice %d e pid %d ha trovato %ld occorrenze del carattere %c\n", d[i].ch-'a', pid[(d[i].ch-'a')], d[i].occ, d[i].ch);
			/* N.B. l'indice del figlio NON e' 'i' dato che l'array e' stato ordinato e quindi per individuare l'indice si deve usare l'informazione relativa al carattere cui il conteggio si riferisce sottraendo 'a'; chiaramente questo calcolo dell'indice va usato anche per selezionare il pid giusto dall'array pid */ 
		}	 
	}

	/* Il padre aspetta i figli */
	for (i=0; i < N; i++)
	{
        	if ((pidFiglio = wait(&status)) < 0)
        	{
                	printf("Errore in wait\n");
                	exit(5);
        	}

        	if ((status & 0xFF) != 0)
                	printf("Figlio con pid %d terminato in modo anomalo\n", pidFiglio);
        	else
                { 
			ritorno=(int)((status >> 8) & 0xFF);
                  	printf("Il figlio con pid=%d ha ritornato il carattere %c (in decimale %d, se 255 problemi)\n", pidFiglio, ritorno, ritorno);
                }
	}

	exit(0);
}
