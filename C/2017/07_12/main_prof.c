/* Soluzione della Prova d'esame del 12 Luglio 2017 - Parte C */
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <time.h>

/* definizione del TIPO pipe_t come array di 2 interi */
typedef int pipe_t[2];

/* definizione del TIPO strut per le singole strutture dati, che verranno inviate sulle pipe da parte dei figli al padre */
typedef struct {
	int pid_nipote; 	/* campo c1 del testo */
	int numero_linea;	/* campo c2 del testo */			
	char linea_letta[250];	/* campo c3 del testo */
				/* bastano 250 caratteri per contenere ogni riga insieme con il terminatore di linea e con il terminatore di stringa (vedi specifica) */
		} strut;

int mia_random(int n)
{ 	/* funzione che calcola un numero random compreso fra 1 e n (fornita nel testo) */
   int casuale;
	casuale = rand() % n;
	casuale++;
	return casuale;
}

int main(int argc, char **argv) 
{
   /* -------- Variabili locali ---------- */
   int pid;			/* process identifier per le fork() */
   int N; 			/* numero di file passati sulla riga di comando (uguale al numero di numeri), che corrispondera' al numero di figli  */
   pipe_t *piped;		/* array dinamico di pipe per comunicazioni figli-padre  */
   pipe_t p;			/* una sola pipe per ogni coppia figlio-nipote */ 
   int i, j;			/* indici per i cicli */
   int X, r;			/* variabili usate dal nipote per il numero di linee (sicuramente strettamente minore di 255, garantito dalla parte shell) e per il valore random (anche lui < di 255) */
   int nr;			/* variabile usata dal padre per controllo sulla read dai figli */
   char opzione[5];		/* variabile stringa che serve per passare al comando head l'opzione con il numero delle linee da selezionare */
				/* supponiamo che bastino 3 cifre per il numero, oltre al simbolo di opzione '-' e al terminatore di stringa */
   strut S;			/* struttura usata dai figli e dal padre */
   int finito;                  /* variabile che serve al padre per sapere se non ci sono piu' strutture da leggere */
   int nrLinee; 		/* variabile che usa ogni figlio per calcolare le linee inviate dal nipote e che quindi rappresenta il numero random calcolato dal nipote */
   int ritorno; 		/* variabile che viene ritornata da ogni figlio al padre */
   int status;			/* variabile di stato per la wait */
   /* ------------------------------------ */
	
	/* controllo sul numero di parametri: almeno il nome di un file e il suo numero di linee */
	if (argc < 3 || (argc-1)%2) /* Meno di due parametri e non pari */  
	{
		printf("Errore: Necessari per %s almeno 2 parametri (nome file e numero linee e comunque in numero pari) e invece argc = %d\n", argv[0], argc);
		exit(1);
	}

	/* calcoliamo il numero di file/numeri passati e quindi di figli da creare */
	N = (argc - 1)/2;
	
	/* controlliamo i numeri passati: non usiamo ora la funzione atoi() per trasformare queste stringhe in numeri, dato che i numeri saranno convertiti dai nipoti. Quindi controlliamo il primo carattere (quello di indice 0) di ogni stringa numerica e se e' il carattere '-' oppure '0' vuol dire che il numero non e' strettamente positivo e diamo errore */
	for (i=0; i < N; i++)
	{
	/* le stringhe che rappresentano i numeri si trovano con l'indice i moltiplicato per 2 e sommando 2, cioe' nelle posizioni 2, 4, 6, etc. e quindi saltando le stringhe che rappresentano i nomi dei file */
		if (argv[(i*2)+2][0] == '-'  || argv[(i*2)+2][0] == '0') 
		{
			printf("Errore %s non rappresenta un numero strettamente positivo \n", argv[(i*2)+2]);
			exit(2);
		}
	}

	printf("DEBUG-Numero processi da creare %d\n", N);

	/* allocazione dell'array di N pipe */
	piped = (pipe_t *) malloc (N * sizeof(pipe_t));
	if (piped == NULL)
	{
		printf("Errore nella allocazione della memoria\n");
		exit(3);
	}
	
	/* creazione delle N pipe figli-padre */
	for (i=0; i < N; i++)
	{
		if (pipe(piped[i]) < 0)
		{
			printf("Errore nella creazione della pipe di indice i = %d\n", i);
			exit(4);
		}
	}

	/* le N pipe nipoti-figli deriveranno dalla creazione di una pipe in ognuno dei figli che poi creeranno un nipote */
		
	/* ciclo di generazione dei figli */
	for (i=0; i < N; i++)
	{
		if ( (pid = fork()) < 0)
		{
			printf("Errore creazione figlio %d-esimo\n", i);
			exit(5);
		}
		
		if (pid == 0) 
		{
			/* codice del figlio Pi */
			/* in caso di errori nei figli o nei nipoti decidiamo di tornare -1 che corrispondera' al valore 255 che non puo' essere un valore accettabile di ritorno (questo e' garantito dalla parte SHELL) */

			/* chiusura dei lati delle pipe non usate nella comunicazione con il padre */
			for (j=0; j < N; j++)
			{
				/* ogni figlio NON legge da nessuna pipe e scrive solo sulla sua che e' la i-esima! */

				close(piped[j][0]);
				if (i != j) close(piped[j][1]);
			}

			/* per prima cosa, creiamo la pipe di comunicazione fra nipote e figlio */
		  	if (pipe(p) < 0)
                	{	
                        	printf("Errore nella creazione della pipe fra figlio e nipote\n");
                        	exit(-1); /* si veda commento precedente */
                	}

			if ( (pid = fork()) < 0) /* ogni figlio crea un nipote */
			{
				printf("Errore nella fork di creazione del nipote\n");
				exit(-1); /* si veda commento precedente */
			}	

			if (pid == 0) 
			{
				/* codice del nipote */
		 		/* in caso di errori nei figli o nei nipoti decidiamo di tornare -1 che corrispondera' al valore 255 che non puo' essere un valore accettabile di ritorno */
				printf("DEBUG-Sono il processo nipote del figlio di indice %d e pid %d e sto per selezionare delle linee del file %s che ha %s linee\n", i, getpid(), argv[(i*2)+1], argv[(i*2)+2]);

				/* chiusura della pipe rimasta aperta di comunicazione fra figlio-padre che il nipote non usa */
				close(piped[i][1]);

				/* inizializziamo il seme per la generazione random di numeri (come indicato nel testo) */
				srand(time(NULL)); 
				/* il nipote ricava dal parametro 'giusto' il numero di linee del suo file e quindi calcola in modo random il numero di linee che inviera' al figlio */
			 	X=atoi(argv[(i*2)+2]);
                		r=mia_random(X);
				printf("DEBUG-Il nipote di indice %d ha calcolato come numero random %d (rispetto al numero %d)\n", i, r, X);
				/* costruiamo la stringa di opzione per il comando head */
				sprintf(opzione, "-%d", r);

				/* ogni nipote deve simulare il piping dei comandi nei confronti del figlio e quindi deve chiudere lo standard output e quindi usare la dup sul lato di scrittura della propria pipe */
				close(1);
				dup(p[1]); 			
				/* ogni nipote adesso puo' chiudere entrambi i lati della pipe: il lato 0 NON viene usato e il lato 1 viene usato tramite lo standard ouput */
				close(p[0]);
				close(p[1]);

				/* si puo' valutare se procedere con la ridirezione dello standard error su /dev/null: nel caso le istruzioni da aggiungere sono
				close(2);
				open("/dev/null", O_WRONLY); */

				/* Il nipote diventa il comando head: bisogna usare le versioni dell'exec con la p in fondo in modo da usare la variabile di ambiente PATH */	    
				/* le stringhe che rappresentano i nomi dei file si trovano con l'indice i moltiplicato per 2 e sommando 1, cioe' nelle posizioni 1, 3, 5, etc. e quindi saltando le stringhe che rappresentano i numeri */
				execlp("head", "head", opzione, argv[(i*2)+1], (char *)0);
				/* attenzione ai parametri nella esecuzione di head: opzione, nome del file e terminatore della lista */ 
				/* NOTA BENE: dato che il testo richiedeva l'uso del comando head, se per caso il file NON esiste o non fosse leggibile, il comando head fallirebbe e non riuscirebbe a mandare alcuna linea al figlio e quindi il valore calcolato dal figlio come numero di linee lette dal nipote e tornato dal figlio al padre dovrebbe essere interpretato come problemi nel nipote! */ 
				
				/* NON si dovrebbe mai tornare qui!!*/
				/* usiamo perror che scrive su standard error, dato che lo standard output e' collegato alla pipe */
				perror("Problemi di esecuzione della head da parte del nipote");
				exit(-1); /* si veda commento precedente */
			}

			/* codice figlio */
			/* ogni figlio deve chiudere il lato che non usa della pipe di comunicazione con il nipote */
			close(p[1]);
			/* adesso il figlio legge dalla pipe fino a che ci sono caratteri e cioe' linee inviate dal nipote tramite la head */
			j=0; /* inizializziamo l'indice della linea */
			nrLinee=0; /* inizializziamo il numero di linee lette che alla fine rappresentera' il numero random calcolato dal nipote */
			S.pid_nipote=pid; /* inizializziamo il campo con il pid del processo nipote */
		        while (read(p[0], &(S.linea_letta[j]), 1))
			{	
				/* se siamo arrivati alla fine di una linea */
				if (S.linea_letta[j] == '\n')  
				{ 
					/* si deve incrementare il numero di linee */
					nrLinee++; 
					/* nell'array linea_letta abbiamo l'ultima linea ricevuta: incrementiamo la sua lunghezza per inserire il terminatore di stringa. Infatti ... */
					j++;		
					S.linea_letta[j]='\0'; /* poiche' il padre deve stampare la linea su standard output, decidiamo che sia il figlio a convertire tale linea in una stringa per facilitare il compito del padre */
					S.numero_linea=nrLinee;
					/* il figlio comunica al padre */
					write(piped[i][1], &S, sizeof(S));
					j=0; /* riazzeriamo l'indice per la prossima linea */ 
				}
				else
					j++; /* incrementiamo l'indice della linea */
			}	

			/* il figlio deve aspettare il nipote per correttezza */
			/* se il nipote e' terminato in modo anomalo decidiamo di tornare -1 che verra' interpretato come 255 e quindi segnalando questo problema al padre */
			ritorno=-1;
			if ((pid = wait(&status)) < 0)
				printf("Errore in wait\n");
			if ((status & 0xFF) != 0)
    				printf("Nipote con pid %d terminato in modo anomalo\n", pid);
    			else
			{	
				printf("DEBUG-Il nipote con pid=%d ha ritornato %d\n", pid, (int)((status >> 8) & 0xFF)); 
				ritorno=nrLinee;
			}

			exit(ritorno); /* il figlio ritorna il numero di linee ricevute dal nipote che rappresenta, implicitamente, il numero random calcolato dal nipote, come richiesto dal testo */
		}
	}
	
	/* codice del padre */
	/* chiusura di tutti i lati delle pipe che non usa, cioe' tutti i lati di scrittura */
	for (i=0; i < N; i++)
		close(piped[i][1]);

	/* il padre recupera le informazioni dai figli: prima in ordine di strutture e quindi in ordine di indice d'ordine dei figli */
        finito = 0; /* all'inizio supponiamo che non sia finito nessun figlio */
        while (!finito)
        {
                finito = 1; /* mettiamo finito a 1 perche' potrebbero essere finiti tutti i figli */
                for (i=0; i<N; i++)
                {
                 	/* si legge la struttura inviata  dal figlio i-esimo */
                        nr = read(piped[i][0], &S, sizeof(S));
			/* si deve controllare se il padre ha letto qualcosa, dato che se un figlio e' terminato non inviera' piu' nulla e quindi il padre NON puo' eseguire la stampa */ 
                        if (nr != 0)
                        {
                        	finito = 0; /* almeno un processo figlio non e' finito */
				/* Nota bene: la stampa della linea con il formato %s NON ha problemi perche' il figlio ha passato una stringa */
                                printf("Il nipote con pid %d ha letto dal file %s nella riga %d questa linea:\n%s",  S.pid_nipote, argv[(i*2)+1], S.numero_linea, S.linea_letta); /* NOTA BENE: dato che la linea contiene il terminatore di linea nella printf NON abbiamo inserito lo \n alla fine */
                        }
                }
         }

	/* il padre aspetta i figli */
	for (i=0; i < N; i++)
	{
		if ((pid = wait(&status)) < 0)
		{
			printf("Errore in wait\n");
			exit(6);
		}

		if ((status & 0xFF) != 0)
    			printf("Figlio con pid %d terminato in modo anomalo\n", pid);
    		else
		{ 
			ritorno=(int)((status >> 8) &	0xFF); 
		  	printf("Il figlio con pid=%d ha ritornato %d (se 255 problemi nel figlio o nel nipote; se 0 problemi nel nipote)\n", pid, ritorno);
		}
	}

	exit(0);
}
