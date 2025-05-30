/* Soluzione della parte C del compito del 10 Luglio 2024 */
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <time.h>

typedef int pipe_t[2];
typedef char tipoL[250]; /* tipo array statico: per ogni linea letta dai figli dal proprio file come indicato dal testo bastano 250 caratteri per linea e terminatore di linea: e' stato definito un tipo come suggerito in una nota del testo */

int mia_random(int n)
{  /* funzione che calcola un numero random compreso fra 1 e n (fornita nel testo) */
   int casuale;
   casuale = rand() % n;
   casuale++;
   return casuale;
}

int main (int argc, char **argv)
{
  /* -------- Variabili locali ---------- */
  int N; 		/* numero di file/processi */
  /* ATTENZIONE NOME N imposto dal testo! */
  int pid;              /* pid per fork */
  pipe_t *pipes;  	/* array di pipe usate a pipeline da primo figlio, a secondo figlio .... ultimo figlio e poi a padre: ogni processo (a parte il primo) legge dalla pipe i-1 e scrive sulla pipe i */
  int n,j,k; 		/* indici */
  /* ATTENZIONE NOME n imposto dal testo! */
  int fd; 		/* file descriptor */
  /* ATTENZIONE NOME L imposto dal testo! */
  tipoL linea;        	/* linea corrente */
  /* ATTENZIONE NOME linea imposto dal testo! */
  tipoL *tutteLinee;    /* array dinamico di linee */
  /* ATTENZIONE NOME tutteLinee imposto dal testo! */
  int L;		/* variabile per salvare la lunghezza in linee dei file */
  int r;                /* variabile per i figli per calcolare il valore random */
  int nroLinea;		/* variabile per contare le linee dei file */
  int nr,nw;            /* variabili per salvare valori di ritorno di read/write da/su pipe */
  int pidFiglio, status, ritorno;	/* per valore di ritorno figli */
  /* ------------------------------------ */

/* controllo sul numero di parametri: almeno 2 file con la loro lunghezza in linee e quindi almeno 4 paramentri (5 con il nome dell'eseguibile) in numero pari */
if ((argc < 5) || (argc-1)%2)
{
	printf("Errore nel numero dei parametri dato che argc=%d (bisogna che il numero dei parametri sia pari e quindi che ci siano almeno due nomi di file insieme con la loro lunghezza in linee!)\n", argc);
	exit(1);
}

/* Calcoliamo il numero di file/lunghezze passati */
N = (argc - 1)/2;

/* Controlliamo i numeri passati: non usiamo ora la funzione atoi() per trasformare queste stringhe in numeri, dato che i numeri saranno convertiti dai figli. Quindi controlliamo il primo carattere (quello di indice 0) di ogni stringa numerica e se e' il carattere '-' oppure '0' vuol dire che il numero non e' strettamente positivo e diamo errore */
for (n=0; n < N; n++)
{
	/* le stringhe che rappresentano i numeri si trovano con l'indice n moltiplicato per * 2 e sommando 2, cioe' nelle posizioni 2, 4, 6, etc. e quindi saltando le stringhe che rappresentano i nomi dei file */
	if (argv[(n*2)+2][0] == '-'  || argv[(n*2)+2][0] == '0')
	{
		printf("Errore %s non rappresenta un numero strettamente positivo \n", argv[(n*2)+2]); 
		exit(2);
	}
}

printf("DEBUG-Numero di processi da creare %d\n", N);

/* allocazione pipe */
if ((pipes=(pipe_t *)malloc(N*sizeof(pipe_t))) == NULL)
{
	printf("Errore allocazione pipe\n");
	exit(3); 
}

/* creazione pipe */
for (n=0;n<N;n++)
	if(pipe(pipes[n])<0)
	{
		printf("Errore creazione pipe\n");
		exit(4);
	}

/* allocazione array di linee: lo fa il padre cosi' i figli se lo trovano gia' allocato! */
if ((tutteLinee=(tipoL *)malloc(N*sizeof(tipoL))) == NULL)
{
	printf("Errore allocazione array di linee\n");
	exit(5); 
}

/* creazione figli */
for (n=0;n<N;n++)
{
	if ((pid=fork())<0)
	{
		printf("Errore creazione figlio\n");
		exit(6);
	}
	if (pid == 0)
	{ /* codice figlio */
		printf("DEBUG-Sono il figlio %d e sono associato al file %s che ha lunghezza in linee %s\n", getpid(), argv[n*2+1], argv[n*2+2]);
		/* nel caso di errore in un figlio decidiamo di ritornare il valore -1 che sara' interpretato dal padre come 255 (valore NON ammissibile) */

		/* chiusura pipes inutilizzate */
		for (j=0;j<N;j++)
		{	/* si veda commento nella definizione dell'array pipes per comprendere le chiusure */
                	if (j!=n)
                		close (pipes[j][1]);
                	if ((n == 0) || (j != n-1))
               			close (pipes[j][0]);
        	}
	
		/* apertura del file associato in sola lettura: l'indice da usare NON e' il solito n+1, ma n va moltiplicato per 2 per saltare le lunghezze! */
		if ((fd=open(argv[n*2+1],O_RDONLY))<0)
		{
			printf("Impossibile aprire il file %s\n", argv[n*2+1]);
			exit(-1);
		}
		
		srand(time(NULL)); /* inizializziamo il seme per la generazione random di numeri */
		/* individuiamo il numero di righe del file associato */
		L=atoi(argv[n*2+2]);
		/* ogni figlio deve calcolare in modo randomico il valore che gli consentira' di individuare la linea da scrivere nell'array che va mandato avanti, fino al padre */
		r=mia_random(L);
		printf("DEBUG-Figlio di indice %d calcolato il valore r=%d\n", n, r);

		/* inizializziamo il numero di linea */
		nroLinea=0; 
		/* inizializziamo l'indice dei caratteri letti per ogni singola linea */
		j = 0;
		/* con un ciclo leggiamo le linee, fino a che non si arriva a quella giusta, come richiede la specifica */
        	while(read(fd,&(linea[j]),1) != 0)
        	{
          		if (linea[j] == '\n') /* siamo a fine linea */
                	{
				nroLinea++; /* la prima linea sara' la numero 1 (come indicato in una nota del testo!) */
				if (nroLinea == r) /* siamo sulla linea giusta */
	                	{

					if (n != 0)
	                		{
						/* se non siamo il primo figlio, dobbiamo aspettare l'array dal figlio precedente per mandare avanti */
                				nr=read(pipes[n-1][0],tutteLinee,N*sizeof(tipoL));
        					/* per sicurezza controlliamo il risultato della lettura da pipe */
                				if (nr != N*sizeof(tipoL))
                				{
                       					printf("Figlio %d ha letto un numero di byte sbagliati %d\n", n, nr);
                       					exit(-1);
                				}
                			}	
					/* a questo punto si deve inserire la linea letta nel posto giusto */
					for (k=0; k <= j; k++)
                       			{ 	/* ricordarsi che non si puo' fare una copia diretta di un array! */
						tutteLinee[n][k]=linea[k];
                       			}

					/* ora si deve mandare l'array in avanti */
        				nw=write(pipes[n][1],tutteLinee,N*sizeof(tipoL));
        				/* anche in questo caso controlliamo il risultato della scrittura */
        				if (nw != N*sizeof(tipoL))
        				{
               					printf("Figlio %d ha scritto un numero di byte sbagliati %d\n", n, nw);
               					exit(-1);
        				}	
					/* si puo' uscire dal ciclo */
					break;
        			}	
				/* si deve azzerare l'indice della linea */
				j = 0;
        		}	
			else
               		{
				j++; /* incrementiamo l'indice della linea */
        		}		
        	}		
		/* ogni figlio deve tornare il numero random */
		exit(r);
	}
} /* fine for */

/* codice del padre */
/* chiusura di tutte le pipe che non usa */
for (n=0;n<N;n++) 
{
	close (pipes[n][1]);
	if (n != N-1) close (pipes[n][0]); 
}

/* il padre deve leggere l'array di linee inviato dall'ultimo figlio */
nr=read(pipes[n-1][0],tutteLinee,N*sizeof(tipoL));
/* per sicurezza controlliamo il risultato della lettura da pipe */
if (nr != N*sizeof(tipoL))
{
	printf("Padre ha letto un numero di byte sbagliati %d\n", nr);
       	/* deciso di commentare questa 
	exit(7);
	cosi' se i figli incorrono in un qualche problema e quindi la pipeline si rompe, il padre comunque eseguira' l'attesa dei figli */
}
else
{	
	printf("Il padre ha ricevuto le seguenti linee dai figli:\n");
	/* il padre deve scrivere le linee sullo standard output */
	for (n=0;n<N;n++)
	{	
		printf("Linea ricevuta dal figlio di indice %d:\n", n);
		for (k=0; k<250; k++)
		{
			/* fino a che non incontriamo il fine linea scriviamo sulllo standard output */
			write(1, &(tutteLinee[n][k]), 1);
			if (tutteLinee[n][k] == '\n')
                	{
				/* quando troviamo il terminatore di linea, ... */
				break; /* usciamo dal ciclo for piu' interno */
			}	
		}
	}	
}	

/* Il padre aspetta i figli */
for (n=0; n < N; n++)
{
        pidFiglio = wait(&status);
        if (pidFiglio < 0)
        {
                printf("Errore in wait\n");
                exit(8);
        }
        if ((status & 0xFF) != 0)
                printf("Figlio con pid %d terminato in modo anomalo\n", pidFiglio);
        else
        { 
		ritorno=(int)((status >> 8) & 0xFF);
        	printf("Il figlio con pid=%d ha ritornato %d (se 255 problemi)\n", pidFiglio, ritorno);
        } 
}
exit(0);
}
