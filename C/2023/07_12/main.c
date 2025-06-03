/* Programma C main.c che risolve la parte C della prova di esame */

#include <stdlib.h> 
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/wait.h> 
#include <string.h> 
#include <ctype.h>
#include <signal.h>
#include <limits.h> 
#include <time.h>

#define ERR "[ERROR]"
#define DBG "[DEBUG]"

/* Definizione del TIPO pipe_t che rappresenta un array di due interi, che useremo per la gestione dei lati delle varie pipe create */
typedef int pipe_t[2];

/* Definzione del TIPO tipoS che rappresenta una particolare struttura che viene passata tra i processi figli in pipeline */
typedef struct { 
    int len_max;    /* Lunghezza massima in linee trovata da uno dei processi figli */
    int idx;        /* Indice del processo figio che ha trovato questo massimo */
} tipoS; 

/* Definizone della funzione che permette al processo padre di calcolare un numero randomico compreso tra 1 e n */
int mia_random(int n)
{ 
    int casuale; 
    casuale = rand() % n; 
    casuale++; 
    return casuale; 
}

int main (int argc, char **argv)
{
    /*---------------- Variabili locali -----------------*/
    int pid;            /* Identificatore dei process figli creati con la fork e attesi con la wait */ 
    int N;              /* Numero di file passati per parametro e dunque di processi figli creati */
    int L;              /* Lunghezza massima in linee dei file passati per parametro */
    int n;              /* Indice d'ordine per i processi figli */
    int k;              /* Indice generico per i cicli */
    pipe_t *piped;      /* Array dinamico di pipe che permette l'implementazione di uno schema di comunicazione tra i processi a PIPELINE: 
                            - Il processo figlio P0 comunica con il figlio P1, che a sua volta comunica con P2 e via dicendo fino a PN-1 che comunica col padre 
                            - Il generico processo figlio Pn (con n != 0) dunque legge dalla pipe di indice n - 1 e scrive su quella di indice n 
                            - Il processo figlio di indice n = 0 si limita a scrivere sulla pipe di indice 0, senza leggere da nessuna pipe */
    pipe_t *pipe_pf;    /* Array dinamico di pipe che permette la comunicazione tra padre e singoli figli */
    tipoS strut;        /* Struttura passata tra i figli nella pipeline */
    int fdr;            /* File descriptor associato all'apertura in lettura dei file nei figli */
    char c;             /* Carattere utilizzato dai figli per la lettura da file */
    int file_len;       /* Lunghezza in righe dei vari file calcolata dai vari figli */
    int row;            /* Indice per tenere traccia nei figli della riga corrente */
    char linea[250];    /* Buffer statico di array utilizzato dai processi figli per contenerene via via le righe lette dai rispettivi file. Viene scelta dimensione 
                            del buffer pari a 250 caratteri ipotizzando che le varie righe lette da file abbiano lunghezza in cararatteri minore o uguale a questo valore */
    int r;              /* Valore randomico generato dal processo padre e comunicato ai vari figli */
    int status;         /* Variabile di stato per la wait */
    int ritorno;        /* Valore ritornato dai processi figli */
    int nr, nw;         /* Variabili di controllo per lettura/scrittura da/su pipe */
    /*---------------------------------------------------*/
    
    /* Controllo lasco numero di parametri passati */
    if (argc < 4)
    { 
        printf(ERR" numero di parametri passati a %s errato - Attesi almeno 3 [(program) numpos filename1 filename2 ... ] - Passati %d\n", argv[0], argc - 1); 
        exit(1); 
    }

    /* Calcolo del valore di N come numero di file passati per parametro */
    N = argc - 2; 

    /* Salvataggio di L con controllo che sia un valore strettamente positivo */
    L = atoi(argv[1]); 
    if (L <= 0)
    { 
        printf(ERR" il parametro %s non e' un numero intero strettamente positivo\n", argv[1]); 
        exit(1); 
    }

    /* Inizializzazione del seme per la funzione rand() */
    srand(time(NULL)); 

    /* Si decide di ignorare il SIGPIPE per il padre, in modo tale che quando questo vada a scrivere ai processi figli, qualora 
        uno o piu' di questi fosse termianto, nell'invio il padre non ricevi il segnale, che potrebbe alla sua termianazione. Inoltre 
        l'azione di ignore viene passata anche ai figli, dunque questo permette a tali processi di terminare in modo anomalo nel caso 
        uno dei figli a valle nella comunicazione a pipeline sia terminato */
    signal(SIGPIPE, SIG_IGN); 

    /* Allocazione dinamica della memoria per gli array di pipe: 
        - per l'array di N pipe che permettono lo schema di comunicazione a pipeline; si noti che sono N dal momento che l'ultimo figlio scrive al padre 
        - per l'array di N pipe per la comunicazione padre --> figli  */
    piped = (pipe_t *)malloc(N * sizeof(pipe_t)); 
    pipe_pf = (pipe_t *)malloc(N * sizeof(pipe_t)); 
    if ((piped == NULL) || (pipe_pf == NULL))
    {
        printf(ERR" Allocazione dinamica della memoria per gli array di pipe non riuscita\n");
        exit(2);
    }

    /* Creazione delle pipe */
    for (k = 0; k < N; k++)
    { 
        if (pipe(piped[k]) < 0)
        { 
            printf(ERR" Creazione della pipe di indice %d per la comunicazione e pipeline non riuscita\n", k); 
            exit(3); 
        }

        if (pipe(pipe_pf[k]) < 0)
        { 
            printf(ERR" Creazione della pipe di indice %d per la comunicazione padre-->figli non riuscita\n", k); 
            exit(4); 
        }
    }

    /* Ciclo di creazione dei processi figli */
    for (n = 0; n < N; n++)
    { 
        if ((pid = fork()) < 0)
        { 
            printf(ERR" fork() di creazione del processo figlio di indice %d non riuscita\n", n); 
            exit(5); 
        }

        if (pid == 0)
        { 
            /* CODICE DEI FIGLI */
            
            /* Si decide che in caso i processi figli incorrano in qualche tipo di errore, ritornino il valore -1 (255 senza segno), cosi' che la terminazione dovuta ad un errore, possa
                essere distinta da quella normale. Infatti i processi i figli ritornano in caso normale 0 o 1, valori differenti da 255 */
            
            printf(DBG"-Sono il processo figlio di indice %d e con pid = %d e sto per leggere dal file %s\n", n, getpid(), argv[n + 2]); 

            /* Il generico processo figlio di indice Pn 
                - Per lo schema di comunicazone a pipeline legge dalla pipe di indice n - 1 e scrivere su quella di indice n, 
                  si noti che il proceso P0 non legge da NESSUNA pipe 
                - Per lo schema di comunicazione col padre si limita a leggere dalla pipe di indice n 
                Alla luce di questa considerazione si vanno a chiudere tutte le pipe non utilizzate */
            for (k = 0; k < N; k++)
            { 
                close(pipe_pf[k][1]); 

                if (k != n)
                { 
                    close(piped[k][1]);
                    close(pipe_pf[k][0]); 
                } 

                if ((k != n - 1) || (n == 0))
                {
                    close(piped[k][0]); 
                }
            }

            /* Apertura del file assciato in lettura */
            if ((fdr = open(argv[n + 2], O_RDONLY)) < 0)
            { 
                printf(ERR" Apertura del file %s da parte del figlio di indice %d non riuscita - file non leggibile oppure inesistente\n", argv[n + 2], n); 
                exit(-1); 
            }
            
            /* Inizializzazione a 0 del contatore di numero di righe, che alla fine del conteggio risultera' essere la lunghezza in righe del file */
            file_len = 0; 

            /* Ciclo di lettura da file per effettuare il conteggio di numero di righe del file */
            while (read(fdr, &c, 1) > 0)
            { 
                if (c == '\n')
                {
                    file_len++;
                }
            }

            /* Riportato il file pointer all'inizio del file per la lettura successiva */
            lseek(fdr, 0L, SEEK_SET); 

            /* Lettura della struttura inviata dal figlio Pn. Si noti che questa azione viene effettuata solo dai processi figli con indice n != 0 */
            if (n != 0)
            { 
                nr = read(piped[n - 1][0], &strut, sizeof(strut)); 

                /* Verifica che la lettura abbia avuto successo */
                if (nr != sizeof(strut))
                { 
                    printf(ERR" Il processo figlio di indice %d ha letto un numero errato di byte dalla pipeline: %d\n", n, nr); 
                    exit(-1); 
                }
            
                /* Controllo se il processo Pn ha trovato il numero massimo di righe nel file associato */
                if (file_len > strut.len_max)
                { 
                    /* Aggiornamento dei campi della strttutura */
                    strut.len_max = file_len; 
                    strut.idx = n; 
                }
            }
            else 
            { 
                /* Caso in cui ci si trova nel processo P0, dunque si inizializza la struttura con i suoi parametri */
                strut.len_max = file_len; 
                strut.idx = n; 
            }
            
            /* Invio della struttura al processo successivo */
            nw = write(piped[n][1], &strut, sizeof(strut)); 

            /* Verifica scrittura su pipeline */
            if (nw != sizeof(strut))
            {
                printf(ERR" Il processo figlio di indice %d ha scritto un numero errato di byte sulla pipeline: %d\n", n, nw); 
                exit(-1); 
            }

            /* Recupero del numero di righe inviato dal padre */
            nr = read(pipe_pf[n][0], &r, sizeof(int)); 

            if (nr != sizeof(int))
            { 
                printf(ERR" Il processo figlio di indice %d ha letto un numero errato dalla pipe di comunicazione col padre: %d\n", n, nr);
                exit(-1);  
            }

            /* Controllo che il valore inviato dal padre sia valido per il figlio corrente */
            if (r <= file_len)
            { 
                /* Il valore inviato dal padre corrisponde ad una linea esistente nel file dunque si va a recuperare questa linea. Si va anche 
                    ad inizializzare il valore di ritorno a 0 come da specifica */
                ritorno = 0; 

                /* Inizializzazione a 0 dell'indice per la scrittura nel buffer e del contatore di righe */
                k = 0; 
                row = 0; 
                while (read(fdr, &(linea[k]), 1) > 0)
                {
                    if (linea[k] == '\n')
                    { 
                        row++; 

                        if (row == r)
                        {   
                            /* Trasformazione della linea in stringa per facilitarne la stampa */
                            linea[k] = '\0'; 

                            /* Si e' trovata la linea che corrisponde all'indice inviato dal padre, dunque si effettua la stampa richiesta dal testo.
                                Si noti che questa non e' esattamente la stampa riportata dal testo, ma una stampa che ritengo piu' utile */
                            printf("Il processo figlio di indice %d e con pid = %d ha trovato correttamente la linea di indice %d (valore prodotto dal padre) "
                                     "dal momento che il suo file associato %s conta una lunghezza totale in righe pari a %d\n", n, getpid(), r, argv[n + 2], file_len); 
                            printf("linea letta: '%s'\n", linea);

                            /* A questo punto e' possibile terminare la lettura da file */
                            break; 
                        }

                        /* Reset dell'indice per il buffer */
                        k = 0; 
                    }
                    else
                    { 
                        /* Se non si ha trovato una linea ci si limita ad aggiornare l'indice per il buffer */
                        k++; 
                    }
                }
            }
            else
            { 
                /* Si imposta il ritorno a 1 come da specifica */
                ritorno = 1; 

                /* Stampa richiesta dal testo */
                printf("Il figlio di indice %d e con pid %d non ha potuto trovare la linea di indice %d (valore prodotto dal padre) dal momento che il file %s associato "
                        "conta solo %d righe\n", n, getpid(), r, argv[n + 2], file_len); 
            }

            exit(ritorno); 
        }
    }

    /* CODICE DEL PADRE */

    /* Il processo padre: 
        - E' l'ultimo processo lettore per lo schema di comunicazione a pipeline e dunque legge dalla pipe di indice N - 1 
        - Si comporta da processo produttore per tutti i figli per comunicare il valore generato randomicamente 
        Alla luce di queste considerazioni e' possibile chiudere tutte le pipe non utilizzate dal padre */
    for (k = 0; k < N; k++)
    {
        close(pipe_pf[k][0]); 
        close(piped[k][1]); 

        if (k != N - 1)
        {
            close(piped[k][0]); 
        }
    }

    /* Recupero della struttura da pipeline */
    nr = read(piped[N - 1][0], &strut, sizeof(strut)); 

    /* Controllo lettura */
    if (nr != sizeof(strut))
    { 
        printf("Il processo padre ha letto un numero errato di byte da pipeline\n"); 
        /* exit(6)      Si decide di non terminare il padre affinche' possa attendere i figli */
    }

    /* Controllo che la lunghezza massima trovata dai processi figli sia pari a L come richiesto dal testo */
    if (strut.len_max == L)
    { 
        /* Generazione del valore randomico compreso tra 1 e L tramite la funzione mia_random() */
        r = mia_random(L); 

        printf(DBG" Il padre ha generato il numero %d\n", r); 

        /* Ciclo di invio del valore randomico ai vari figli */
        for (n = 0; n < N; n++)
        { 
            write(pipe_pf[n][1], &r, sizeof(int)); 
        }  
    }
    else 
    { 
        printf(DBG" La lunghezza massima in linee trovata dai filgli (%d) e' diversa da L = %d\n", strut.len_max, L); 

        /* Sblocco dei figli con l'invio di un valore grade */
        r = INT_MAX;
        
        /* Ciclo di invio del valore sicuramente superiore a al numero da loro calcolato come lunghezza per sbloccarli dalla lettura 
            ma evitare che stampino qualche tipo di riga */
        for (n = 0; n < N; n++)
        { 
            write(pipe_pf[n][1], &r, sizeof(int)); 
        }
    }
    

    /* Ciclo di attesa dei processi figli con recupero e stampa del valore tornato */
    for (n = 0; n < N; n++)
    { 
        if ((pid = wait(&status)) < 0)
        { 
            printf("Errore: wait() di attesa di uno dei processi figli non riuscita\n"); 
            exit(7);  
        }

        if ((status & 0xFF) != 0)
        { 
            printf("Il processo figlio con pid = %d e' terminato in modo anomalo\n", pid); 
        }
        else 
        { 
            ritorno = (int)((status >> 8) & 0xFF);
            
            switch (ritorno)
            {
                case 0:
                { 
                    printf("Il processo figlio con pid = %d ha ritornato il valore %d, e' dunque riuscito a leggere correttamente la linea da file\n", pid, ritorno); 
                    break;
                }
                
                case 1: 
                { 
                    printf("Il processo figlio con pid = %d ha ritornato il valore %d, NON e' dunque riuscito a leggere la linea dal proprio file\n", pid, ritorno); 
                    break;
                }

                default:
                { 
                    printf("Il processo figlio con pid = %d ha ritornato il valore %d, e' dunque incorso in qualche errore durante l'esecuzione\n", pid, ritorno); 
                }
            }
        }
    }
    exit(0); 
}
