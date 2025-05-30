/* Programma C main.c che risolve la parte C della prova di esame totale tenutasi l'8 giugno 2016 */

#include <stdlib.h> 
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/wait.h> 
#include <string.h> 
#include <ctype.h>
#include <signal.h> 
#include <time.h>

/* Definizione dei bit dei permessi (in ottale) per gli eventuali file creati */
#define PERM 0644 	

/* Definizione del TIPO pipe_t che rappresenta un array di due interi, che useremo per la gestione dei lati delle varie pipe create */
typedef int pipe_t[2];

/* Definizione della funzione handler del SIGPIPE, che si limita ad effettuare una stampa di debug. Per rendere questa stampa quanto piu' utile possibile si decide
di voler stampare anche l'indice del processo la cui scrittura da pipe ha causato il SIGPIPE; dal momento che l'handler per definzione della signal accetta solo un intero,
che altro non e' che l'identificatore del segnale che ha generato la sua chiamata, non possiamo passare direttamente il valore dell'indice, dunque quello che si fa e' rendere
la variabile indice dei processi direttamente una variabile con scope globale, in modo tale che il suo valore possa essere visto anche dentro all'handler */
int n; 				
void handler(int signo)
{ 
    signal(SIGPIPE, SIG_IGN); /* Operazione di trattamento dei segnali stile SystemV */

    printf("Il processo padre ha ricevuto il SIGPIPE (segnale no. %d) a seguito di un tentativo di scrittura sulla pipe di comunicazione col "
            "figlio di indice %d, dunque questo figlio e' terminato\n", signo, n); 
    
    signal(SIGPIPE, handler); /* Reinstallazione dell'handler, altra operazione per trattamento stile SystemV */
}

/* Definizione della funzione che calcola un valore numerico compreso tra 0 e n - 1 (riportata nel testo)*/
int mia_random(int n)
{ 
    int casuale; 
    casuale = rand() % n; 
    return casuale; 
}

int main (int argc, char **argv)
{
	/*---------------- Variabili locali -----------------*/
	int pid; 			/* identificatore dei processi figli creati con fork ed attesi con wait */
	int N; 				/* numero di file passati per paraemtro, e dunque di processi figli creati */
	int H; 				/* lunghezza in linee dei file passati per parametro. Si suppone infatti che abbiano tutti stessa lunghezza, 
						e che questa sia strettamente minore di 255 */
	int k; 				/* indice generico per i cicli */
	int fcreato; 		/* file descriptor associato alla creazione del file /tmp/creato da parte del padre. 
						Dal momento che il file viene creato nel padre PRIMA della creazione dei processi figli, 
						tutti i figli condivideranno l'I/O pointer del file, dunque NON si andanno a sovrascrivere 
						le scritture degli altri figli, infatti ogni scrittura aggiorna la posizine del puntatore nel file */
	int fdr; 			/* file descriptor associato all'apertura del proprio file da parte dei figli */
	char linea[255]; 	/* buffer di caratteri usato da ogni figlio per memorizzare la linea corrente. Si e' scelto 255 
						come valore ragionevole per la massima lunghezza delle righe */
	pipe_t *pipe_fp; 	/* array dinamico di pipe per lo schema di comunicazione nel verso da figli a padre */
	pipe_t *pipe_pf; 	/* array dinamico di pipe per lo schema di comunicazione nel verso da Padre a figli */
	int ran; 			/* variabile atta a contenere i valori randomici generati dalla funzione mia_random() */
    int lung;           /* lunghezza della riga corrente ricevuta dal padre da parte di ogni figlio */
	int ran_lung; 		/* lunghezza scelta in modo randomico ad ogni iterazione su riga inviata da un qualche figlio al padre */
	int status; 		/* variabile di stato per la wait */
	int ritorno; 		/* valore ritornato dai processi figli. viene utilizzato direttamente nei figli come contatore di caratteri scritti */
	int nr, nw; 		/* variabili di controllo per lettura/scrittura da/su pipe */
	/*---------------------------------------------------*/
	
	/* Controllo lasco numero di parametri passati */
	if (argc < 6)
	{ 
		printf("Errore: numero di parametri passati a %s errato - Attesi almeno 5 parametri [ (program) file1 file2 file3 file4 ... numpos] - Passati: %d\n", argv[0], argc - 1); 
		exit(1);
	}

	/* Calcolo del valore di N */
	N = argc - 2; 

	/* Recupero di H e controllo della sua validita' */
	H = atoi(argv[argc - 1]); 
	if (H <= 0 || H >= 255) 
	{ 
		printf("Errore: il parametro %s non e' un numero intero compreso nel range (0, 255)\n", argv[argc - 1]); 
		exit(2); 
	}

    /* Dal momento in cui se un processo figlio non riuscisse ad aprire il proprio file terminerebbe, il padre si troverebbe ad inivare inevitabilemente un messaggio (indice randomico)
        ad una pipe SENZA LETTORE, dunque riceverebbe il SIGPIPE e terminerebbe dunque in modo anomalo senza attendere i processi figli. Dunque per ovviare alla cosa si va a trattare il 
        SIGPIPE mediante un apposito gestore chiamato handler */
    signal(SIGPIPE, handler); 

    /* Inizializzazione del seme per la generazione randomica da parte della rand() */
    srand(time(NULL));

    /* Creazione del file /tmp/creato. Dal momento che si tratta di un file temporaneo usiamo la open con l'opzione O_TRUNC in modo tale che ad ogni esecuzione questo venga
        troncato e il contenuto dunque cancellato */
    if ((fcreato = open("/tmp/creato", O_CREAT|O_WRONLY|O_TRUNC, PERM)) < 0)
    { 
        printf("Errore: creazione da parte del padre del file '/tmp/creato' non riuscita\n");
        exit(3);
    }


	/* Allocazione dinamica della memoria per i due array di pipe. Dal momento che serviranno sempre per la comunicazione tra padre e figli (i entrambi i versi), ciascun array 
		necessita di N pipe */
	pipe_fp = (pipe_t *)malloc(N * sizeof(pipe_t));
	pipe_pf = (pipe_t *)malloc(N * sizeof(pipe_t));  
	if ((pipe_fp == NULL) || (pipe_pf == NULL))
	{ 
		printf("Errore: allocazione dinamica della memoria per gli array di pipe non riuscita\n");
		exit(4);
	}

	/* Ciclo di creazione delle pipe */
	for (k = 0; k < N; k++)
	{
		if (pipe(pipe_fp[k]) < 0)
		{
			printf("Errore: creazione della pipe di indice %d per la comunicaizone nel verso figli->padre non riuscita\n", k);
			exit(5); 
		}
		if (pipe(pipe_pf[k]) < 0)
		{
			printf("Errore: creazione della pipe di indice %d per la comunicaizone nel verso padre->figli non riuscita\n", k);
			exit(6); 
		}
	}

	/* Ciclo di creazione dei processi figli */
	for (n = 0; n < N; n++)
	{ 
		if ((pid = fork()) < 0)
		{ 
			printf("Errore: fork di creazione del processo figlio di indice %d non riuscita\n", n); 
			exit(7); 
		}

		if (pid == 0)
		{ 
			/* CODICE DEI FIGLI */

			/* Si decide che in caso i processi figli incorrano in qualche tipo di errore ritornino il valore -1 (255 senza segno), cosi' che la terminazione dovuta ad un errore possa
				essere distinta rispetto a quella normale. Infatti nei casi normali viene tornato dai figli il numero di caratteri scritti, ma al massimo un proceso figlio scrive 1 
                CARATTERE PER RIGA, ma H < 255 per specifica. Dunque in OGNI caso sara' possibile distinguere il modo in cui e' terminato ogni figlio */
            
            /* Ogni processo figlio Pn: 
                - Scrive sulla pipe di indice n con verso figli->padre la lunghezza della linea corrente 
                - Legge dalla pipe di indice n con verso padre->figli l'indice random 
               Dunque tutti gli altri lati NON utilizzati possono essere chiusi */
            for (k = 0; k < N; k++)
            { 
                close(pipe_fp[k][0]); 
                close(pipe_pf[k][1]); 

                if (k != n) 
                { 
                    close(pipe_fp[k][1]); 
                    close(pipe_pf[k][0]); 
                }
            }

            /* Apertura del file associato in lettura */
            if ((fdr = open(argv[n + 1], O_RDONLY)) < 0)
            { 
                printf("Errore: apertura in lettura del file %s da parte del figlio di indice %d non riuscita - file non leggibile oppure inesistente\n", argv[n + 1], n);
                exit(-1);
            }

            /* Lettura da file carattere per carattere */
            k = 0;          /* Inizializzazione a 0 dell'indice della linea */
            ritorno = 0;    /* Inizializzazione a 0 del contatore di scritture. Si noti che sara' anche il valore tornato al padre */
            while (read(fdr, &(linea[k]), sizeof(char)) > 0)
            { 
                if (linea[k] == '\n')
                { 
                    /* Trovata una riga */

                    /* Incremento di 1 della variabile k cosi' che possa rappresentare l'effetiva lugnhezza della linea (compreso terminatore) */
                    k++; 

                    /* Invio al padre della lunghezza */
                    write(pipe_fp[n][1], &k, sizeof(int)); 

                    /* Attesa dell'indice randomico inviato dal padre */
                    nr = read(pipe_pf[n][0], &ran, sizeof(int)); 

                    /* Conttrollo lettura */
                    if (nr != sizeof(int))
                    { 
                        printf("Errore: il figlio di indice %d ha letto un numero di byte errati dalla pipe di comunicazione col padre\n", n);
                        exit(-1); 
                    }

                    /* Controllo che il valore letto sia un indice valido per la riga corrente: bisogna verificare che ran < k, infatti k ora e' "l'indice" relativo al carattere successivo 
                        all'ultimo della riga corrente, se dunque ran e' strettamente minore di questo valore sara' considerato valido. Si noti poi che se ran = k - 1, il carattere associato
                        e' proprio quello di newline che termina la riga */
                    if (ran < k)
                    { 
                        /* Stampa su file del carattere corrispondente all'indice */
                        write(fcreato, &(linea[ran]), sizeof(char)); 

                        /* Incremento del contatore di caratteri scritti */
                        ritorno++; 
                    }

                    /* Reset dell'indice di riga */
                    k = 0; 
                }
                else 
                {   
                    /* Se no si e' alla fine della riga si procede con la lettura di caratteri incrementando l'indice del buffer linea */
                    k++;
                }
            }

            exit(ritorno);
		}
	}

    /* CODICE DEL PADRE */

    /* Il processo padre: 
        - Si comporta da lettore nei confronti di TUTTI i figli, dunque legge da tutte le pipe di verso figli->padre 
        - Si comporta da scirttore nei confronti di TUTTI i figli, dunque scrive su tutte le pipe di verso padre-> figli 
       Alla luce di cio' e' possibile chiudere tuttu i lati delle pipe non utilizzati */
    for (k = 0; k < N; k++)
    { 
        close(pipe_fp[k][1]);
        close(pipe_pf[k][0]); 
    }

    /* Ciclo di recupero delle informazioni inviate dai figli: prima seguendo l'ordine delle linee, e all'interno di ogni linea seguendo l'ordine dei file.
        Dal momento che la lunghezza in righe dei file passati per parametro e' nota a priori e' possibile creare un for che effettui esattamente H iterazioni 
        una per ogni riga */
    for (k = 1; k <= H; k++)
    {
        /* Genrazione di un numero randomico compreso tra 0 e N - 1 (indici dei figli), per selezionare il figlio di cui considerare la lunghezza spedita */
        ran = mia_random(N); 

        printf("DEBUG-Per la riga %d il padre ha scelto di considerare la lunghezza inviata dal processo figlio di indice %d\n", k, ran); 

        /* Ciclo di recupero delle lunghezze della riga k-esima nei vari file dei figli */
        for (n = 0; n < N; n++)
        { 
            /* Lettura da pipe dalla lunghezza della riga k trovata dal processo di indice n */
            nr = read(pipe_fp[n][0], &lung, sizeof(int)); 

            /* Controllo lettura */
            if (nr != sizeof(int))
            { 
                printf("Errore: lettura della lunghezza derra riga %d del file %s associato al figlio di indice %d non riuscita\n", k, argv[n + 1], n);
                /* exit(8);     Si decide di non terminare il processo, affinche' il padre possa attendere i figli */
            }

            if (n == ran)
            { 
                /* Trovato indice del figlio generato randomicamente, dunque ci salviamo la sua lunghezza in un apposita variabile */
                ran_lung = lung;
            }
        }

        /* Calcolo dell'indice randomico compreso tra 0 e ran_lung - 1 e che verra' passato a tutti i figli */
        ran = mia_random(ran_lung); 

        printf("DEBUG-La lunghezza trovata da questo figlio e' %d, e l'indice randomico generato dal padre e': %d\n", ran_lung, ran);

        /* Ciclo di invio del valore randomico calcolato a tutti i figli */
        for (n = 0; n < N; n++)
        {
            nw = write(pipe_pf[n][1], &ran, sizeof(int)); 

            if (nw != sizeof(int)) 
            { 
                printf("Errore: scrittura dell'indice randomico da parte del padre sulla pipe di indice %d non riuscita\n", n);
                /* exit(9);     Si decide di non terminare il processo, affinche' il padre possa attendere i figli */
            }
        }
    }

	/* Ciclo di attesa dei processi figli con recupero e stampa del valore tornato */
    for (n = 0; n < N; n++)
    { 
        if ((pid = wait(&status)) < 0)
        { 
            printf("Errore: wait di attesa di uno dei figli non riuscita\n");
            exit(10);
        }

        if (WIFEXITED(status))
        {
            ritorno = WEXITSTATUS(status);
            printf("Il processo figlio con pid = %d ha ritornato il valore %d (se 255 e' incorso in qualche problema durante l'esecuzione)\n", pid, ritorno);
        }
        else
        { 
            printf("Il processo figlio con pid = %d e' terminato in modo anomalo\n", pid);
        }
    }
	exit(0); 
}
