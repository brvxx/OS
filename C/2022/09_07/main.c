/* Programma C main.c che risolve la parte C della prova di esame totale tenutasi il 7 settembre 2022 */

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
    int pid;            /* Identificatore dei process figli creati con la fork e attesi con la wait */ 
    int N;              /* Numero di file passati per parametro e dunque di processi figli creati */
    int n;              /* Indice d'ordine per i processi figli */
    int k;              /* Indice generico per i cicli */
    pipe_t *piped;      /* Array dinamico di pipe che permettono l'implementazione di uno schema a comunicazione a pipeline tra i processi in gioco, dove: 
                            - Il processo P0 comunica con P1, che a sua volta comunica con il processo P2 e via dicendo, fino ad arrivare al processo PN-1 che comunica col padre 
                            - Il generico processo Pn dunque legge dalla pipe di indice n - 1 e scrive su quella di indice n
                            - Il processo P0 si comporta in modo diverso, infatti essendo il primo processo NON legge da nessuna pipe */
    int fcreato;        /* File descriptor associato alla creazione del file nel processo padre */
    int fdr;            /* File descriptor associato all'apertura in lettura dei file nei processi figli */
    char car;           /* Carattaere letto via via nei vari figli */
    int pos;            /* Variabile utilizzata dai figli per tenere traccia della posizione dei caratteri letti nel file. Si noti che la specifica richiede che 
                            la posizione del primo carattere sia la posizione 1 E NON 0 */
    char *cur;          /* Array dinamico di char mandato avanti nella pipeline tra i vari processi figli per la comunicazione */
    int status;         /* Variabile di stato per la wait */
    int ritorno;        /* Valore ritornato dai processi figli */
    int nr, nw;         /* Variabili di controllo per lettura/scrittura da/su pipe */
    /*---------------------------------------------------*/
    
    /* Controllo lasco numero di parametri */
    if (argc < 3)
    { 
        printf("Errore: Numero di parametri passato a %s errato - Attesi almeno 2 [(program) filename1 filename2 ... ] - Passati: %d\n", argv[0], argc - 1); 
        exit(1); 
    }

    /* Calcolo del valore di N come numero di file passati come parametri */
    N = argc - 1; 

    /* Come richiesto nel testo il processo padre crea subito il file che prende il nome del proprio COGNOME */
    if ((fcreato = creat("OGGEA", PERM)) < 0)
    { 
        printf("Errore: Creazione del file di nome OGGEA da parte del processo padre non riuscita\n"); 
        exit(2); 
    }

    /* Allocazione dinamica della memoria per l'array di pipe. Si noti che dal momento che il processo padre e' incluso nella comunicazione, per implementare la comunicazione a 
        pipeline saranno necessarie N pipe */
    piped = (pipe_t *)malloc(N * sizeof(pipe_t)); 
    if (piped == NULL)
    { 
        printf("Errore: Allocazione dinamica della memoria per l'array di pipe non riuscita\n"); 
        exit(3); 
    }

    /* Ciclo di creazione delle pipe */
    for (k = 0; k < N; k++)
    { 
        if (pipe(piped[k]) < 0)
        {
            printf("Errore: Creazione della pipe di indice %d non riuscita\n", k);
            exit(4);
        }
    }

    /* Allocazione dinamica della memoria per l'array da passare tra i vari processi figli in pipeline */
    cur = (char *)malloc(N * sizeof(char)); 
    if (cur == NULL)
    { 
        printf("Errore: Allocazione dinamica della memoria per l'array cur non riuscita\n"); 
        exit(5); 
    }

    /* Ciclo di creazione dei processi figli */
    for (n = 0; n < N; n++)
    { 
        if ((pid = fork()) < 0)
        { 
            printf("Errore: fork() di creazione del processo figlio di indice %d non riuscita\n", n); 
            exit(6); 
        }

        if (pid == 0)
        { 
            /* CODICE DEI FIGLI */

            /* Si decide che in caso i processi figli incorrano in qualche tipo di errore, ritornino il valore -1 (255 senza segno), cosi' che la terminazione dovuta ad un errore, possa
                essere distinta da quella normale. Infatti nel caso normale i processi figli ritornano al padre l'ultimo carattere letto da file, che sicuramente avra' valore minore di 255 */
            printf("DEBUG-Sono il processo figlio di indice %d e con pid = %d e sto per effettuare la lettura del file %s\n", n, getpid(), argv[n + 1]); 

            /* Si decide nei processi figli di andare ad ignorare il SIGPIPE, questo per prevenire che uno dei processi possa terminare in modo anomalo, 
                infatti qualora il processo successivo a Pn avesse a lui associato un file NON leggibile, questo terminerebbe. Ma allora il processo Pn andrebbe 
                a terminare in modo anomalo tendando di passare in avanti l'array cur */
            signal(SIGPIPE, SIG_IGN);

            /* Il generico processo figlio Pn, con n != 0 legge dalla pipe di indice n - 1 e scrive su quella di indice n; dunque tutti i lati delle altre pipe non utilizzate possonoe essere 
                chiusi. Si noti poi che il processo figlio di indice n = 0 non legge da nessuna pipe, e dunque puo' chiudere tutti i lati di lettura delle pipe */
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

            /* Apertura del file associato al figlio */
            if ((fdr = open(argv[n + 1], O_RDONLY)) < 0)
            { 
                printf("Errore: Apertura in lettura del file %s da parte del figlio di indice %d non riuscita - File non leggibile oppure inesistente\n", argv[n + 1], n); 
                exit(-1); 
            }

            /* Ciclo di lettura da file carattere per carattere */
            pos = 0;        /* Inizializzazione della variabile di posizione nel file */
            while (read(fdr, &car, 1) > 0)
            { 
                pos++;      /* Posizioni partono da 1 */

                if (pos % 2 == 1)
                { 
                    /* Trovato carattere in posizione dispari, dunque se n != 0 allora il processo Pn si mette in attesa di ricevere l'array cur passato dal processo 
                        precedente per poterlo aggiornare e passarlo nuovamente avanti nella pipeline */
                    if (n != 0)
                    { 
                        nr = read(piped[n - 1][0], cur, N * sizeof(char)); 

                        /* Controllo lettura da pipeline */
                        if (nr != N * sizeof(char))
                        { 
                            printf("Errore: Il processo figlio di indice %d ha letto un numero errato di byte da pipeline: %d\n", n, nr); 
                            exit(-1); 
                        }
                    }

                    /* Aggiornamento dell'array col carattere dispari appena letto */
                    cur[n] = car; 

                    /* Invio dell'array al processo successivo */
                    nw = write(piped[n][1], cur, N * sizeof(char)); 

                    /* Controllo della scrittura su pipe */
                    if (nw != N * sizeof(char))
                    { 
                        printf("Errore: Il processo figlio di indice %d ha scritto un numero errato di byte da pipeline: %d\n", n, nw); 
                        exit(-1); 
                    }
                }
            }

            /* Come da specifica in caso di successo il processo figlio ritorna al padre l'ultimo carattere letto */
            exit(car); 

        }
    }

    /* CODICE DEL PADRE */

    /* Il processo padre partecipa nello schema di comunicazione a pipeline ma solo in qualita' di lettore dei vari array finali prodotti dalla catena dei figli; infatti, il processo padre 
        si limita a leggere dalla pipe di indice N - 1, dunque tutti gli altri lati delle pipe potranno essere chiusi */
    for (k = 0; k < N; k++)
    { 
        close(piped[k][1]); 

        if (k != N - 1)
        { 
            close(piped[k][0]); 
        }
    }

    /* Ciclo di recupero dei vari array da pipeline */
    while (read(piped[N - 1][0], cur, N * sizeof(char)) > 0)
    { 
        /* Stampa dell'array di caratteri sul file creato */
        write(fcreato, cur, N * sizeof(char));  

        /* Per questioni di leggibilita' si decide che al termine della stampa di ogni array si vada a capo */
        write(fcreato, "\n", 1); 
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
            printf("Il processo figlio con pid = %d ha ritornato il carattere '%c' (in decimale: %d, se 255 e' incorso in qualche errore durante l'esecuzione)\n", pid, ritorno, ritorno); 
        }
    }
    exit(0); 
}
