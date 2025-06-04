/* Programma C main.c che risolve la parte C della prova di esame tenutasi il 30 maggio 2024. Si tratta della versione v2 dal momento che e' un esame gia' risolto in passato */

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
    int N;              /* Numero passato come primo parametro */
    int n;              /* Indice d'ordine per i processi figli */
    int k;              /* Indice generico per i cicli */
    pipe_t *piped;      /* Array dinamico di pipe per la comunicazione tra processo padre e processi figli */
    pipe_t p;           /* Singola pipe per la comunicaizone tra un figlio e il rispettivo nipote generato */
    int outfile;        /* File descriptor relativo alla creazione del file nel padre */
    char buffer[250];   /* Buffer statico di caratteri per la lettura delle linee inviate dai processi figli al padre */
    char pidn[11];      /* Buffer statico di caratteri utilizzato dai processi figli per trasformare il pid dei nipoti in una stringa */
    int status;         /* Variabile di stato per la wait */
    int ritorno;        /* Valore ritornato dai processi figli */
    /*---------------------------------------------------*/
    
    /* Controllo lasco numero di parametri passati */
    if (argc != 3)
    { 
        printf("Errore: Numero di parametri passati a %s errato - Attesi esattamente 2 [(program) numpos filename ] - Passati: %d\n", argv[0], argc - 1); 
        exit(1);
    }

    /* Salvataggio del parametro N con rispettivo controllo */
    N = atoi(argv[1]); 
    if (N <= 0)
    { 
        printf("Errore: il parametro %s non rappresenta un numero intero strettametne positivo\n", argv[1]); 
        exit(2); 
    }

    /* Creazione del file da parte del padre */
    if ((outfile = open(argv[2], O_CREAT|O_WRONLY|O_TRUNC, PERM)) < 0)
    { 
        printf("Errore: Creazione del file %s da parte del padre non riuscita\n", argv[2]); 
        exit(3); 
    }

    /* Allocazione dinamica dell'array di pipe */
    piped = (pipe_t *)malloc(N * sizeof(pipe_t)); 
    if (piped == NULL)
    {
        printf("Errore: Allocazione dinamica della memoria per l'array di pipe non riuscita\n"); 
        exit(4); 
    }

    /* Ciclo di creazione delle pipe */
    for (k = 0; k < N; k++)
    { 
        if (pipe(piped[k]) < 0)
        { 
            printf("Errore: Creazione della pipe di indice %d non riuscita\n", k); 
            exit(5); 
        }
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
                essere distinta da quella normale. Infatti nel caso normale, questi ritornano ovviametne il valore di ritorno del grep, che sono valori bassi, dunque distinguibili dal 255 */
            printf("DEBUG-Sono il processo figlio di indice %d e con pid = %d e sto per creare un processo nipote\n", n, getpid()); 

            /* Il generico processo figlio Pn si comporta da produttore nei confronti del processo padre, andando a scrivere sul lato di scrittura della pipe di indice n dell'array. ALla luce di
                tale considerazione sara' possibile chiudere tutti i lati delle pipe non utilizzati */
            for (k = 0; k < N; k++)
            { 
                close(piped[k][0]); 

                if (k != n)
                { 
                    close(piped[k][1]); 
                }
            }

            /* Creazione della pipe p per la comunicazione col processo nipote che si sta per creare */
            if (pipe(p) < 0)
            {
                printf("Errore: Creazione della pipe p da parte del figlio di indice %d non riuscita\n", n); 
                exit(-1); 
            }

            /* Creazione del processo nipote */
            if ((pid = fork()) < 0)
            { 
                printf("Errore: fork di creazione del processo nipote di indice %d non riuscita\n", n); 
                exit(-1); 
            }

            if (pid == 0)
            { 
                /* CODICE DEI NIPOTI */

                /* Sebbene il valore di ritorno dei nipoti non venga recuperato da nessun processo, si decide per omogeneita' coi figli di tornare il valore -1 in caso di errore */
                printf("DEBUG-Sono il processo nipote di indice %d e con pid = %d e sto per eseguire il comando ps\n", n, getpid()); 

                /* Chisura del lato di scrittura della pipe di indice n lasciato aperto dal figlio per la comunicaizone col padre */
                close(piped[n][1]); 

                /* Implementazione del piping dei comandi in qualita' di primo comando nei confronti del processo figlio associato */
                close(1); 
                dup(p[1]); 

                /* A questo punto e' possibile chiudere entrambi i lati di p, dal momento che: 
                    - p[0] non viene utilizzata dal nipote perche' si comporta come produttore nei confronti del figlio 
                    - p[1] e' stato copiato al posto dello stdout */
                close(p[0]); 
                close(p[1]); 

                /* Esecuzione del comando ps senza alcuna opzione aggiuntiva. Si noti che dal momento che il ps e' un comando di sistema, conviene utilizzare la funzione execlp, dunque la versione 
                    con la 'p' che permette la ricerca del comando dentro la variabile di ambiente PATH */
                execlp("ps", "ps", (char *)0); 

                /* Se si arriva ad eseguire questa parte di codice signfica che il processo nipote ha avuto problemi con l'esecuzione del comando */
                perror("problemi con l'esecuzione del comando ps da parte di uno dei nipoti"); 
                exit(-1); 
            }

            /* Ritorno al codice dei figli */ 

            /* Trasformazione del pid del processo nipote creato in una stringa, cosi' da poter essere passata nell'ecexlp */
            sprintf(pidn, "%d", pid); 

            /* Implementazione del piping dei comandi in qualita' di secondo comando nei confronti del processo nipote PPn */
            close(0); 
            dup(p[0]); 

            /* Analogamente per lo stesso ragionamento fatto nel nipote e' possibile chiudere entrambi i lati della pipe p */
            close(p[0]); 
            close(p[1]); 

            /* Implementazione della ridirezione dello stdout, per far si' che la linea trovata dal processo figlio venga scritta sulla pipe di comunicazione col padre 
                e non a video */
            close(1); 
            dup(piped[n][1]); 

            /* Avendo ora copiato il riferimento del lato di scrittura di questa pipe nello stdout, e' possiible chiuderlo */
            close(piped[n][1]); 

            /* Esecuzione del comando grep in piping col ps eseguito dal nipote, per cercare appunto la riga corrispondente al processo nipote stesso. Analogamente a quanto fatto 
                per il nipote, dal momento che il grep e' sempre un comando di sistema, si usera' la primitiva execlp */
            execlp("grep", "grep", pidn, (char *)0); 

            /* Se si arriva ad eseguire questa parte di codice allora l'esecuzione del grep e' fallita, dunque si riporta su stder una stampa di errore */
            perror("Problemi con l'esecuzione del comando grep da parte di uno dei figli"); 
            exit(-1); 

            /* Si noti che no si effettua l'attesa del processo nipote creato, dal momento che la specifica richiede che ogni figlio esegua CONCORRENTEMENTE al proprio nipote */
        }
    }

    /* CODICE DEL PADRE */

    /* Il processo padre si comporta da consumatore per TUTTI i processi figli, dunque leggendo dal lato di lettura di tutte le N pipe, alla luce di cio' e' dunque possibile 
        andare a chiudere tutti i lati di scrittura delle pipe */
    for (k = 0; k < N; k++)
    {
        close(piped[k][1]); 
    }

    /* Ciclo di ricezione delle linee inviate dai figli secondo l'ordine dei file */
    for (n = 0; n < N; n++)
    {   
        /* Inizializzazione a 0 dell'indice per il buffer di lettura */
        k = 0; 
        while(read(piped[n][0], &(buffer[k]), 1) > 0)
        {
            k++; 
        }

        /* Scrittura della linea inviata dal processo Pn sul file creato */
        write(outfile, buffer, k); 

        /* Si noti che viene utilizzato il valore di k, perche' al termine del ciclo di lettura questo valore rappresenta implicitamente la lunghezza della riga letta 
            contando anche il carattere newline */
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

            if (ritorno != 0)
            { 
                printf("Il processo figlio con pid = %d ha ritornato il valore %d, dunque il grep non e' riuscito ad eseguire correttamente, oppure se il valore tornato "
                        "e' 255, il processo figlio e' incorso in qualche altro tipo di problema\n", pid, ritorno); 
            } 
            else
            { 
                printf("Il processo figlio con pid = %d ha ritornato il valore %d, dunque il grep ha trovato con successo la riga del nipote\n", pid, ritorno);
            }
        }
    }
    exit(0); 
}
