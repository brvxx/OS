/* Programma C main.c che risolve la parte C della prova di esame totale tenutasi il 16 giugno 2021 */

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

/* Definizione del TIPO tipoL che rappresenta un array di 250 caratteri. La definizione di questo tipo facilita la gestione dell'array di linee da passare tra i vari 
    figli nel pipe ring */
typedef char tipoL[250]; 

int main (int argc, char **argv)
{
    /*---------------- Variabili locali -----------------*/
    int pid;            /* Identificatore dei process figli creati con la fork e attesi con la wait */ 
    int N;              /* Numero di file passati per parametro e dunque di processi figli creati */
    int n;              /* Indice d'ordine per i processi figli */
    int i, j, k;           /* Indici generici per i cicli */
    pipe_t *piped;      /* Array dinamico di pipe utilizzate per l'implementazione di una comunicazione/sincronizzazione tra i processi a pipe ring, dove: 
                            - Il processo P0 comunica con P1 che a sua volta comunica con P2 e via dicendo, fino ad arrivare a PN-1 che comunica di nuovo con P0 
                            - Il generico processo figlio Pn legge dalla pipe di indice n e scrive sulla pipe di indice (n + 1) % N 
                            - Il processo padre da il via alla comunicazione scrivendo sulla pipe di indice 0 un array senza informazioni significative al processo P0 */
    tipoL *tutteLinee;  /* Array dinamico di linee che viene passato tra i vari processi per il loro schema di comunicazione a pipe ring */
    tipoL linea;        /* Buffer statico di caratteri per la lettura delle varie linee da parte dei processi figli */
    int fdr;            /* File descriptor associato all'apertura dei vari file in lettura nei figli */
    int fcreato;        /* File descriptor associato al file creato dal padre */
    int status;         /* Variabile di stato per la wait */
    int ritorno;        /* Valore ritornato dai processi figli */
    int nr, nw;         /* Variabili di controllo per lettura/scrittura da/su pipe */
    /*---------------------------------------------------*/
    
    /* Controllo lasco numero di parametri passati */
    if (argc < 3)
    { 
        printf("Errore: numero di parametri passati a %s errato - Attesi almeno 2 [(program) filename1 filename2 ... ] - Passati: %d\n", argv[0], argc - 1); 
        exit(1); 
    }

    /* Calcolo del valore di N come numero di file passati */
    N = argc - 1; 

    /* Allocazione dinamica della memoria per l'array di linee */
    tutteLinee = (tipoL *)malloc(N * sizeof(tipoL)); 
    if (tutteLinee == NULL)
    {
        printf("Errore: Allocazione dinamica della memoria per l'array di linee tutteLinee non riuscita\n"); 
        exit(2); 
    }

    /* Allocazione dinamica della memoria per l'array di pipe */
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
            printf("Errore: Creazione della pipe di indice %d per la comunicazione a pipe ring dei processi non riuscita\n", k);
            exit(4);  
        }
    }

    /* Creazione del file corrispondente al proprio cognome da parte del padre */
    if ((fcreato = creat("Salseo", PERM)) < 0)
    { 
        printf("Errore: Creazione del file Salseo non riuscita\n"); 
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
                essere distinta da quella normale. Infatti nel caso normale i vari processi figli ritornano al padre la lunghezza in caratteri dell'ultima linea letta che per specifica 
                risulta avere lunghezza massima di 250 caratteri (compreso il terminatore), dunque inferiore a 255 */
            printf("DEBUG-Sono il processo figlio di indice %d e con pid = %d e sto per effettuare la lettura del file %s\n", n, getpid(), argv[n + 1]); 

            /* Chiusura dei lati delle pipe non utilizzati dal processo figlio alla luce di quanto esplicitato ad inizio del codice per la comunicazione a pipe ring */
            for (k = 0; k < N; k++)
            { 
                if (k != n)
                { 
                    close(piped[k][0]); 
                }

                if (k != (n + 1) % N) 
                { 
                    close(piped[k][1]); 
                }
            }

            /* Apertura del file associato */
            if ((fdr = open(argv[n + 1], O_RDONLY)) < 0)
            {
                printf("Errore: Apertura in lettura del file %s da parte del processo figlio di indice %d non riuscita - File non leggibile oppure inesistente\n", argv[n + 1], n); 
                exit(-1); 
            }

            /* Ciclo di lettura da file carattere per carattere con salvataggio delle linee e sincronizzazione coi vari figli a pipe ring */
            k = 0;      /* Inizializzazione a 0 dell'indice per il buffer */
            while (read(fdr, &(linea[k]), 1) > 0)
            { 
                if (linea[k] == '\n')
                { 
                    /* Trovata una linea, dunque ci si mette in attesa della ricezione dell'array spedito dal figlio precedente */
                    nr = read(piped[n][0], tutteLinee, N * sizeof(tipoL)); 

                    /* Controllo che la lettura dell'array sia riuscita */
                    if (nr != N * sizeof(tipoL))
                    { 
                        printf("Errore: Il processo figlio di indice %d ha letto un numero errato di byte da pipe ring: %d\n", n, nr); 
                        exit(-1); 
                    }

                    /* Si incrementa il valore di k in modo tale che questo rappresenti la lunghezza della linea corrente tenendo in conto anche del terminatore */
                    k++; 

                    /* Copia della riga nell'array attraverso la memcpy */
                    memcpy(tutteLinee[n], linea, k); 

                    if (n == N - 1)
                    { 
                        /* Solo il processo figlio di indice N - 1 va a stampare le varie linee dell'array corrente sul file creato dal padre */

                        /* Ciclo che scorre le varie LINEE */
                        for (j = 0; j < N; j++)
                        {   
                            /* Ciclo che scorre i vari caratteri delle SINGOLE linee. Dal momento che si ha definito che la lunghezza massima delle righe sia 250 caratteri 
                                e' dunque possibile far gireare questo for per tale valore, effettuando una break alla lettura del \n che POTREBBE trovarsi prima della fine 
                                di tale array  */
                            for (i = 0; i < 250; i++)
                            { 
                                /* Scrittura del carattere corrente */
                                write(fcreato, &(tutteLinee[j][i]), 1); 

                                if (tutteLinee[j][i] == '\n')
                                { 
                                    /* Se ci si trova in corrispondenza del terminatore di linea significa che si ha concluso la lettura della linea corrente ed e' dunque possibile passare 
                                        alla successiva */
                                    break; 
                                }
                            }
                        }
                    }

                    /* Invio dell'array di linee al processo successivo secondo lo schema di comunicazione e pipeline */
                    nw = write(piped[(n + 1) % N][1], tutteLinee, N * sizeof(tipoL)); 

                    /* Controllo che la scrittura su pipe ring sia riuscita */
                    if (nw != N * sizeof(tipoL))
                    { 
                        printf("Errore: Il processo figlio di indice %d ha scritto un numero errato di byte nel pipe ring: %d\n", n, nw); 
                        exit(-1); 
                    } 

                    /* Salvataggio della lugnhezza della riga, cosi' che per la riga finale il valore di ritorno sia gia' preimpostato a quello della lunghezza di tale linea */
                    ritorno = k; 
                    k = 0;  /* Reset del valore dell'indice del buffer per la lettura della linea successiva */
                }
                else 
                { 
                    /* Se non ci si trova alla fine della linea ci si limita ad incrementare il valore dell'indice */
                    k++; 
                }
            }

            /* Ritorno della lunghezza dell'utlima linea letta da file */
            exit(ritorno); 
        }
    }

    /* CODICE DEL PADRE */

    /* Il processo padre si limita a dare il via al processo di comunicazione, andando dunque a mandare al processo figlio P0 un array tutteLinee NON inizializzato, per poi chiudere tutti i lati 
        delle pipe TRANNE quello in lettura delle pipe di indice 0, ossia quella su cui scrive l'ultimo processo. Questo per fare in modo che l'ultima scrittura (inutile) da parte del processo PN-1
        non finisca su una pipe senza lettore (dal momento che P0 verosimilmente sara' gia' terminato) causando il SIGPIPE, ma ci sia appunto il padre a fungere da lettore fittizio per questa pipe */   
    for (k = 0; k < N; k++)
    {
        if (k != 0)
        { 
            close(piped[k][0]); 
            close(piped[k][1]); 
        }
    }
    
    /* Invio dell'array al processo P0 per iniziare la sincronizzazione */
    nw = write(piped[0][1], tutteLinee, N * sizeof(tipoL)); 

    /* Controllo scrittura */
    if (nw != N * sizeof(tipoL))
    { 
        printf("Errore: il processo padre ha scritto un numero errato di byte sulla pipe di comunicazione col processo P0\n"); 
        /* exit(7)      Si decide comunque di non terminare il padre affinche' questo possa comunque attendere i processi figli */
    }

    /* Effettuata la scrittura sulla pipe 0 il padre puo' procedere con la chiusura di quel lato */
    close(piped[0][1]); 

    /* Ciclo di attesa dei processi figli con recupero e stampa del valore tornato */
    for (n = 0; n < N; n++)
    { 
        if ((pid = wait(&status)) < 0)
        { 
            printf("Errore: wait() di attesa di uno dei processi figli non riuscita\n"); 
            exit(6); 
        }

        if ((status & 0xFF) != 0)
        { 
            printf("Il processo figlio con pid = %d e' terminato in modo anomalo\n", pid); 
        }
        else 
        { 
            ritorno = (int)((status >> 8) & 0xFF); 
            printf("Il processo figlio con pid = %d ha ritornato il valore %d (se 255 e' incorso in qualche errore durante l'esecuzione)\n", pid, ritorno); 
        }
    }
    exit(0); 
}
