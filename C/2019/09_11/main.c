/* Programma C main.c che risolve la parte C della prova di esame totale tenutasi il 11 settembre 2019 */

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
    pipe_t *piped;      /* Array dinamico di pipe per la comunicazione tra processi in coppia, con verso first->second */
    int letti;            /* Contatore di caratteri letti in ogni processo figlio */
    char c;             /* Carattere letto DA FILE dai singoli processi */
    char c_letto;       /* Carattere letto DA PIPE dai processi dispari di ogni coppia, inviato dal processo pari della coppia */
    char c_max;         /* Carattere massimo risultante dal confronto tra c e c_max per ogni carattere contenuto nel file */
    int fdr;            /* File descriptor associta all'apertura in lettura del file nei vari figli */
    int fcreato;        /* File descriptor asscoiato al file creato da parte del secondo figlio di ogni coppia */
    char *Fcreato;      /* Buffer di memoria allocato dinamicamente per conternere esattamente la lunghezza del nome del file da creare per ogni coppia */
    int status;         /* Variabile di stato per la wait */
    int ritorno;        /* Valore ritornato dai processi figli */
    int nr, nw;         /* Variabili di controllo per lettura/scrittura da/su pipe */
    /*---------------------------------------------------*/
    
    /* Controllo lasco numero di parametri passati e controllo che N sia pari */
    if ((argc < 3) || ((argc - 1) % 2 == 1))
    { 
        printf("Errore: numero di parametri passati a %s errato - Atteso un numero PARI di parametri maggiore uguale di 2 [(program) filename1 filename2 ... ] - Passati: %d\n", argv[0], argc - 1); 
        exit(1); 
    }

    /* Calcolo del valore di N come numero di file passati */
    N = argc - 1; 

    /* Allocazione dinamica della memoria per l'array di N/2 pipe. Infatti avendo N processi (quanti il numero di file) ma una comunicazione che avviene tra COPPIE di processi, allora il 
        numero necessario di pipe sara' pari alla META' dei processi totali (ossia il numero di copppie). Si noti che dal momento che N e' pari, la divisione per 2 dara' come risultato un 
        numero intero */
    piped = (pipe_t *)malloc(N/2 * sizeof(pipe_t)); 
    if (piped == NULL)
    { 
        printf("Errore: allocazione dinamica della memoria per l'array di pipe non riuscita\n"); 
        exit(2);
    }

    /* Ciclo di creazione delle pipe */
    for (k = 0; k < N/2; k++)
    { 
        if (pipe(piped[k]) < 0)
        { 
            printf("Errore: crezione della pipe di indice %d non riuscita\n", k); 
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

            /* Si decide che in caso i processi figli incorrano in qualche tipo di errore, ritornino il valore 0 , cosi' che la terminazione dovuta ad un errore, possa
                essere distinta da quella normale. Infatti i processi figli ritornano il numero di caratteri letti dal file associato, che viene garantito dalla parte 
                di shell essere un valore compreso nel range (1, 255], quindi sicuramente DIVERSO da 0. */
            
            /* Apertura del file associato, codice identico per entrambi i figli di ogni coppia */
            if ((fdr = open(argv[n + 1], O_RDONLY)) < 0)
            { 
                printf("Errore: apertura in lettura del file %s da parte del figlio di indice %d non riuscita\n", argv[n + 1], n); 
                exit(0); 
            }


            /* Divisione del codice dei figli delle singole coppie */
            if (n % 2 == 0)
            { 
                /* Codice dei figli di indice pari, dunque primi figli delle varie coppie */

                printf("DEBUG-Sono in processo figlio di indice %d e con pid = %d, primo figlio della coppia C%d, e sto per leggere dal file %s\n", n, getpid(), n/2, argv[n + 1]); 

                /* Dal momento che ci possono essere dei casi particolari in cui il primo processo della coppia, il processo scrittore per la pipe, va a scrivere su una 
                    pipe senza lettore (caso in cui il file del secondo processo non e' leggibile, oppure se presenta per un qualche motivo molti meno caratteri), questo 
                    processo INEVITABILMENTE riceverebbe il SIGPIPE terminando in modo anomalo. Si decide dunque di ingorare questo segnale */
                signal(SIGPIPE, SIG_IGN);

                /* Il generico processo figlio Pp si comporta come produttore per il processo accoppiato Pd, andando a scrivere sul lato di scrittura della pipe di indice n/2. 
                    Di conseguenza, tutti i lati delle pipe non utilizzati verranno chiusi */
                for (k = 0; k < N/2; k++)
                { 
                    close(piped[k][0]); 

                    if (k != n/2)
                    { 
                        close(piped[k][1]); 
                    }
                }

                /* Inizializzazione a 0 del contatore di caratteri */
                letti = 0; 

                /* Ciclo di lettura da file carattere per carattere */
                while (read(fdr, &c, 1) > 0)
                { 
                    /* Incremento del contatore di caratteri letti */
                    letti++; 

                    /* Invio del carattere letto al processo dispari della coppia */
                    nw = write(piped[n/2][1], &c, 1); 

                    /* Verifica della corretta scrittura su pipe */
                    if (nw != 1)
                    { 
                        printf("Errore: Il processo figlio di indice %d non e' riuscito a scrivere correttamente un carattere nella pipe di comunicazione col figlio accoppiato\n", n); 
                        exit(0);
                    }
                }
            }
            else 
            { 
                /* Codice dei figli di indice dispari, dunque secondi figli delle varie coppie */

                printf("DEBUG-Sono in processo figlio di indice %d e con pid = %d, secondo figlio della coppia C%d, e sto per leggere dal file %s\n", n, getpid(), n/2, argv[n + 1]); 
                
                /* Il generico processo figlio Pd si comporta come consumatore per il processo accoppiato Pp, andando a leggere dal lato di lettura della pipe di indice n/2. 
                    Di conseguenza, tutti i lati delle pipe non utilizzati verranno chiusi */
                for (k = 0; k < N/2; k++)
                { 
                    close(piped[k][1]); 

                    if (k != n/2)
                    { 
                        close(piped[k][0]); 
                    }
                }

                /* Come da richiesta nel testo, la prima cosa che fanno i figli dispari delle coppie e' creare il file Fcreato. Per prima cosa si va ad allocare la memoria necessaria per 
                    il suo nome. La dimensione del buffer viene calolata come lunghezza della stringa del nome del file associata al processo + 10, ossia la lunghezza dell'estensione + il
                    terminatore di stringa '\0' */
                Fcreato = (char *)malloc((strlen(argv[n + 1]) + 10) * sizeof(char)); 
                if (Fcreato == NULL)
                { 
                    printf("Errore: Allocazione dinamica della memoria per il nome del file della coppia C%d non riuscita\n", n/2); 
                    exit(0); 
                }

                /* Copia del nome del file associato al processo nel buffer */
                strcpy(Fcreato, argv[n + 1]); 

                /* Concatenazione con l'estensione '.MAGGIORE' */
                strcat(Fcreato, ".MAGGIORE"); 

                /* Creazione effettiva del file */
                if ((fcreato = creat(Fcreato, PERM)) < 0)
                { 
                    printf("Errore: Creazione del file %s da parte del secondo figlio della coppia C%d non riuscita\n", Fcreato, n/2); 
                    exit(0); 
                }


                /* Inizializzazione a 0 del contatore di caratteri */
                letti = 0; 

                /* Ciclo di lettura da file carattere per carattere */
                while (read(fdr, &c, 1) > 0)
                { 
                    /* Incremento del contatore di caratteri letti */
                    letti++; 

                    /* Ricezione del carattere scritto dal processo pari della coppia */
                    nr = read(piped[n/2][0], &c_letto, 1); 

                    /* Verifica della corretta lettura da pipe */
                    if (nr != 1)
                    { 
                        printf("Errore: Il processo figlio di indice %d non e' riuscito a leggere dalla pipe il carattere inviato dal primo figlio della coppia\n", n); 
                        exit(0);
                    }

                    /* Ricerca del massimo tra i due caratteri (quello inviato dal primo figlio della coppia e quello letto da file) */
                    if (c > c_letto)
                    {
                        c_max = c; 
                    }
                    else 
                    { 
                        /* se c <= c_letto */
                        c_max = c_letto; 
                    }

                    /* Scrittura del carattere sul file creato */
                    write(fcreato, &c_max, 1); 
                }
            }

            exit(letti); 
        }
    }

    /* CODICE DEL PADRE */

    /* Il proesso pedre e' escluso dallo schema di comunicazione; alla luce di tale considerazione si vanno a chiudere TUTTI i lati di TUTTE le pipe */
    for(k = 0; k < N/2; k++)
    {
        close(piped[k][0]);
        close(piped[k][1]);
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
            printf("Il processo figlio con pid = %d ha ritornato il valore %d (se 0 e' incorso in qualche errore durante l'esecuzione)\n", pid, ritorno); 
        }
    }
    exit(0); 
}
