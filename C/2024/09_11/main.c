/* Programma C main.c che risolve la parte C della prova di esame totale tenutasi il 11 settembre 2024 */

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
    int pid;            /* Identificatore dei process figli creati con fork ed attesi con wait */
    int N;              /* Numero di file passati per parametro, e analogamente di processi figli creati */
    int n;              /* Indice dei processi figli */
    int k;              /* Indice generico per i cicli */
    int outfile;        /* File descriptro associato all'al file cerato dal padre "RISULTATO" */
    pipe_t *piped;      /* Array dinamico di pipe per la comunicazione figli->padre */
    pipe_t p;           /* Singola pipe per implementare la comunicazione tra ogni figlio e il rispetivo nipote creato */
    char linea[250];    /* Buffer statico di caratteri utilizzato dal padre per leggere le varie linee spedite dai figli. Si e' scelto
                            per il buffer dimensione di 250 caratteri perche' la specifica riporta che questa sia la lunghezza massima 
                            di caratteri (compreso il terminatore) delle varie linee ottenute con ls */
    int status;         /* Variabile di stato per la wait */
    int ritorno;        /* Valore ritornato dai processi figli */
    /*---------------------------------------------------*/
    
    /* Controllo lasco numero di parametri passati */
    if (argc < 4)
    { 
        printf("Errore: numero di parametri passato a %s errato - Attesi almeno 3 parametri [(program) dirass filename1 filename2 ... ] - Passati: %d\n", argv[0], argc - 1); 
        exit(1); 
    }

    /* Calcolo di N come numero di file passati */
    N = argc - 2;   

    /* Come da specifica il padre prima di tutto crea il file /tmp/RISULTATO se questo non esiste, oppure se gia' essite lo apre in append */
    if ((outfile = open("/tmp/RISULTATO", O_CREAT|O_WRONLY|O_APPEND, PERM)) < 0)
    { 
        printf("Errore: creazione/apertura del file /tmp/RISULTATO da parte del processo padre non riuscita\n"); 
        exit(2); 
    }

    /* Scrittura della linea corrispondente alla direcotry passata come primo parametro in una linea separata nel file. Si noti che dal momento che i file terminano sempre 
        con un carattere newline la scrittura, anche se il file gia' esisteva, avviene su una linea sepearata, poi alla fine della scrittura si dovra' immettere il carattere \n 
        per far si' che quelle successive siano separate dalla scrittura corrente */
    write(outfile, argv[1], strlen(argv[1])); 
    write(outfile, "\n", sizeof(char)); 

    /* Allocazione dinamica della memoria per l'array di N pipe. N che corrisponde al numero di file passati e dunque di figli creati, infatti si necessita di una pipe per ogni coppia 
        padre-figlio, in modo tale che il padre possa ricevere le linee stamapate dai figli secondo l'ordine dei file */
    piped = (pipe_t *)malloc(N * sizeof(pipe_t)); 
    if (piped == NULL)
    {
        printf("Errore: allocazione dinamica della memoria per l'array di pipe non riuscitan\n"); 
        exit(3); 
    }

    /* Ciclo di creazione delle pipe */
    for (k = 0; k < N; k++)
    { 
        if (pipe(piped[k]) < 0)
        { 
            printf("Errore: creazione della pipe di indice %d non riuscita\n", k); 
            exit(4);
        }
    }

    /* Ciclo di creazione dei processi figli */
    for (n = 0; n < N; n++)
    { 
        if ((pid = fork()) < 0) 
        { 
            printf("Errore: fork di crezione del figlio di indice %d non riuscita\n", n);
            exit(5);  
        }

        if (pid == 0)
        { 
            /* CODICE DEI FIGLI */

            /* Si decide che in caso i processi figli incorrano in qualche tipo di errore, ritornino il valore -1 (255 senza segno), cosi' che la terminazione dovuta ad un errore, possa
              essere distinta da quella normale, rappresentata tramite i valori di ritorno del grep, molto "lontani" da 255 */ 

            printf("DEBUG-Sono il processo figlio di indice %d e con pid = %d, associato al file %s e sto per creare un processo nipote\n", n, getpid(), argv[n + 2]); 

            /* Il generico processo figlio Pn si limita a scrivere sulla pipe di indice n di comunicazione col padre, dunque tutti gli altri lati delle pipe dello stesso array 
                possono essere chiusi */
            for (k = 0; k < N; k++)
            { 
                close(piped[k][0]); 
                
                if (k != n)
                { 
                    close(piped[k][1]); 
                }
            }

            /* Creazione della pipe di comunicazione col nipote */
            if (pipe(p) < 0)
            { 
                printf("Errore: creazione della pipe p di comunicazione tra il figlio di indice %d e il rispettivo nipote non riuscita\n", n); 
                exit(-1); 
            }

            /* Creazione del processo nipote */
            if ((pid = fork()) < 0)
            { 
                printf("Errore: fork di creazione del processo nipote da parte del figlio di indice %d non riuscita\n", n);
                exit(-1); 
            }

            if (pid == 0)
            { 
                /* CODICE DEI NIPOTI */

                /* Per omogeneita' con i processi figli si decide che in caso di errore anche i nipoti ritornino il valore -1, sebbene a tutti gli effetti il loro valore 
                    non verra' letto da nessun processo, dal momento che figli e nipoti eseguono concorrentemente i figli non andranno ad attendere questi ultimi */
                
                printf("DEBUG-Sono il processo nipote con indice %d e con pid = %d e sto per eseguire il comando ls -li sulla directory corrente\n", n, getpid()); 

                /* Chisura del lato di scrittura della pipe n lasciato aperto dal figlio assciato */
                close(piped[n][1]); 

                /* Implementazione del piping dei comandi in qualita' di primo comando nei confronti del processo figlio. Dunque viene chiuso lo stdout per essere poi ridiretto 
                    sul lato di scrittura della pipe p mediante una dup di questo file descriptor */
                close(1); 
                dup(p[1]);
                
                /* A questo punto e' possibile andare a chiudere entrambi i lati di p, dal momento che quello di lettura non viene utilizzato e il riferimento a quello di 
                    scrittura si trova al posto dello stdout */
                close(p[0]); 
                close(p[1]); 

                /* Esecuzione del comando ls -li, che permette di ricavare le informazioni di tutti i file/directory che si trovano nella directory corrente, assieme alle informazioni 
                    sull'i-number, come richiesto dalla specifica. Si utilizza la primitiva execlp dal momento che si va ad eseguire un comando di sistema, il cui percorso e' contenuto 
                    nella variabile d'ambiente PATH, utilizzata dalla versione dell'exec con 'p' */
                execlp("ls", "ls", "-li", (char *)0); 

                /* Se il nipote arriva ad eseguire questa parte di codice significa che l'exec e' fallita, dunque si va a notificare l'utente con una stampa sullo stderr, dal momento 
                    che lo stdout e' stato ridirezionato */
                perror("Problema con la execlp del ls da parte di uno dei nipoti"); 
                exit(-1); 
            }

            /* Ritorno al codice dei processi figli */

            /* Implementazione del piping dei comandi nei confronti del nipote, in qualita' di secondo comando, dunque si effettua una ridirezione dello stdin in modo che questo 
                punti al lato di lettura della pipe p */
            close(0); 
            dup(p[0]); 

            /* Chisura di entrambi i lati di p, per lo stesso motivo del nipote */
            close(p[0]); 
            close(p[1]); 

            /* Dal momento che il processo figlio deve comunque mandare la propria linea trovata al processo padre, e' dunque necesssario effettuare una ridirezione dello stdout 
                verso il lato di scrittura della pipe n di comunicazione col padre */
            close(1); 
            dup(piped[n][1]); 

            /* A questo punto dal momento che il riferimento al lato di scrittura della pipe e' stato copiato nel fd = 1 e' possibile chiudere il riferimento originario */
            close(piped[n][1]); 

            /* Esecuzione del comando grep per isolare la linea corrispondente al file associato al processo. Analogmamente per gli stessi ragionamenti fatti nel nipote viene 
                utilizzata la primitiva execlp */ 
            execlp("grep", "grep", argv[n + 2], (char *)0); 

            /* Analogmanete a quanto fatto per il nipote, se l'exec fallisce diamo una notifica dell'errore con la perror sullo stderr */
            perror("Problema con la execlp del grep da parte di uno dei figli"); 
            exit(-1); 
        }
    }

    /* CODICE DEL PADRE */

    /* Il processo padre si comporta da lettore per tutti i processi figli, alla luce di cio' puo' andare a chiduere i lati di scrittura di tutte le pipe */
    for (k = 0; k < N; k++)
    { 
        close(piped[k][1]); 
    }

    /* Ciclo di recupero delle linee spedite dai processi figli IN ORDINE dei file */
    for (n = 0; n < N; n++)
    { 
        /* Dal momento che il padre non conosce a priori la lunghezza delle righe spedite dei figli, si effettua un ciclo di lettura carattere per carattre da pipe 
            che si esauria' al raggiungimento della fine della linea con il \n */

        /* Inizializzazione a 0 dell'indice per la scrittura nel buffer di caratteri linea */
        k = 0; 
        while (read(piped[n][0], &(linea[k]), sizeof(char)) > 0)
        { 
            k++; 
        }

        /* Usciti dal ciclo k sarebbe l'indice del carattere successivo a \n, dunque implicitamente rappresenta la lunghezza in byte della linea compreso il carattere newline. 
            Grazie a questo e' ora possibile effettuare la stampa della riga sul file risultato. Si noti che la presenza del '\n' garantisce che ogni line stia sulla PROPRIA riga. 
            Si noti anche che dal momento che k e' inizializzato a 0, se l'esecuzione del grep NON trova match con il nome del file associato a Pn, e quindi non viene stampato nulla 
            sulla pipe, nel ciclo sopra NON si entra, ma allora k rimane 0 e questa scrittura non altera lo stato del file RISULTATO, in quanto scrittura di 0 byte */
        write(outfile, linea, k * sizeof(char)); 
    }
        
    /* Ciclo di attesa dei processi figli con recupero e stampa del valore tornato */
    for (n = 0; n < N; n++)
    { 
        if ((pid = wait(&status)) < 0)
        { 
            printf("Errore: wait di attesa di uno dei figli da parte del processo padre NON riuscita\n"); 
            exit(6); 
        }

        if (WIFEXITED(status))
        { 
            ritorno = WEXITSTATUS(status); 

            if (ritorno != 0)
            { 
                /* Caso in cui il grep ha riportato un errore o ci sono stati altri problemi nel figlio o nel nipote associato */
                printf("IL processo figlio con pid = %d ha ritornato il valore %d, dunque l'esecuzione del comando grep e' fallita, oppure se 255 il processo e' "
                        "incorso in qualche problema durante l'esecuzione\n", pid, ritorno); 
            }
            else
            { 
                 /* Caso di successo del comando grep */
                printf("Il processo figlio con pid = %d ha ritornato il valore %d, dunque il comando-filtro grep ha eseguito correttametne ed e' stato trovato il file\n", pid, ritorno); 
            }
        }
    }

    exit(0); 
}
