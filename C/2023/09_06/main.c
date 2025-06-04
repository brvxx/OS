/* Programma C main.c che risolve la parte C della prova di esame totale tenutasi il 6 settembre 2023 */

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

/* Definizione del TIPO tipoS che rappresenta una particolare struttura che viene mandata tra i vari figli attraverso la pipeline per scambiarsi informazioni */
typedef struct { 
    long int len_min;   /* Numero minimo di linee calcolato dal processo figlio Pn */
    int idx_min;        /* Indice del processo che ha calcolato questo minimo */
} tipoS; 

int main (int argc, char **argv)
{
    /*---------------- Variabili locali -----------------*/
    int *pid;           /* Array dinamico di pid utilizzati nel padre per riconoscere il pid del figlio che ha identificato la lunghezza minima */ 
    int N;              /* Numero di file passati per parametro e dunque di processi figli creati */
    int n;              /* Indice d'ordine per i processi figli */
    int k;              /* Indice generico per i cicli */
    pipe_t *piped;      /* Array dinamico di pipe per implementare lo schema di comunicazione a pipeline tra i processi, dove: 
                            - Il processo P0 comunica col processo P1, che a sua volta comunica con P2 e via dicendo. Fino ad arrivare a PN-1 che comunica col padre 
                            - Il generico processo Pn (con n != 0) legge dunque dalla pipe di indice n - 1 e scrive su quella di indice n 
                            - Il processo P0 non effettua la lettura da nessuna pipe e si limita a scrivere sulla pipe di indice 0 */
    pipe_t *pipe_pf;    /* Array dinamico di pipe per il secondo schema di comunicazione tra processo padre e figli */
    tipoS S;            /* Struttura passata nella pipeline */
    int fdr;            /* File descriptor associato all'apertura dei file nei figli */
    char linea[250];    /* Buffer statico per la lettura delle righe nei vari figli */
    char c;             /* Variabile utilizzata dai figli per la lettura dei singoli caratteri da file */
    long int cur_tot;   /* Numero di linee calcolato dai processi figli per il proprio file */
    long int min;       /* Variabile richiesta dal testo per salvare il valore corrispondente al minimo di righe nei figli (inviato dal padre) */
    int status;         /* Variabile di stato per la wait */
    int ritorno;        /* Valore ritornato dai processi figli */
    int pidFiglio;      /* Identificatore dei processi attesi con la wait */
    int nr, nw;         /* Variabili di controllo per lettura/scrittura da/su pipe */
    /*---------------------------------------------------*/
    
    /* Controllo lasco numero di parametri */
    if (argc < 3)
    {
        printf("Errore: Numero di parametri passato a %s errato - Attesi almeno 2 [(program) filename1 filename2 ... ] - Passati: %d\n", argv[0], argc - 1); 
        exit(1); 
    }

    /* Calcolo del valore di N come numero di file passati */
    N = argc - 1; 

    /* Allocazione dinamica degli array di pipe per i due differenti schemi di comunicazione */
    piped = (pipe_t *)malloc(N * sizeof(pipe_t));
    pipe_pf = (pipe_t *)malloc(N * sizeof(pipe_t));  
    if ((piped ==  NULL) || (pipe_pf == NULL))
    { 
        printf("Errore: Allocazione dinamica degli array di pipe non riuscita\n"); 
        exit(2); 
    }

    /* Creazione delle pipe */
    for (k = 0; k < N; k++)
    { 
        if (pipe(piped[k]) < 0)
        { 
            printf("Errore: creazione della pipe di indice %d per la comunicazione tra padre e figli non riuscita\n", k); 
            exit(3); 
        }

        if (pipe(pipe_pf[k]) < 0)
        { 
            printf("Errore: creazione della pipe di indice %d per la comunicazione a pipeline \n", k); 
            exit(4); 
        }
    }

    /* Allocazione dinamica dell'array di pid */
    pid = (int *)malloc(N * sizeof(int)); 
    if (pid == NULL)
    {
        printf("Errore: Allocazione dinamica dell'array di pid non riuscita\n"); 
        exit(5); 
    }

    /* Ciclo di creazione dei processi figli */
    for (n = 0; n < N; n++)
    { 
        if ((pid[n] = fork()) < 0)
        { 
            printf("Errore: fork() di creazione del processo figlio di indice %d non riuscita\n", n); 
            exit(6); 
        }

        if (pid[n] == 0)
        { 
            /* CODICE DEI FIGLI */

            /* Si decide che in caso i processi figli incorrano in qualche tipo di errore, ritornino il valore -1 (255 senza segno), cosi' che la terminazione dovuta ad un errore, possa
                essere distinta da quella normale. Infatti i processi figli ritornano al padre il loro indice d'ordine che ragionevolmente si suppone essere minore di 255  */
            printf("DEBUG-Sono il processo figlio di indice %d e con pid = %d e sto per effettuare la lettura del file %s\n", n, getpid(), argv[n + 1]); 

            /* Il generico processo figlio Pn 
                - Per lo schema di comunicazione a pipeline legge dalla pipe di indice n - 1 e scrive su quella di indice n. Mentre il processo P0 si limita a scrivre sulla pipe 0
                - Per lo schema di comunicazione col padre, si comporta da produttore andando a scrivere sulla pipe di indice n 
               Alla luce di tali considerazioni e' dunque possibile chiudere tutti i lati delle pipe non utilizzati */
            for (k = 0; k < N; k++)
            { 
                close(pipe_pf[k][1]); 

                if ((k != n - 1) || (n == 0))
                { 
                    close(piped[k][0]); 
                }

                if (k != n)
                { 
                    close(piped[k][1]); 
                    close(pipe_pf[k][0]); 
                }
            }

            /* Apertura in lettura del file associato */
            if ((fdr = open(argv[n + 1], O_RDONLY)) < 0)
            { 
                printf("Errore: Apertura in lettura del file %s da parte del figlio %d non riuscita - File non leggibile oppure inesistente\n", argv[n + 1], n); 
                exit(-1); 
            }
            
            /* Inizializzazione del contatore di righe a 0 */
            cur_tot = 0L; 

            /* Ciclo di lettura del file per il conteggio delle righe totali */
            while (read(fdr, &c, 1) > 0)
            {
                if (c == '\n')
                {
                    cur_tot++; 
                }
            }

            /* Ripristino della posizione del file porinter affinche' nella lettura successiva si possa ripartire dall'inizio del file */
            lseek(fdr, 0L, SEEK_SET); 

            /* Ricezione della struttura inviata dal processo precedente nella pipeline. Si noti che il processo di indice 0 non deve effettuare questa lettura */
            if (n != 0)
            { 
                nr = read(piped[n - 1][0], &S, sizeof(S)); 

                /* Controllo che la lettura da pipeline abbia avuto successo */
                if (nr != sizeof(S))
                { 
                    printf("Errore: Il processo figlio di indice %d ha letto un numero errato di byte da pipeline: %d\n", n, nr); 
                    exit(-1); 
                }

                /* Controllo che il numero di righe calcolate dal processo corrente sia inferiore al valore riportato nella struttura */
                if (cur_tot < S.len_min)
                { 
                    S.len_min = cur_tot; 
                    S.idx_min = n; 
                }
            }
            else 
            { 
                /* Il processo P0 si limita ad inizializzare la struttura coi propri valori non dovendo effettuare alcun confronto */
                S.idx_min = n; 
                S.len_min = cur_tot; 
            }

            /* Invio della struttura in avanti nella pipeline al processo successivo */
            nw = write(piped[n][1], &S, sizeof(S)); 

            /* Controllo che la scrittura abbia avuto successo */
            if (nw != sizeof(S)) 
            { 
                printf("Errore: Il processo figlio di indice %d ha scritto un numero errato di byte sulla pipeline: %d\n", n, nw);  
                exit(-1);  
            }


            /* Ricezione del valore inviato dal padre */
            nr = read(pipe_pf[n][0], &min, sizeof(min)); 

            if (nr != sizeof(min))
            { 
                printf("Errore: Lettura della lunghezza minima da parte del processo figlio di indice %d non riuscita\n", n); 
                exit(-1); 
            }

            /* Seconda lettura del file carattere per carattere ma con salvataggio delle righe, per la ricerca della riga che abbia indice pari al valore inviato 
                Dal padre */
            k = 0;          /* Inizializzazione dell'indice del buffer a 0 */
            cur_tot = 0L;   /* Reset del valore di cur_tot affinche' possa essere riutilizzato per il conteggio delle righe */
            while (read(fdr, &(linea[k]), 1) > 0)
            {
                if (linea[k] == '\n')
                { 
                    /* Trovata una riga, dunque si incrementa il contatore di righe */
                    cur_tot++; 

                    if (cur_tot == min)
                    { 
                        /* Trovata la riga che corrisponde all'indice inviato dal padre, dunque si effettua la trasformazion di tale linea in stringa affinche' possa 
                            essere stampata correttamente con una printf */
                        linea[k] = '\0'; 

                        /* Stampa richiesta dal testo */
                        printf("Il processo di indice %d e con pid = %d ha trovato correttametne la linea di indice %ld (valore inviato dal padre) nel file %s e tale linea e': "
                                "%s\n", n, getpid(), min, argv[n + 1], linea); 
                        
                        /* A questo punto non e' piu' necessario proseguire con la lettura e si puo' dunque uscire dal ciclo */
                        break; 
                    }

                    /* Reset dell'indice per la riga successiva */
                    k = 0; 
                }
                else 
                { 
                    /* Se non ci si trova alla fine di una riga ci si limita ad incrementare l'indice del buffer */
                    k++; 
                }
            }

            /* Come da specifica del testo ogni figlio ritorna al padre il proprio indice d'ordine */
            exit (n); 
        }
    }

    /* CODICE DEL PADRE */

    /* Il processo padre: 
        - Per lo schema di comunicazione a pipeline si limita a leggere dalla pipe di indice N - 1 la struct risultante dalla comunicazione tra i vari figli 
        - Per il secondo schema di comunicazione si comporta da produttore per tutti i figli, andando dunque a scrivere sul lato di lettura di tutte le pipe dell'array pipe_pf 
       Alla luce di tali osservazioni si vanno dunque a chiudre tutti i lati delle pipe non utilizzati */
    for (k = 0; k < N; k++)
    { 
        close(pipe_pf[k][0]); 
        close(piped[k][1]); 

        if (k != N - 1)
        { 
            close(piped[k][0]); 
        }
    }       
    
    /* Lettura della struttura inviata da pipeline */
    nr = read(piped[N - 1][0], &S, sizeof(S)); 

    /* Controllo della lettura da pipeline */
    if (nr != sizeof(S))
    { 
        printf("Errore: Il processo padre ha letto un numero errato di byte dalla pipeline: %d\n", nr); 
        exit(7); 
    }
    else 
    { 
        /* Stampa richiesta dal testo */
        printf("Il processo figlio di indice %d e con pid = %d ha calcolato il numero minimo di righe: %ld per il suo file %s\n", S.idx_min, pid[S.idx_min], S.len_min, argv[S.idx_min + 1]); 

        /* Ciclo di invio a tutti i processi figli del valore minimo ricavato */
        for (n = 0; n < N; n++)
        { 
            nw = write(pipe_pf[n][1], &(S.len_min), sizeof(S.len_min)); 
            
            /* Controllo della scrittura */
            if (nw != sizeof(S.len_min))
            { 
                printf("Errore: Il processo padre ha scritto un numero errato di byte sulla pipe di indice %d di comunicazione coi figli: %d", n, nw); 
                /* exit(8)      Si decide di non terminare il processo padre affinche' il processo padre possa attendere i processi figli */
            }
        }
 
    }

    /* Ciclo di attesa dei processi figli con recupero e stampa del valore tornato */
    for (n = 0; n < N; n++)
    { 
        if ((pidFiglio = wait(&status)) < 0)
        { 
            printf("Errore: wait() di attesa di uno dei processi figli non riuscita\n"); 
            exit(9); 
        }

        if ((status & 0xFF) != 0)
        { 
            printf("Il processo figlio con pid = %d e' terminato in modo anomalo\n", pidFiglio); 
        }
        else 
        { 
            ritorno = (int)((status >> 8) & 0xFF); 
            printf("Il processo figlio con pid = %d ha ritornato il valore %d (se 255 e' incorso in qualche errore durante l'esecuzione)\n", pidFiglio, ritorno); 
        }
    }
    exit(0); 
}
