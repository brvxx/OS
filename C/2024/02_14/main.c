/* Programma C main.c che risolve la parte C della prova di esame totale tenutasi il 14 febbraio 2024 */

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

/* Definizione del TIPO Strut che rappresenta una particolare struttura che possa contenere informazioni dei vari figli */
typedef struct {
    char trovata[12];   /* Stringa la cui semantica specificata dal testo vuole che contenga: 
                            - "TROVATA" se il processo nipote ha effettivamente trovato la stringa associata al figlio Pn nel file 
                            - "NON TROVATA" se il processo nipote non ha trovato tale stringa */ 
    int pid_n;          /* Indice del processo nipote che ha cercato la stringa */
} Strut; 

int main (int argc, char **argv)
{
    /*---------------- Variabili locali -----------------*/
    int pid;            /* Identificatore dei process figli creati con la fork e attesi con la wait */ 
    int N;              /* Numero di stringhe passate per parametro e dunque di processi figli creati */
    int n;              /* Indice d'ordine per i processi figli */
    int k;              /* Indice generico per i cicli */
    pipe_t *piped;      /* Array dinamico di pipe che permette l'implementazione di uno schema di comunicazione tra i processi a pipeline, dove: 
                            - IL processo P0 comunica con P1, che a sua volta comunica con P2, fino ad arrivare a PN-1 che comunica col padre 
                            - Il generico processo figlio Pn dunque leggera' dalla pipe di indice n - 1 e scrivera' su quella di indice n 
                            - Il processo P0 invece si limita a scrivere senza leggere da alcuna pipe */
    Strut *cur;         /* Array dinamico di strutture che viene allocato dinamicamente da ogni figlio in base alla dimensione che questo necessita */
    int status;         /* Variabile di stato per la wait */
    int ritorno;        /* Valore ritornato dai processi figli */
    int nr, nw;         /* Variabili di controllo per lettura/scrittura da/su pipe */
    /*---------------------------------------------------*/
    
    /* Controllo lasco numero di parametri passato */
    if (argc < 4)
    { 
        printf("Errore: Numero di parametri passati a %s errato - Attesi almeno 3 [(program) file string1 string2 ... ] - Passati: %d\n", argv[0], argc - 1); 
        exit(1); 
    }

    /* Calcolo del valore di N come numero di stringhe passate per paraemtro */
    N = argc - 2; 

    /* Allocazione dinamica della memoria per l'array di N pipe */
    piped = (pipe_t *)malloc(N * sizeof(pipe_t)); 
    if (piped == NULL)
    {
         printf("Errore: Allocazione dinamica per l'array di pipe non riuscita\n"); 
         exit(2); 
    }

    /* Ciclo di creazione delle pipe */
    for (k = 0; k < N; k++)
    { 
        if (pipe(piped[k]) < 0)
        {
            printf("Errore: Creazione della pipe di indice %d non riuscita\n", k); 
            exit(3);
        }
    }

    /* Ciclo di creazione dei processi figli */
    for (n = 0; n < N; n++)
    { 
        if ((pid = fork()) < 0)
        { 
            printf("Errore: fork() di creazione del processo figlio di indice %d non riuscita\n", n); 
            exit(4); 
        }

        if (pid == 0)
        { 
            /* CODICE DEI FIGLI */

            /* Si decide che in caso i processi figli incorrano in qualche tipo di errore, ritornino il valore -1 (255 senza segno), cosi' che la terminazione dovuta ad un errore, possa
                essere distinta da quella normale. Infatti nel caso normale i processi figli ritornano il valore tornato dai rispettivi nipoti, i quali eseguono il comando grep, che sicuramente 
                ritorna dei valori piccoli, distinguibili da 255 */
            printf("DEBUG-Sono il processo figio di indice %d con pid = %d e associato alla stringa '%s' e sto per creare un processo nipote\n", n, getpid(), argv[n + 2]); 

            /* Per ogni processo figlio Pn, dal momento che questo legge solo dalla pipe di indice n - 1 e scrive su quella di indice n, e' possibile andare a 
                chiudere i lati di tutte le pipe non utilizzate. Si noti poi che il processo P0 non leggendo da alcuna pipe potra' chiudere tutti i lati di lettura  */
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

            /* Creazione del processo nipote */
            if ((pid = fork()) < 0)
            { 
                printf("Errore: Creazione del processo nipote di indice %d non riuscita\n", n); 
                exit(-1); 
            }

            if (pid == 0)
            { 
                /* CODICE DEI NIPOTI */

                /* Per lo stesso ragionamento fatto per i figli si decide che in caso di errore i nipoti ritornino -1 */

                printf("DEBUG-Sono il processo nipote di indice %d e con pid = %d e sto per eseguire il grep di '%s' nel file %s\n", n, getpid(), argv[n + 2], argv[1]); 

                /* Chiusura dei lati delle pipe lasciati aperti dal figlio Pn per la comunicazione a pipeline, dal momento che il nipote non li utilizzera' */
                close(piped[n - 1][0]); 
                close(piped[n][1]); 

                /* Si decide di andare a ridirezionare lo stdout verso il device nullo, cosi' che la stampa del programma non risenta delle varie stampe del 
                    grep effettuati dai processi nipoti */
                close(1); 
                open("/dev/null", O_WRONLY); 

                /* Stessa cosa per lo stderr */
                close(2); 
                open("/dev/null", O_WRONLY); 
                
                /* Esecuzione del comando grep della stringa associata al nipote nel file F (primo parametro passato). Si decide di utilizzare la primitiva 
                    execlp, ossia la versione con p, dal momento che si sta eseguendo un comando di sistema, il cui percoso dunque sara' contenuto dentro la 
                    variabile di ambinete PATH */
                execlp("grep", "grep", argv[n + 2], argv[1], (char *)0); 

                /* Se si arriva ad eseguire questa parte di codice allora l'exec e' fallita, ma dal momento che sono stati ridirezionati sia stdout che 
                    stderr non si puo' fare altro che terminare con errore, senza effettuare alcuna stampa di errore */
                exit(-1); 
            }

            /* Ritorno al codice dei figli */

            /* Recupero del valore ritornato dal processo nipote creato */  
            ritorno = 1;    /* Pre-inizializzazione del valore di ritorno a 1, ossia di insuccesso della ricerca, che andra' a rappresentare anche i casi 
                                in cui ci sono state problematiche di altro tipo */
            if ((pid = wait(&status)) < 0)
            { 
                printf("Errore: wait di attesa del nipote di indice %d non riuscita\n", n);
                /* exit(-1)     Si decide appunto di non terminare l'esecuzione del figlio, ma semplicemente trattare il caso di errore come se non sia 
                                stata trovavata la stringa cercata */ 
            }

            if ((status & 0xFF) != 0)
            { 
                printf("Il processo nipote di indice %d e con pid = %d e' terminato in modo anomalo\n", n, pid); 
                /* exit(-1)   Stesso ragionamento fatto per l'errore della wait */
            }
            else 
            { 
                ritorno = (int)((status >> 8) & 0xFF); 
                printf("DEBUG-Il processo nipote di indice %d e con pid = %d ha ritornato il valore %d (se 255 e' incorso in qualche errore durante l'esecuzione)\n", n, pid, ritorno); 
            }

            /* Allocazione dinamica della memoria per l'array di strutture che verra' poi passato avanti nella pipeline al processo successivo. Si noti che questo array che 
                si va ad allocrare avra' dimensione n + 1, dal momento che deve contenere tutte le strutture passate dal figlio precedente ma anche quella corrente che il figlio 
                deve andare ad INIZIALIZZARE */
            cur = (Strut *)malloc((n + 1) * sizeof(Strut)); 
            if (cur == NULL)
            { 
                printf("Errore: Allocazione dinamica dell'array di strtture cur nel figlio di indice %d non riuscita\n", n); 
                exit(-1); 
            }

            /* Lettura dell'array di strutture mandate dal figlio precedente. Ovviamente il figlio P0 non dovra' leggere da alcuna pipe in quanto primo processo della pipeline */
            if (n != 0)
            { 
                nr = read(piped[n - 1][0], cur, n * sizeof(Strut)); 

                /* Controllo validita' lettura */
                if (nr != n * sizeof(Strut))
                { 
                    printf("Errore: il figlio di indice %d ha letto un numero errato di byte dalla pipeline: %d\n", n, nr); 
                    exit(-1); 
                }
            }
            
            /* Inizializzazione della struttura del processo corrente */
            cur[n].pid_n = pid; 
            
            if (ritorno == 0)
            {   
                /* Se il valore tornato dal processo nipote e' 0, questo significa che questo ha trovato effettivamente la stringa dentro al file F, dunque come da specifica si associa 
                    la stringa "TROVATA" nella struct associata al processo corrente */
                strcpy(cur[n].trovata, "TROVATA"); 
            }
            else 
            { 
                /* Qualsiasi altro valore, che sia un 1 o un -1 (causato da un generico problema del nipote) viene interpretato come una stringa non trovata */
                strcpy(cur[n].trovata, "NON TROVATA"); 
            }

            /* Invio dell'array al processo successivo tramite pipeline */
            nw = write(piped[n][1], cur, (n + 1) * sizeof(Strut)); 

            /* Controllo validita' scrittura */
            if (nw != (n + 1) * sizeof(Strut))
            { 
                printf("Errore: il processo figlio di indice %d ha scritto un numero di byte errato sulla pipeline: %d\n", n, nw); 
                exit(-1); 
            }
            
            /* Come da specifica del testo, i figli ritornano il valore tornato dai rispettivi nipoti */
            exit(ritorno); 
        }
    }

    /* CODICE DEL PADRE */

    /* IL processo padre partecipa nello schema di comunicazione a pipeline, limitandosi pero' a leggere il risultato finale della comunicazione dalla pipe 
        di indice N - 1 (ultima pipe). Alla luce di cio' e' dunque possibile andare a chiudere tutti i lati delle pipe non utilizzati */
    for (k = 0; k < N; k++)
    { 
        close(piped[k][1]); 

        if (k != N - 1)
        { 
            close(piped[k][0]);
        }
    }
    
    /* Allocazione dinamica dell'array cur. Dal momento che il padre riceve l'array finale, questo dovra' contenere esattamente N strutture, esattamente quanti 
        sono i processi figli generati */
    cur = (Strut *)malloc(N * sizeof(Strut)); 
    if (cur == NULL)
    { 
        printf("Errore: Allocazione dinamica dell'array cur nel padre NON riuscita\n");
        /* exit(5)  Si decide di non terminare il padre, affinche' questo possa attendere i processi figli creati */
    }
     
    /* Ricezione dell'array da pipeline */
    nr = read(piped[N - 1][0], cur, N * sizeof(Strut)); 

    /* Controllo lettura da pipeline */
    if (nr != N * sizeof(Strut))
    { 
        printf("Errore: Il processo padre ha letto un numero errato di byte dalla pipeline: %d\n", nr); 
        /* exit(6)      Per lo stesso ragionamento fatto sopra */
    }
    else
    { 
        /* Ciclo di lettura delle varie strutture dell'array con stampa */
        for (n = 0; n < N; n++)
        { 
            if (strcmp(cur[n].trovata, "TROVATA") == 0)
            { 
                /* Il nipote di indice n ha trovato la stringa a lui associata, nel file F */
                printf("Il processo processo figlio di indice %d ha ritornato la stringa '%s', dunque il processo nipote con pid = %d da lui generato "
                        "ha trovato correttamente la stringa '%s' nel file %s\n", n, cur[n].trovata, cur[n].pid_n, argv[n + 2], argv[1]); 
            }
            else 
            { 
                printf("Il processo processo figlio di indice %d ha ritornato la stringa '%s', dunque il processo nipote con pid = %d da lui generato "
                        "NON e' riuscito a trovare la stringa '%s' nel file %s\n", n, cur[n].trovata, cur[n].pid_n, argv[n + 2], argv[1]); 
            }
        } 
    }
       

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
