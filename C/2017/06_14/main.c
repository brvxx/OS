/* Programma C main.c che risolve la parte C della prova di esame totale tenutasi il 14 giugno 2017 */

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
	int N; 				/* numero di file passati per parametro e dunque di figli creati */
	char Cx; 			/* carattere da cercare in ongi processo figlio, passato per parametro */
	int n; 				/* indice per i figli */
	int fdr;			/* file descriptor associato all'apertura in lettura dei file da parte dei figli */
	char c; 			/* carattere usato per la lettura dei singoli file */
	char ctrl; 			/* carattere inviato dal padre ai figli da sostituire al posto dell'occorrenza trovata di Cx */
	long int pos_curr; 	/* posizione corrente nel file */
	int sost; 			/* numero totale di sostituzioni effettuate dal figlio */
	pipe_t *pipe_fp;	/* array dinamico di pipe per la comunicazione nel verso figli-padre */
	pipe_t *pipe_pf; 	/* array dinamico di pipe per la comunicazione nel verso padre-figli */
    int finito;         /* variabile di controllo per il padre, se valore 0 allora esiste ancora un qualche processo figlio non temrinato, se vale 1 
                            allora tutti i processi figli sono terminati */
	int k; 				/* indice generico per i cicli */
	int status; 		/* variabile di stato per la wait */
	int ritorno; 		/* valore ritornato dai processi figli */
	int nr, nw; 		/* variabili di controllo per lettura/scrittura da/su pipe */
	/*---------------------------------------------------*/
	
	/* Controllo lasco numero di parametri passati */
	if (argc < 3)
	{ 
		printf("Errore: numero di parametri passati errato - Attesi almeno 2 [filename1 ... Cx] - Passati: %d\n", argc - 1);
		exit(1);
	}

	/* Calcolo del valore di N come numero totale di file passati */
	N = argc - 2; 

	/* Controllo ultimo parametro */
	if (strlen(argv[argc - 1]) != 1)
	{ 
		printf("Errore: il parametro %s non e' un singolo carattere\n", argv[argc - 1]); 
		exit(2);
	} 

	/* Salvataggio del carattere passato nella variabile Cx */
	Cx = argv[argc - 1][0]; 

	/* Allocazione dinamica della memoria per gli array di pipe per la comunicazione in entrambi i versi */
	pipe_pf = (pipe_t *)malloc(N * sizeof(pipe_t)); 
	pipe_fp = (pipe_t *)malloc(N * sizeof(pipe_t)); 	

	if ((pipe_fp == NULL) || (pipe_pf == NULL))
	{ 
		printf("Errore: allocazione della memoria per uno degli array di pipe non riuscita\n"); 
		exit(3);
	}

	/* Ciclo di creazione delle pipe */
	for (k = 0; k < N; k++)
	{ 
		if (pipe(pipe_pf[k]) < 0)
		{
			printf("Errore: creazione della pipe di comunicazione padre-figlio di indice %d non riuscita\n");
			exit(4);
		}

		if (pipe(pipe_fp[k]) < 0)
		{
			printf("Errore: creazione della pipe di comunicazione figlio-padre di indice %d non riuscita\n");
			exit(5);
		}
	}

	/* Ciclo di creazione dei processi figli */
	for (n = 0; n < N; n++)
	{ 
		if ((pid = fork()) < 0)
		{
			printf("Errore: fork di creazione del figlio di indice %d non riuscita\n");
			exit(6);
		}

		if (pid == 0)
		{ 
			/* CODICE DEI FIGLI */

			/* Si decide che nel caso in cui un processo figlio incorra in qualche tipo di errore, questo ritorni il valore -1 (255 senza sengo), dal momento che il normale 
				valore di ritorno corrisponde al numero totale di sostituzioni, che per specifica sono strettamente minori di 255, dunque sara' possibile distinguere i due casi */

			printf("DEBUG-Sono il processo figlio di indice %d e con pid = %d e sto per cercare le occorrenze del carattere '%c' nel file %s\n", n, getpid(), Cx, argv[n + 1]);

			/* Ogni processo figlio Pn: 
				- Si comporta da lettore per la pipe di indice n di comunicazione padre-figli 
				- Si comporta da scrittore per la pipe di indice n di comunicazione figli-padre 
			   Dunque tutti i lati non usati possono essere chiusi */
            
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

            /* Apertura del file associato al processo sia in lettura che scrittura */
            if ((fdr = open(argv[n + 1], O_RDWR)) < 0)
            { 
                printf("Errore: apertura del file %s da parte del figlio di indice %d non riuscita - file non esistente oppure non legibile/scrivibile\n", argv[n + 1], n);
                exit(-1);
            }

            /* Ciclo di lettura del file carattere per carattere */

            /* Inizializzazione a 0 del numero di sostituzioni effettuate */
            sost = 0;
            while (read(fdr, &c, 1) > 0)
            { 
                if (c == Cx)
                { 
                    /* Trovata un occorrenza di Cx nel figlio Pn */

                    /* Calcolo della posizione da inviare al processo padre. Dal momento che la lettura sposta il file pointer al carattere successivo, possiamo decidere 
                        di far partire la numerazione dei caratteri da 1, in modo tale che il valore tornato dalla lseek (ossia la posizione del file pointer) rappresenti 
                        la posizione dell'occorenza ma per una numerazione che parte da 1 e NON DA 0 */
                    pos_curr = lseek(fdr, 0L, SEEK_CUR); 

                    /* Invio al padre della posizione corrente dell'occorrenza trovata */
                    write(pipe_fp[n][1], &pos_curr, sizeof(long int));

                    /* Attesa del carattere */
                    nr = read(pipe_pf[n][0], &ctrl, sizeof(char)); 
                    
                    /* Controllo sulla lettura */
                    if (nr != 1)
                    { 
                        printf("Errore: Lettura del carattere da sostituire nel figlio di indice %d non riuscita\n", n);
                        exit(-1);
                    }

                    /* Si decide che nel caso in cui l'utente non specifici nessun carattere e dunque prema direttamente "invio", si vada direttamente ad utilizzare il '\n'
                        immesso dall'utente per riconoscere questo caso e NON andare ad effettuare la sostituzione dell'occorrenza trovata */
                    if (ctrl != '\n')
                    { 
                        /* Carattere inviato dal padre valido, dunque si incrementa il contatore di sostituzioni */
                        sost++; 

                        /* Si porta indietro di 1 posizione il file pointer in modo da effetuare la sostituzione su file */
                        lseek(fdr, -1L, SEEK_CUR);

                        /* Scrittura del carattere inviato dal padre */
                        nw = write(fdr, &ctrl, sizeof(char)); 

                        /* Controllo sulla scrittura */
                        if (nw != sizeof(char))
                        { 
                            printf("Errore: il processo figlio con indice %d non e' riuscito a scrivere il carattere '%c' sul file %s\n", n, ctrl, argv[n + 1]);
                            exit(-1); 
                        }
                    }

                    /* Nel caso sia passato il carattere newline non viene effettuata alcuna sostituzione */
                }
            }

            exit(sost);
		}
	}

    /* CODICE DEL PADRE */

    /* Il processo padre: 
        - Legge le posizioni inviate dai figli da TUTTE le pipe 
        - Scrive i caratteri immessi da parte dell'utente su TUTTE le pipe
       Dunque tutte le altre pipe non usate pssono essere chiuse */

    /* Ciclo di chiusura dei lati delle pipe non utilizzati */
    for (k = 0; k < N; k++)
    { 
        close(pipe_fp[k][1]); 
        close(pipe_pf[k][0]);
    }

    /* Ciclo di recupero delle informazioni inviate dai figli secondo l'ordine dei file . Questo termina qual'ora tutti i processi figli sono terminati, per determinare la cosa 
        si fa utilizzo della varaibile finito. Questa viene ad ogni iterazione messa a 1 ipotizzando dunque che tutti i processi siano terminati, poi nel caso che il padre riesca 
        a leggere qualcosa dalla pipe di un figlio, la variabile viene rimessa a 0 in modo tale che il ciclo possa continuare */
    
    finito = 0;     /* Inizializzazione a 0 per entrare nel ciclo */
    while (!finito)
    { 
        finito = 1;
        
        /* Tentativo di lettura della singola posizione per OGNI figlio */
        for (n = 0; n < N; n++)
        {
            nr = read(pipe_fp[n][0], &pos_curr, sizeof(long int)); 

            if (nr != 0)
            { 
                /* Trovato un processo figlio non terminato, si porta dunque la variabile finito a 0 */
                finito = 0; 

                /* Stampa richiesta dal testo */
                printf("Il processo figlio di indice %d ha trovato un occorrenza del carattere '%c' nel file %s alla posizione: %ld\n", n, Cx, argv[n + 1], pos_curr); 

                /* Richiesta all'utente di immissione di un carattere da sostituire */
                printf("Immettere 1 solo carattere da sostituire (seguito dall'invio); Se non si voglia effettuare la sostituzione premere solo INVIO:\n"); 

                /* lettura del carattere immesso */
                scanf("%c", &ctrl); 

                /* Dal momento che la scrittura su stdin e' bufferizzata, e qui si e' andati a leggere 1 SOLO CARATTERE di quelli immessi, nel caso in cui sia stato 
                    immesso il carattere da sostuire + newline, nel buffer rimarrebbe il carattere \n, letto nella successiva scanf. Bisogna dunque andare ad effettuare
                    una lettura "a vuoto" */
            }


        }
    }    


	/* Ciclo di attesa dei processi figli con recupero e stampa del valore tornato */
    for (n = 0; n < N; n++)
    {
        if ((pid = wait(&status)) < 0)
        { 
            printf("Errore: wait di attesa di uno dei figli non riuscita\n");
            exit(7);
        }

        if (WIFEXITED(status))
        { 
            ritorno = WEXITSTATUS(status);
            printf("Il processo figlio con pid = %d ha ritornato il valore %d (se 255 e' incorso in un qualche errore durante la sua esecuzione)\n", pid, ritorno);
        }
        else 
        { 
            printf("Il processo figlio con pid = %d e' terminato in modo anomalo");
        }
    }
	exit(0); 
}
