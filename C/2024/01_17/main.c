/* Programma C main.c che risolve la parte C della prova di esame totale tenutasi il 17 gennaio 2024 */

#include <stdlib.h> 
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/wait.h> 
#include <string.h> 
#include <ctype.h>
#include <signal.h> 
#include <limits.h>

/* Definizione del TIPO pipe_t che rappresenta un array di due interi, che useremo per la gestione dei lati delle varie pipe create */
typedef int pipe_t[2];

int main (int argc, char **argv)
{
    /*---------------- Variabili locali -----------------*/
    int pid;            /* Identificatore dei process figli creati con la fork e attesi con la wait */ 
    int N;              /* Numero di file passati per parametro e dunque di processi figli creati */
    int n;              /* Indice d'ordine per i processi figli */
    int k;              /* Indice generico per i cicli */
    pipe_t *pipe_fp;    /* Array dinamico di pipe che permettono la comunicazione tra processi nel verso figli-->pare */
    pipe_t *pipe_pf;    /* Array dinamico di pipe che permettono la comunicazione tra processi nel verso padre-->figli */
    int fdr;            /* File descriptor associato all'apertura del file nei figli */
    char c;             /* Variabile utilizzata dai figli per salvare il primo carattere dell'ultima linea */
    char ctrl;          /* Carattere di controllo inviato dal padre ai figli per richiederne la stampa o meno. Si decide che: 
                            - se vale 'S' allora il processo figlio che lo ha ricevuto dovra' stampare 
                            - se vale 'N' allora il processo figlio che lo ha ricevuot NON dovra' stampare */
    char car;           /* Carattere usato dal padre per leggere i caratteri dei figli */
    char c_min;         /* Carattere minimo calcolato dal padre tra quelli inviati dai figli */
    int idx_min;        /* Indice del processo figlio che ha trovato il minimo carattere */
    char linea[250];    /* Buffer statico di caratteri per la lettura delle linee da parte dei figli nei loro file. Si suppone come indicato 
                            nel testo che la lunghezza delle righe sia sempre minore o uguale a 250 caratteri (compreso il terminatore) */
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

    /* Calcolo di N come numero totale di file passati per paraemtro */
    N = argc - 1; 

    /* Allocazione dinamica della memoria per gli array di pipe per i 2 schemi di comunicazione */
    pipe_fp = (pipe_t *)malloc(N * sizeof(pipe_t));
    pipe_pf = (pipe_t *)malloc(N * sizeof(pipe_t)); 
    if ((pipe_fp == NULL) || (pipe_pf == NULL))
    {
        printf("Errore: Allocazione dinamica della memoria per gli array di pipe non riuscita\n");
        exit(2); 
    }

    /* Ciclo di creazione delle pipe */
    for (k = 0; k < N; k++)
    { 
        if(pipe(pipe_fp[k]) < 0)
        {
            printf("Errore: Creazione della pipe di indice %d per la comunicazione figli-->padre non riuscita\n", k);
            exit(3); 
        }

        if(pipe(pipe_pf[k]) < 0)
        {
            printf("Errore: Creazione della pipe di indice %d per la comunicazione padre-->figli non riuscita\n", k);
            exit(4); 
        }
    }

    /* Ciclo di creazione dei processi figli */
    for (n = 0; n < N; n++)
    { 
        if ((pid = fork()) < 0)
        { 
            printf("Errore: fork() di creazione del processo figlio di indice %d non riuscita\n", n); 
            exit(5); 
        }

        if (pid == 0)
        { 
            /* CODICE DEI FIGLI */

            /* Si decide che in caso i processi figli incorrano in qualche tipo di errore, ritornino il valore -1 (255 senza segno), cosi' che la terminazione dovuta ad un errore, possa
                essere distinta da quella normale, per la quale i figli ritornano i valori 0 o 1, differenti da 255 */
            
            printf("DEBUG-Sono il processo figlio di indice %d e con pid = %d e sto per leggere dal file %s\n", n, getpid(), argv[n + 1]); 

            /* Il generico processo figlio Pn 
                - Scrive sulla pipe di indice n per lo schema di comunicazione figli-->padre 
                - Legge dalla pipe di indice n per lo schema di comunicaizone padre-->figli 
               Alla luce dei tali considerazioni, tutti gli altri lati della pipe possono essere chiusi */
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

            /* Apertura del file associato */
            if ((fdr = open(argv[n + 1], O_RDONLY)) < 0)
            { 
                printf("Errore: Apertura del file %s da parte del figlio di indice %d non riuscita - File non leggibile oppure inesistente\n", argv[n + 1], n); 
                exit(-1); 
            }

            /* Inizializzazione dell'indice per il buffer a 0 */
            k = 0; 

            /* Ciclo di lettura da file con salvataggio delle righe */
            while (read(fdr, &(linea[k]), 1) > 0)
            { 
                if (k == 0)
                { 
                    /* Ci si trova sul primo carattere della linea, dunque lo si salva, cosi' che al termine dell'iterazione rimanga salvato il primo carattere 
                        dell'ultima linea */
                    c = linea[k]; 
                }

                if (linea[k] == '\n')
                { 
                    /* Trasformazione della linea in stringa, cosi' che possa essere stampata */
                    linea[k] = '\0'; 

                    /* Trovata una riga dunque si azzera l'indice k */
                    k = 0; 
                }
                else 
                { 
                    k++; 
                }
            }

            /* Invio al padre del primo carattere dell'ultima linea */
            nw = write(pipe_fp[n][1], &c, 1); 

            /* Controllo scrittura */
            if (nw != 1)
            {
                printf("Errore: Il figlio di indice %d ha scritto al padre un numero di byte errato: %d\n", n, nw); 
                exit(-1);
            }

            /* Attesa del carattere di controllo da parte del padre */
            nr = read(pipe_pf[n][0], &ctrl, 1); 

            /* Controllo lettura */
            if (nr != 1)
            { 
                printf("Errore: Il figlio di indice %d non e' riuscito a ricevere il byte di controllo dal padre\n", n); 
                exit(-1); 
            }

            /* Pre-inizializzazione del valore di ritorno a 0; verra' aggiornato a 1 qualora si riceva l'indiczione di stampare */
            ritorno = 0;

            /* Valutazione del carattere di controllo ricevuto */
            if (ctrl == 'S')
            { 
                /* Aggiornamento del ritorno */
                ritorno = 1; 

                /* Si effettua la stampa come da richiesta nel testo */
                printf("Sono il processo figlio di indice %d e con pid = %d e ho individuato il minimo carattere '%c' tra tutti i figli nella linea: %s\n", n, getpid(), c, linea); 
            }
            /* Se il carattere di controllo e' diverso da 'S' non si stampa NULLA */

            exit(ritorno); 
        }
    }

    /* CODICE DEL PADRE */

    /* Il processo padre: 
        - Si comporta dal processo consumatore nei confronti di tutti i figli, dunque leggendo da TUTTE le pipe col verso figli-->padre 
        - Si comporta da produttore per l'altro schema di comunicazione sempre per tutti i figli, scrivendo dunque su tutte le pipe con verso padre-->figli 
       Alla luce di cio' e' possibile chiudere tutte i lati delle pipe non utilizzati */
    for (k = 0; k < N; k++)
    { 
        close(pipe_pf[k][0]); 
        close(pipe_fp[k][1]); 
    }   

    /* Si decide di ingorare il SIGPIPE per il processo padre, infatti qualora uno o piu' file di quelli associati ai figli non siano apribili, i rispettivi processi 
        verrebbero terminbati prima di ricevere il carattere di controllo del padre, dunque il padre tenterebbe la scrittura su una pipe senza lettore, ricevendo cosi'
        il SIGPIPE */
    signal(SIGPIPE, SIG_IGN); 

    /* Ciclo di recupero dei caratteri inviati dai figli con calcolo del valore minimo */
    c_min = CHAR_MAX;   /* Inizializzazione del minimo al massimo valore di char, affinche' possa essere sicuramente aggiornato da quelli inviati dai figli */
    for (n = 0; n < N; n++)
    { 
        nr = read(pipe_fp[n][0], &car, 1); 

        /* Controllo lettura */
        if (nr != 1)
        { 
            printf("Errore: Il processo padre non e' riuscito a leggere il carattere inviato dal figlio di indice %d\n", n); 
            /* exit(6)      Si decide di non terminare il padre, affinche' questo possa attendere tutti i figli */
        } 
        else 
        { 
            /* Calcolo del minimo ed eventuale salvataggio dell'indice del processo. Si noti che questa valutazione viene fatta solo se la lettura ha avuto successo 
                se no si rischia di effettuare il confronto con un carattere non inizializzato, ottenendo cosi' un risultato sfalsato */
            if (car < c_min)
            { 
                c_min = car; 
                idx_min = n;
            }
        }
        
    }

    /* Ciclo di invio dei caratteri di controllo */
    for (n = 0; n < N; n++)
    { 
        if (n == idx_min)
        { 
            ctrl = 'S'; 
        }
        else 
        { 
            ctrl = 'N'; 
        }

        write(pipe_pf[n][1], &ctrl, 1); 
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
            printf("Il processo figlio con pid = %d ha ritornato il valore %d (se 255 e' incorso in qualche errore durante l'esecuzione)\n", pid, ritorno); 
        }
    }
    exit(0); 
}
