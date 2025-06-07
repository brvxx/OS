/* Programma C main.c che risolve la parte C della seconda prova in itinere tenuta il giorno 05 giugno 2025 */

#include <stdlib.h> 
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/wait.h> 
#include <string.h> 
#include <ctype.h>
#include <signal.h> 

/* Definizione della cosatante LSIZE che rappresenta la lunghezza massima delle righe del file passato come primo parametro. Si noti che sebbene questo valore
     non sia specificato nel testo e' ragionevole assumere che 250 caratteri possa essere una lunghezza sufficiente per le singole righe (compreso il terminatore) */
#define LSIZE 250	

/* Definizione del TIPO pipe_t che rappresenta un array di due interi, che useremo per la gestione dei lati delle varie pipe create per lo schema di comunicazione */
typedef int pipe_t[2];

int main (int argc, char **argv)
{
    /*---------------- Variabili locali -----------------*/
    int pid;            /* Identificatore dei processi figli creati con la fork e attesi con la wait */ 
    int M;              /* Numero di righe del file passato come secondo parametro e dunque numero di processi figli creati dal padre */
    int m;              /* Indice d'ordine per i processi figli */
    int k;              /* Indice generico per i cicli */
    pipe_t *piped;      /* Array dinamico di pipe che permette l'implementazione di una sincronizzazione/comunicazione a pipeline tra i SOLI processi figli, dove:    
                            - Il processo P0 comunica col processo P1 che a sua volta comunica con P2, fino ad arrivare a PM-1 che non comunica con nessuno 
                            - Il generico processo Pm (con m != 0 e m != M - 1) effettua la lettura della pipe di indice m - 1 e la scrittura su quella di indice m 
                            - IL processo P0 si limita a scrivere sulla pipe 0 
                            - Il processo PM-1 si limita a leggere dalla pipe di indice M - 2 
                           Si noti che dal momento che il padre non e' incluso nello schema di sincronizzazione, il numero di pipe necessarie sara' M - 1 e NON M */
    int openfile;       /* File descriptor associato all'apertura del file da parte del padre */
    char ctrl;          /* Carattere di controllo passato tra i figli per sincronizzarsi. Si noti che il valore effettivo di questo parametro non e' importante ai 
                            fini dell'esecuzione, dunque non verra' inizializzato */
    char LINE[LSIZE];   /* Buffer statico di caratteri usato dai figli per leggere la rispettiva riga del file aperto dal padre. Si noti che dal momento che l'apertura 
                            viene fatta SOLO ed esclusivamente dal padre TUTTI i figli condivideranno l'I/O pointer sul file, per questo motivo e' necessaria la sincronizzazione 
                            affinche' ogni figlio legga solo ed esclusivametne la propria linea */
    int status;         /* Variabile di stato per la wait */
    int ritorno;        /* Valore ritornato dai processi figli */
    int nr, nw;         /* Variabili di controllo per lettura/scrittura da/su pipe */
    /*---------------------------------------------------*/
    
    /* Controllo stretto numero di parametri passati */
    if (argc != 4)
    {
        printf("Errore: Numero di parametri passati a %s errato - Attesi esattamente 3 parametri [(program) numpos filename string ] - Passati: %d\n", argv[0], argc - 1);
        exit(1);
    }

    /* Recupero e controllo del valore M passato per parametro */
    M = atoi(argv[1]); 
    if (M <= 0)
    {
        printf("Errore: Il parametro %s passato non rappresenta un numero intero strettamente positivo\n", argv[1]); 
        exit(2); 
    }

    /* Come richiesto dal testo il padre come prima cosa effettua l'apertura in lettura del file passato come primo parametro al processo */
    if ((openfile = open(argv[2], O_RDONLY)) < 0)
    {
        printf("Errore: Apertura in lettura del file '%s' da parte del processo padre non riuscita - File non leggibile oppure inesistente\n", argv[2]);
        exit(3); 
    }

    /* Allocazione dinamica della memoria per l'array di M - 1 pipe */
    piped = (pipe_t *)malloc((M - 1) * sizeof(pipe_t)); 
    if (piped == NULL)
    { 
        printf("Errore: Allocazione dinamica della memoria per l'array di pipe non riuscita\n"); 
        exit(4); 
    }

    /* Ciclo di creazione delle M - 1 pipe */
    for (k = 0; k < M - 1; k++)
    {
        if (pipe(piped[k]) < 0)
        { 
            printf("Errore: Creazione della pipe di indice %d per la comunicazione a pipeline non riuscita\n", k); 
            exit(5); 
        }
    }

    printf("DEBUG-Sono il processo padre con pid=%d e creero' %d processi figli\n", getpid(), M);

    /* Ciclo di creazione dei processi figli */
    for (m = 0; m < M; m++)
    { 
        if ((pid = fork()) < 0)
        { 
            printf("Errore: fork() di creazione del processo figlio di indice %d non riuscita\n", m); 
            exit(6); 
        }

        if (pid == 0)
        { 
            /* CODICE DEI FIGLI */

            /* Si decide che in caso i processi figli incorrano in qualche tipo di errore, ritornino il valore -1 (255 senza segno), cosi' che la terminazione dovuta ad un errore, possa
                essere distinta da quella normale. Infatti nel caso normale i processi figli ritornano al padre il valore tornato dal nipote, il quale esegue il comando chmod, per il quale
                i valori di ritorno sono molto piccoli dunque differenti dal 255 */
            
            /* printf("DEBUG-Sono il processo figlio di indice %d e con pid = %d e sto per creare un processo nipote che eseguira' il comando chmod\n", m, getpid()); */

            /* Chiusura dei lati delle pipe non utilizzati rispettando lo schema di sincronizzazione esplicitato ad inizio del codice */
            for (k = 0; k < M - 1; k++)
            {   
                /* Chiusura lati di lettura */
                if ((k != m - 1) || (m == 0))
                {
                    close(piped[k][0]); 
                }

                /* Chiusura lati di scrittura */
                if ((k != m) || (m == M - 1))
                { 
                    close(piped[k][1]);
                }
            }

            /* Lettura da pipeline per sincronizzazione con gli altri processi. Ovviamente questa lettura NON viene effettuata dal processo di indice 0 dal momento che e' il primo 
                processo della pipeline */
            if (m != 0) 
            { 
                nr = read(piped[m - 1][0], &ctrl, 1); 

                /* Verifica della validita' della lettura */
                if (nr != 1)
                { 
                    printf("Errore: Il figlio di indice %d ha letto un numero errato di byte dalla pipeline: %d\n", m, nr); 
                    exit(-1); 
                }
            }

            /* Ciclo di lettura della linea associata al file. Si noti che dal momento che si e' imposto che la lunghezza del buffer sia al massimo lSIZE 
                e' dunque possibile far variare il valore dell'indice per il buffer tra 0 e LSIZE - 1 */
            for (k = 0; k < LSIZE; k++)
            { 
                read(openfile, &(LINE[k]), 1);

                if (LINE[k] == '\n')
                { 
                    /* Raggiunto il termine della linea, dunque si trasforma la linea in una STRINGA, affinche' possa essere utilizzata dal processo nipote come 
                        parametro per il comando chmod. Qeusto viene fatto andando a sostituire il carattere newline '\n' con il terminatore di stringa '\0' */
                    LINE[k] = '\0'; 

                    /* Si esce dal ciclo per evitare di leggere delle righe NON associate al processo corrente. Si noti che l'uscita dal ciclo e' NECESSARIA 
                        per prevenire la lettura delle righe associate agli altri processi */
                    break; 
                }
            }

            /* Invio del "via libera" al processo successivo sulla pipeline. In questo caso sara' il processo di indice M - 1 che NON dovra' effetuarla */
            if (m != M - 1)
            { 
                nw = write(piped[m][1], &ctrl, 1); 

                /* Verifica della validita' della scrittura su pipeline */
                if (nw != 1) 
                { 
                    printf("Errore: Il figlio di indice %d ha scritto un numero errato di byte sulla pipeline: %d\n", m, nw);
                    exit(-1); 
                }
            }

            /* Creazione del processo nipote */
            if ((pid = fork()) < 0)
            { 
                printf("Errore: fork di creazione del nipote di indice %d NON riuscita\n", m); 
                exit(-1); 
            }

            if (pid == 0)
            { 
                /* CODICE DEI NIPOTI */

                /* Analogamente a quanto detto per i figli, i nipoti che eseguono il comando chmod in caso di errore ritorneranno il valore -1 (255) */
                
                /* printf("DEBUG-Sono il processo nipote di indice %d e con pid = %d e sto per eseguire il cambio dei permessi con il comando chmod "
                        "per il file %s\n", m, getpid(), LINE); */
                
                /* Per correttezza il processo nipote chiude i lati delle pipe lasciati aperti dal rispettivo processo nipote */
                if (m != 0)
                {
                    close(piped[m][0]);
                }
                if (m != M - 1)
                { 
                    close(piped[m][1]); 
                }

                /* Dal momento che ai fini dell'esecuzione serve solo il valore TORNATO dai nipoti, si decide che onde evitare di ottenere a video delle
                     stampe di otuput non necessarie si ridirezioni sia lo stdout che lo stderr verso il device null, andndo dunque a perdere queste
                     stampe */
                close(1); 
                open("/dev/null", O_WRONLY); 

                close(2); 
                open("/dev/null", O_WRONLY); 

                /* Esecuzione del comando chmod con i permessi espressi nella stringa passata come terzo parametro sul file ricavato dal figlio. Si noti 
                    che dal momento che chmod e' un comando di sistema il suo percorso si trova all'interno della variabile di ambiente PATH, dunque per 
                    eseguire il comando verra' utilizzata la primitiva execlp */
                execlp("chmod", "chmod", argv[3], LINE, (char *)0); 

                /* Se si arriva ad eseguire questa parte di codice significa che l'esecuzione del comando chmod non e' riuscita, dunque non potendo notificare 
                    l'utente in nessun modo, avendo ridirezionato sia lo stdout che lo stder, ci si limita a ritornare il valore -1, ossia la terminazione di 
                    errore */
                exit(-1); 
            }

            /* Attesa del processo nipote da parte del figlio */
            ritorno = -1;      /* Inizializzazione del valore di ritorno a -1, questo valore verra' aggiornato a quello tornato dal nipote solo nel caso in cui
                                    si riesca effettivamente a ricevere questo valore */
            if ((wait(&status)) < 0)
            {
                printf("Errore: Wait di attesa del nipote di indice %d non riuscita\n", m); 
            }
            else if ((status & 0xFF) != 0)
            {
                printf("Errore: Il processo nipote di indice %d e con pid = %d e' terminato in modo anomalo\n", m, pid);  
            }
            else 
            { 
                ritorno = (int)((status >> 8) & 0xFF); 
                /* printf("DEBUG-Il processo nipote di indice %d e con pid = %d ha ritornato il valore %d\n", m, pid, ritorno); */
            }

            exit(ritorno); 

        }
    }

    /* CODICE DEL PADRE */
    
    /* Il processo padre e' totalmente escluso dallo schema di comunicazione/sincronizzazione, dunque e' possibile chiudere tutte le pipe */
    for (k = 0; k < M - 1; k++)
    {
        close(piped[k][0]); 
        close(piped[k][1]);
    }
         
    /* Ciclo di attesa dei processi figli con recupero e stampa del valore tornato */
    for (m = 0; m < M; m++)
    { 
        if ((pid = wait(&status)) < 0)
        { 
            printf("Errore: wait() di attesa di uno dei processi figli non riuscita\n"); 
            exit(7); 
        }

        if ((status & 0xFF) != 0)
        { 
            printf("Il processo figlio con pid=%d e' terminato in modo anomalo\n", pid); 
        }
        else 
        { 
            ritorno = (int)((status >> 8) & 0xFF); 

            if (ritorno != 0)
            { 
                /* Nipote ha fallito la propria esecuzione, oppure si ha avuto qualche problema nel figlio o nel nipote */
                printf("Il nipote del figlio con pid=%d ha ritornato %d e quindi ha fallito l'esecuzione del comando chmod oppure "
                        "se 255 il nipote o figlio ha avuto dei problemi\n", pid, ritorno); 
            }
            else
            {
                printf("Il nipote del figlio con pid = %d ha ritornato %d, quindi tutto OK!\n", pid, ritorno); 
            }   
        }
    }
    exit(0); 
}
