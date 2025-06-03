/* Programma C main.c che risolve la parte C della prova di esame totale tenutasi il 17 febbraio 2021 */

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
    int Q;              /* Numero di file passati per parametro e dunque di processi figli creati */
    int q;              /* Indice d'ordine per i processi figli */
    int k;              /* Indice generico per i cicli */
    pipe_t *piped;      /* Array dinamico di pipe, tramite le quali i processi figli implementano uno schema di sincronizzazione/comunicazione a ring, per il quale 
                            - Il processo P0 manda il via libera a P1, che a sua volta lo manda a P2, e via dicendo, fino ad arrivare a PN-1 che ritorna a mandare il via
                              libera nuovamente al processo P0. La sincronizzazione ovviamente si esaurisce nel momento in cui i figli finiscono le righe dei file associati 
                            - Dunque il generico figlio Pq legge dalla pipe di indice q e scrive sulla pipe di indice (q + 1) % Q. Si noti che cercare il resto rispetto a Q 
                              per quel valore e' necessario affinche' il processo PQ-1 vada a scrivere sulla pipe di indice 0 e non su quella di indice Q, che NON ESISTE */
    int fdr;            /* File descriptor associato all'apertura in lettura dei file nei figli */
    int occ;            /* Contatore di occorrenze dei caratteri numerici nelle varie righe dei file dei figli */
    char linea[250];    /* Buffer statico di caratteri per la lettura delle varie linee nei vari processi figli. Si noti che sebbene nel testo non sia specificata la massima 
                            lunghezza in caratteri di queste linee, e' ragionevole pensare che 250 caratteri siano sufficienti per contenerle (terminatore incluso)*/
    char ctrl;          /* Carattere di controllo/sincronizzazione spedito all'interno del ring. Si noti che non e' necessario andare ad inizializzare questa variabile, dal 
                            momento che viene utilizzata solo per il controllo; nessun processo utilizzera' mai il suo valore effettivo */
    int status;         /* Variabile di stato per la wait */
    int ritorno;        /* Valore ritornato dai processi figli */
    int nr, nw;         /* Variabili di controllo per lettura/scrittura da/su pipe */
    /*---------------------------------------------------*/
    
    /* Controllo lasco numero di parametri */
    if (argc < 3)
    { 
        printf("Errore: Numero di parametri passato a %s errato - Attesi almeno 2 parametri [(program) filename1 filename2 ... ] - Passati: %d\n", argv[0], argc - 1); 
        exit(1); 
    }

    /* Calcolo del valore di Q come numero di file passati */
    Q = argc - 1; 

    /* Si decide che sia per il processo padre, che per i processi figli, venga ignorato il SIGPIPE, questo perche' se ci fosse un processo Pq il quale non riesca ad aprire il 
        proprio file, quello che succede e' che questo processo terminerebbe. Allora il processo che si trova a monte, quando prova a mandare a questo processo Pq il carattere di 
        controllo e' come se provasse a scrivere su una pipe senza lettore, ricevendo dunque il SIGPIPE e terminando in modo anomalo. 
        Per lo stesso principio, se questo Pq fosse proprio P0, la terminazione anomala accadrebbe per il padre. */
    signal(SIGPIPE, SIG_IGN); 

    /* Allocazione dinamica della memoria per l'array di pipe per la sincronizzazione ad anello dei processi */
    piped = (pipe_t *)malloc(Q * sizeof(pipe_t));   
    if (piped == NULL)
    { 
        printf("Errore: Allocazione dinamica della memoria per l'array di pipe non riuscita\n"); 
        exit(2); 
    }

    /* Ciclo di creazione delle pipe */
    for (k = 0; k < Q; k++)
    { 
        if (pipe(piped[k]) < 0)
        { 
            printf("Errore: Creazione della pipe di indice %d non riuscita\n", k); 
            exit(3);
        }
    }

    /* Ciclo di creazione dei processi figli */
    for (q = 0; q < Q; q++)
    { 
        if ((pid = fork()) < 0)
        { 
            printf("Errore: fork() di creazione del processo figlio di indice %d non riuscita\n", q); 
            exit(4); 
        }

        if (pid == 0)
        { 
            /* CODICE DEI FIGLI */

            /* Si decide che in caso i processi figli incorrano in qualche tipo di errore, ritornino il valore -1 (255 senza segno), cosi' che la terminazione dovuta ad un errore, possa
                essere distinta da quella normale. Infatti nel caso normale i processi figli ritornano il numero di occorrenze numeriche nell'ultima riga, che per specifica risultera' 
                essere strettamente minore di 255 (-1) */
            printf("DEBUG-Sono il processo figlio di indice %d e con pid = %d e sto per effettuare la lettura del file %s\n", q, getpid(), argv[q + 1]); 

            /* Chiusura dei lati delle pipe non utilizzati dal processo figlio per la comunicazione a ring */
            for (k = 0; k < Q; k++)
            { 
                if (k != q)
                { 
                    close(piped[k][0]); 
                }

                if (k != (q + 1) % Q)
                {
                    close(piped[k][1]); 
                }
            }

            /* Apertura del file associato al processo */
            if ((fdr = open(argv[q + 1], O_RDONLY)) < 0)
            {
                printf("Errore: Apertura in lettura del file %s da parte del figlio di indice %d non riuscita - File non leggibile oppure inesistente\n", argv[q + 1], q); 
                exit(-1); 
            }

            /* Ciclo di lettura delle righe da file con implementazione della sincronizzazione a ring con gli altri figli */
            k = 0;      /* Inizializzazione a 0 dell'indice per il buffer linea */
            occ = 0;    /* Inizializzazione a 0 del numero di occorrenze numeriche */
            while (read(fdr, &(linea[k]), 1) > 0)
            {

                if (linea[k] == '\n')
                { 
                    /* Letta una linea dunque si attende il via libera da parte del processo precedente per stampare */
                    nr = read(piped[q][0], &ctrl, 1); 

                    /* Controllo della lettura da pipe */
                    if (nr != 1) 
                    { 
                        printf("Errore: Il figlio di indice %d non e' riuscito a leggere il carattere di controllo - byte letti: %d\n", q, nr); 
                        exit(-1); 
                    }

                    /* Trasformazione della linea in stringa per permetterne la stampa tramite printf */
                    linea[k] = '\0'; 

                    /* Stampa su stdout richiesta dal testo */
                    printf("Il processo figlio di indice %d e con pid = %d ha letto %d caratteri numerici dalla linea: '%s'\n", q, getpid(), occ, linea); 

                    /* Invio del via libera al processo successivo */
                    nw = write(piped[(q + 1) % Q][1], &ctrl, 1); 

                    /* Controllo della scrittura su pipe */
                    if (nw != 1)
                    {
                        printf("Errore: Il figlio di indice %d non e' riuscito a scrivere il carattere di controllo - byte scritti: %d\n", q, nw); 
                        exit(-1);  
                    }

                    /* Reset di k e occ dal momento che si sta per iniziare la lettura di una nuovo riga. Si noti che il valore di occ viene salvato in ritorno, pronto per 
                        essere tornato al padre, qualora si abbia a che fare con l'ultima riga */
                    ritorno = occ; 
                    occ = 0; 
                    k = 0; 
                }
                else
                { 
                    /* Controllo che si abbia trovato un carattere numerico */
                    if (isdigit(linea[k]))
                    { 
                        occ++; 
                    }

                    k++;
                }
            }

            exit(ritorno); 
        }
    }

    /* CODICE DEL PADRE */

    /* Il processo padre e' sostanzialmente escluso dalla sincornizzazione/comunicazione; infatti si limita a mandare il via libera iniziale al processo P0 tramite il lato di scrittura della 
        pipe di indice 0 per poi non utilizzare piu' le pipe. Alla luce di cio' e' possibile andare a chiudere tutti i lati delle pipe non utilizzate dela padre */
    for (k = 0; k < Q; k++)
    { 
        close(piped[k][0]); 

        if (k != 0)
        { 
            close(piped[k][1]); 
        }
    }

    /* Invio al figlio P0 il primo via libera */
    nw = write(piped[0][1], &ctrl, 1); 

    /* Controllo della scrittura su pipe */
    if (nw != 1)
    { 
        printf("Errore: il processo padre non e' riuscito a mandare il carattere di controllo al figlio P0 - byte scritti: %d\n", nw);
        /* exit(5)      Si decide di non terminare il padre in modo tale che possa comunque attendere i processi figli */ 
    }

    /* Chiusura del lato di scrittura della pipe di indice 0 utilizzate per comunicare con P0 */
    close(piped[0][1]);    
    
    /* Ciclo di attesa dei processi figli con recupero e stampa del valore tornato */
    for (q = 0; q < Q; q++)
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
