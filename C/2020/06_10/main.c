/* Programma C main.c che risolve la parte C della prova di esame */

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
    pipe_t *piped;      /* Array dinamico di pipe che permettono di implementare uno schema di comunicazione tra i processi nel verso figli-->padre */
    int fcreato;        /* File descriptor associato al file creato dal padre di nome Camilla */
    int fdr;            /* File descriptor associato all'apertura dei file associati ai vari figli */
    char linea[250];    /* Buffer statico di caratteri per la lettura delle righe nei figli */
    char car;           /* Variabile utilizzata dai vari figli per salvare i primi caratteri delle righe */
    int inviate;        /* Contatore di righe inviate al padre per i vari processi figli */
    int status;         /* Variabile di stato per la wait */
    int ritorno;        /* Valore ritornato dai processi figli */
    /*---------------------------------------------------*/
    
    /* Controllo lasco numero di parametri passati */
    if (argc < 3)
    { 
        printf("Errore: Numero di parametri passato a %s errato - Attesi almeno 2 parametri [(program) filename1 filename2 ... ] - Passati: %d\n", argv[0], argc - 1); 
        exit(1);
    }

    /* Calcolo del valore di Q come numero di file passati */
    Q = argc - 1; 

    /* Creazione del file Camilla da parte del padre */
    if ((fcreato = creat("Camilla", PERM)) < 0)
    { 
        printf("Errore: Creazione del file camilla da parte del padre non riuscita\n"); 
        exit(2); 
    }

    /* Allocazione dinamica della memroia per l'array di pipe */
    piped = (pipe_t *)malloc(Q * sizeof(pipe_t)); 
    if (piped == NULL)
    { 
        printf("Errore: Allocazione dinamica della memoria per l'array di pipe non riuscita\n"); 
        exit(3); 
    }
    
    /* Ciclo di creazione delle pipe */
    for (k = 0; k < Q; k++)
    { 
        if (pipe(piped[k]) < 0)
        { 
            printf("Errore: Creazione della pipe di indice %d non riuscita\n", k); 
            exit(4);
        }
    }

    /* Ciclo di creazione dei processi figli */
    for (q = 0; q < Q; q++)
    { 
        if ((pid = fork()) < 0)
        { 
            printf("Errore: fork() di creazione del processo figlio di indice %d non riuscita\n", q); 
            exit(5); 
        }

        if (pid == 0)
        { 
            /* CODICE DEI FIGLI */

            /* Si decide che in caso i processi figli incorrano in qualche tipo di errore, ritornino il valore -1 (255 senza segno), cosi' che la terminazione dovuta ad un errore, possa
                essere distinta da quella normale. Infatti nel caso normale i processi figli ritornano il numero di righe inviate al padre, che per specifica risulta essere strettamente 
                minore di 255 */
            printf("DEBUG-Sono il processo figlio di indice %d e con pid = %d e sto per effettuare la lettura del file %s\n", q, getpid(), argv[q + 1]); 

            /* Il generico processo figlio Pq si comporta da produttore nei confronti del padre, andndo a scrivere sul lato di scrittura della pipe di indice q. Alla luce di tale 
                considerazione e' dunque possibile chiudere tutti i lati delle altre pipe non utilizzate per la comunicazione */
            for (k = 0; k < Q; k++)
            { 
                close(piped[k][0]); 
                
                if (k != q)
                {
                    close(piped[k][1]); 
                }
            }

            /* Apertura del file associato al figlio */
            if ((fdr = open(argv[q + 1], O_RDONLY)) < 0)
            { 
                printf("Errore: Apertura in lettura del file %s da parte del figlio di indice %d non riuscita - File non leggibile oppure inesistente\n", argv[q + 1], q); 
                exit(-1); 
            }

            /* Ciclo di lettura delle righe del file, carattere per carattere */
            k = 0;      /* Inizializzazione dell'indice per il buffer linea a 0 */
            while (read(fdr, &(linea[k]), 1) > 0)
            { 
                if (linea[k] == '\n')
                { 
                    /* Trovata riga del file, dunque si va ad incrementare il valore k di 1, affinche' possa rappresentare la lunghezza di tale linea, tenendo in conto anche 
                        del terminatore \n */
                    k++; 

                    /* Verifica validita' della riga controllando se: 
                        - Il primo carattere di questa sia numerico 
                        - La sua lunghezza sia strettamente minore di 10 (compreso il terminatore) */
                    if (isdigit(car) && k < 10)
                    { 
                        /* Linea valida, dunque si procede all'invio al padre */
                        write(piped[q][1], linea, k); 

                        /* Incremento del contatore di linea inviate */
                        inviate++; 
                    }

                    /* Dal momento che si deve iniziare una nuova riga si resetta a 0 il valore dell'indice k */
                    k = 0; 
                }
                else 
                { 
                    /* Caso in cui non ci si trovi al termine di una linea */
                    if (k == 0)
                    { 
                        /* Salvataggio primo carattere della linea */
                        car = linea[k]; 
                    }

                    k++; 
                }
            }

            exit(inviate); 
        }
    }

    /* CODICE DEL PADRE */

    /* Il processo padre si comporta da consumatore nei confronti di tutti i processi figli, andando dunque a leggere dai lati di lettura di tutte le pipe per la comunicazione. 
        Alla luce di tale osservazione e' possibile andare a chiduere tutti i lati delle pipe che non vengano utilizzati dal padre */
    for (k = 0; k < Q; k++)
    { 
        close(piped[k][1]); 
    }       
    
    /* Ciclo di recupero delle linee inviate dai figli */
    for (q = 0; q < Q; q++)
    { 
        printf("DEBUG-Lettura delle righe inviate dal processo di indice q = %d\n", q); 

        /* Ciclo di lettura delle righe da pipe carattere per carattere */
        k = 0;  /* Inizializzazione a 0 dell'indice per il buffer */
        while (read(piped[q][0], &(linea[k]), 1) > 0)
        { 
            if (linea[k] == '\n')
            { 
                /* Raggiunta fine della liena, dunque si trasforma la linea in stringa (come da richiesta del testo) andando ad aggiungere il terminatore \0 in seguito al carattere di newline */
                linea[k + 1] = '\0'; 

                printf("Il processo figlio di indice %d ha letto dal file %s la seguente linea: %s\n", q, argv[q + 1], linea); 

                /* Sebbene non sia specificato nel testo, nella soluzione della prof viene riportata la necesstita' di fare una stampa della riga (ovviamente senza il terminatore \0)
                    sul file Camilla creato dal padre */
                write(fcreato, linea, k + 1); 

                /* Reset dell'indice k per la linea successiva */
                k = 0; 
            }
            else 
            { 
                k++;  
            }
        }
    }

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
