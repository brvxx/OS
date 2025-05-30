/* Programma C main.c che risolve la parte C della prova di esame totale tenutasi il 10 luglio 2024 */

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

/* Definizione di una funzione che calcoli un valore randomico, compreso tra 1 e n */
int mia_random(int n)
{ 
    int casuale; 
    casuale = rand() % n; 
    casuale++; 
    return casuale;
}

/* Definizone de tipo tipoL che rappresenta un array di 250 caratteri, usato come buffer di memoria per le singole linee lette da file. 
    Si e' scelto il valore 250 dal momento che il testo specifica che le linee abbiano lunghezza minore o uguale a 250 caratteri (compreso il terminatore) */
typedef char tipoL[250];

int main (int argc, char **argv)
{
    /*---------------- Variabili locali -----------------*/
    int pid; 			/* identificatore dei processi figli creati con la fork e attesi con la wait */
    int N; 				/* numero di file passati per parametro */
    int L; 				/* lunghezza in linee del file associati ai singoli processi */
    int n; 				/* indice dei processi figli */
    int k; 				/* indice generico per i cicli */
    pipe_t *piped;		/* array dinamico di pipe che viene utilizzato per creare uno schema di comunicazione a PIPELINE: 
                            - il primo processo P0 comunica con P1, che a sua volta comunica con P2, etc... fino a PN-1 che comunica col processo padre 
                            - il generico processo figlio Pn (con n != 0) dunque legge dalla pipe di indice n - 1 e scrive su quella di indice n 
                            - il processo figlio P0 essendo il primo non legge da nessuna pipe, si limita a scrivere sulla pipe di indice 0 dando il via alla comunicazione */
    tipoL linea; 		/* buffer di 250 caratteri atto alla lettura delle singole linee da parte dei figli */
    tipoL *tutteLinee; 	/* array dinamico di array di caratteri che viene mandato avanti nella pipeline e viene aggiornato da ogni processo 
                            figlio con la prorpia linea da cercare */
    int fdr; 			/* file descriptor associato all'apertura dei file associati ai vari figli */
    int r; 				/* valore randomico generato da ogni figlio che corrisponde all'indice della linea da cercare */
    int rows; 			/* variabile contatore delle linee lette da file nei vari processi figli */
    int status; 		/* variabile di stato per la wait */
    int ritorno; 		/* valore ritornato dai processi figli */
    int nr, nw; 		/* variabili di controllo per lettura/scrittura da/su pipe */
    /*---------------------------------------------------*/

    /* Controllo lasco numero di parametri e controllo che questo valore sia pari */
    if ((argc < 5) || ((argc - 1) % 2 == 1))
    { 
        printf("Errore: numero di parametri passato a %s errato - Attesi almeno 4 parametri, di cui due file, seguiti dalla rispettiva lunghezza in linee "
                "[(program) filename1 len1 filename2 len2 ... ] - Passati: %d\n", argv[0], argc - 1); 
        exit(1);
    }

    /* Calcolo di N come numero di soli file passati */
    N = (argc - 1)/2; 

    /* Controllo che le varie lunghezze passate siano dei numeri strettamente positivi. Dal momento che la specifica richiede che venga controllato che questi parametri siano 
        solo strettamente positivi, si va a dar per scontato che rappresent dei numeri interi. Per effettuare il controllo bastera' verificare che il primo carattere non sia un '-' 
        (non negativo) oppure uno '0' (non nullo) */
    for (k = 2; k < argc; k += 2)
    { 
        if ((argv[k][0] == '0') || (argv[k][0] == '-'))
        { 
            printf("Errore: il parametro %s passato non rappresenta un numero intero strettamente positivo\n", argv[k]); 
            exit(2); 
        }
    }

    /* Allocazione dinamica della memoria per l'array di linee che verra' mandato avanti nella pipeline dai figli */
    tutteLinee = (tipoL *)malloc(N * sizeof(tipoL)); 
    if (tutteLinee == NULL)
    { 
        printf("Errore: allocazione dinamica della memoria per l'array di linee non riuscita\n"); 
        exit(4); 
    }

    /* Allocazione dinamica della memoria per l'array di pipe. Dal momento che dobbiamo collegare tra di loro a pipeline gli N figli ma ANCHE IL PADRE, necessariamente necessitiamo 
        di N pipe */
    piped = (pipe_t *)malloc(N * sizeof(pipe_t)); 
    if (piped == NULL)
    { 
        printf("Errore: allocazione dinamica della memoria per l'array di pipe non riuscita\n"); 
        exit(3);
    }

    /* Ciclo di creazione delle pipe */
    for (k = 0; k < N; k++)
    { 
        if (pipe(piped[k]) < 0)
        { 
            printf("Errore: crezione della pipe di indice %d della pipeline non riuscita\n", k);
            exit(5); 
        }
    }

    /* Ciclo di creazione dei figli */
    for (n = 0; n < N; n++) 
    { 
        if ((pid = fork()) < 0)
        { 
            printf("Errore: fork di creazione del processo figlio di indice %d non riuscita\n", n);
            exit(6);  
        }

        if (pid == 0)
        { 
            /* CODICE DEI FIGLI */

            /* Si decide che nel caso in cui i processi figli incorrano in qualche tipo di errore ritornino il valore -1 (255 senza segno), in modo tale che questa terminazione possa 
                essere distinta dal caso normale, in cui viene tornato r, che al massimo sara' pari alla lunghezza in linee del file associato al singolo processo figlio, ma che per 
                specifica ha lunghezza in linee STRETTAMENTE MINORE di 255 */

            printf("DEBUG-Sono il processo figlio di indice %d e con pid = %d e sto per leggere dal file %s con lunghezza in "
                    "righe pari a %s\n", n, getpid(), argv[2*n + 1], argv[2*n + 2]); 

            /* Il processo figlio Pn, con n != 0, legge dalla pipe di indice n - 1 e scrive su quella di indice n, alla luce di cio' e' quindi possibile andare a chiudere tutti i lati delle pipe  
                che non vengono utilizzate dal singolo processo. Mentre il processo di indice 0 non legge da nessuna pipe, puo' quindi chiudere direttamente TUTTI i lati di lettura */
            for (k = 0; k < N; k++)
            { 
                if (k != n)
                { 
                    close(piped[k][1]);
                }

                if ((k != n - 1) || (n == 0)) 
                { 
                    close(piped[k][0]); 
                }
            }

            /* Apertura del file associato */
            if ((fdr = open(argv[2*n + 1], O_RDONLY)) < 0)
            { 
                printf("Errore: Apertura del file %s da parte del processo figlio di indice %d non riuscita - file non leggibile oppure inesistente\n", argv[2*n + 1], n); 
                exit(-1); 
            }

            /* Come da specifica ogni processo figlio prima di fare qualsiasi cosa inizializza il seme per la funzione randomizzatrice di numeri rand() */
            srand(time(NULL)); 

            /* Salvataggio della lunghezza associata al file del processo corrente dentro la variabile L */
            L = atoi(argv[n*2 + 2]);

            /* Generazione del valore random della linea r mediante la funzione mia_random */
            r = mia_random(L); 

            printf("DEBUG-Il processo figlio di indice %d ha generato come indice di riga randomico il valore %d\n", n, r); 

            /* Inizializzazione a 0 del contatore di riga e della variabile k utilizzata come indice per la scrittura dei caratteri letti dentro linea. Si noti che come richiesto da 
                specifica, la numerazione delle linee partira' da 1 e non 0 */
            rows = 0; 
            k = 0; 

            /* Ciclo di lettura del file carattere per carattere con salvataggio delle linee */
            while (read(fdr, &(linea[k]), sizeof(char)) > 0)
            { 
                if (linea[k] == '\n')
                { 
                    /* Trovata una linea, dunque si incrementa il contatore */
                    rows++; 
                    
                    if (rows == r)
                    { 
                        /* Trovata linea che corrisponde all'indice random calcolato, dunque se e solo se n != da 0 viene effettuata la lettura dell'array passato 
                            dal processo precedente */
                        if (n != 0)
                        {   
                            /* Lettura di TUTTO l'array */
                            nr = read(piped[n - 1][0], tutteLinee, N * sizeof(tipoL));

                            /* Controllo della lettura, fondamentale per uno schema di comunicazione a pipeline, infatti se un qualche processo fallisce, la pipeline "si rompe" */
                            if (nr != N * sizeof(tipoL))
                            { 
                                printf("Errore: il figlio di indice %d ha letto un numero errato di byte dalla pipeline: %d\n", n, nr); 
                                exit(-1);
                            }
                        }

                        /* A prescindere dall'indice del processo, si va a copiare la memoria contenuta nel buffer linea, dentro all'array tutteLinee, nella rispettiva posizione 
                            associata al singolo processo figlio. Si noti che nel caso di n == 0 l'array viene inizializzato per la prima volta. */

                        /* Per effettuare la copia della memoria si usa la funzione memcpy passando come numero di byte da copiare l'intera lunghezza della riga, compreso il terminatore, 
                            grazie all'indice k, che una volta incrementato di 1 rappresenta questo valore di lunghezza */
                        memcpy(tutteLinee[n], linea, k + 1); 

                        /* Invio dell'array tutteLinee al processo successivo */
                        nw = write(piped[n][1], tutteLinee, N * sizeof(tipoL)); 

                        /* Controllo numero di byte scritti */
                        if (nw != N * sizeof(tipoL))
                        {
                            printf("Errore: il figlio di indice %d ha scritto un numero errato di byte nella pipeline: %d\n", n, nw); 
                            exit(-1);
                        }

                        /* A questo punto e' possibile terminare uscire dal ciclo di lettura, in quanto le prossime letture sarebbero superflue */
                        break; 
                    }

                    /* Reset dell'indice per la linea successiva*/
                    k = 0; 
                }
                else 
                { 
                    /* Se non si e' giunti alla fine di una riga ci si limita ad incrementare l'indice */
                    k++; 
                }
            }

            /* Come da specifica ogni processo figlio ritorna al padre il valore randomico generato */
            exit(r); 
        }
    }

    /* CODICE DEL PADRE */

    /* IL processo padre fa parte dello schema di comunicazione e si comporta da consumatore nei confronti di PN-1, andando appunto a leggere dalla pipe di indice N - 1, dunque 
        tutte le altre pipe possono essere chiuse */
    for (k = 0; k < N; k++)
    { 
        close(piped[k][1]); 
        if (k != N - 1)
        {
            close(piped[k][0]); 
        }
    }

    /* Lettura dell'array risultante dalla pipeline */
    nr = read(piped[N - 1][0], tutteLinee, N * sizeof(tipoL)); 

    /* Controllo sulla lettura */
    if (nr != N * sizeof(tipoL))
    {
        printf("Errore: il processo padre ha letto dalla pipeline un numero errato di byte: %d\n", nr); 
        /*exit(7)   Si decide comunque di non terminare il processo padre, affimche' questo possa attendere tutti i figli */
    }
    else 
    { 
        /* Stampa richiesta dal testo */
        printf("Il padre ha ricevuto le seguenti linee dai figli:\n"); 
       
        /* Ciclo di lettura delle singole linee. Si noti che dal momento che si tratta di linee (\n terminate) e NON di stringhe, non e' possibile andare a
            stampare a video le righe con una printf, ma sara' necessario un ulteriore ciclo di lettura dei caratteri */
        for (n = 0; n < N; n++) 
        { 
            /* Stampa richiesta dal testo */
            printf("Linea ricevuta dal figlio di indice %d: \n", n); 

            /* Ciclo di stampa dei caratteri della linea riporata dal figlio di indice n */
            for (k = 0; k < 250; k++) 
            { 
                /* Scrittura del k-esimo carttere su stdout */
                write(1, &(tutteLinee[n][k]), sizeof(char)); 

                /* Controllo se ci si trovi alla fine della linea */
                if (tutteLinee[n][k] == '\n')
                { 
                    /* Se si incontra nella scrittura un \n allora la linea e' terminata e si DEVE uscire dal ciclo di lettura, infatti il restante 
                        contenuto del buffer sono byte non inizializzati, dunque spazzatura */
                    break; 
                }
            }
        }
    
    }

    /* Ciclo di attesa dei processi figli con recupero e stampa del valore tornato */
    for (k = 0; k < N; k++)
    { 
        if ((pid = wait(&status)) < 0)
        { 
            printf("Errore: wait di uno dei figli non riuscita\n"); 
            exit(8); 
        }

        if (WIFEXITED(status))
        { 
            ritorno = WEXITSTATUS(status); 
            printf("Il processo figlio con pid = %d ha ritornato il valore %d (se 255 allora e' incorso in qualche problema durante l'esecuzione)\n", pid, ritorno); 
        }
        else 
        { 
            ritorno = WTERMSIG(status); 
            printf("Il processo figlio con pid = %d e' terminato in modo anomalo a causa del segnale no. %d\n", pid, ritorno);
        }
    }

    exit(0); 
}
