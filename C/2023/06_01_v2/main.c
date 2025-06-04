/* Programma C main.c che risolve la parte C della prova di esame tenutasi il 01 giugno 2023. Si tratta della versione 2 dal momento che questo esame 
    e' gia' stato risolto il precedenza, si tratta di una seconda versione */

#include <stdlib.h> 
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/wait.h> 
#include <string.h> 
#include <ctype.h>
#include <signal.h> 

/* Definizione del TIPO pipe_t che rappresenta un array di due interi, che useremo per la gestione dei lati delle varie pipe create */
typedef int pipe_t[2];

/* Definizione del TIPO Strut che rappresenta una particolare struttura utilizzata per contenere le informazioni della comunicazione tra processi */
typedef struct {
    int pidn;           /* Pid del processo nipote creato dal processo figli */
    char linea[250];    /* Ultima linea letta dal processo figlio da quelle mandate dal rispettivo nipote */
    int len;            /* Lunghezza dell'ultima riga letta dal figlio */
} Strut;

int main (int argc, char **argv)
{
    /*---------------- Variabili locali -----------------*/
    int pid;            /* Identificatore dei process figli creati con la fork e attesi con la wait */ 
    int N;              /* Numero di file passati per parametro e dunque di processi figli creati */
    int n;              /* Indice d'ordine per i processi figli */
    int k;              /* Indice generico per i cicli */
    pipe_t *piped;      /* Array dinamico di pipe per la comunicazione tra il processo padre e i figli */
    pipe_t p;           /* Singola pipe per la comunicazione tra i figli e i relativi nipoti */
    Strut S;            /* Struttura utilizzata dal padre e dai figli per lo scambio di comunicazioni */
    char buffer[250];   /* Buffer statico di cartteri per la lettura delle varie linee inviate dal nipote ai figli. Viene creato di lunghezza pari 
                            a 250 caratteri dal momento che si suppone che le linee dei file siano al massimo di tale dimensione */
    int status;         /* Variabile di stato per la wait */
    int ritorno;        /* Valore ritornato dai processi figli */
    int nr, nw;         /* Variabili di controllo per lettura/scrittura da/su pipe */
    /*---------------------------------------------------*/
    
    /* Controllo lasco numero di parametri passati */
    if (argc < 3)
    { 
        printf("Errore: Numero di parametri passati a %s errato - Attesi almeno 2 [(program) filename1 filename2 ... ] - Passati: %d\n", argv[0], argc - 1); 
        exit(1); 
    }
    
    /* Calcolo del valore di N come numero totale di file passati */
    N = argc - 1; 

    /* Allocazione dinamica dell'array di pipe */
    piped = (pipe_t *)malloc(N * sizeof(pipe_t)); 
    if (piped == NULL)
    { 
        printf("Errore: Allocazione dinamica della memoria per l'array di pipe non riuscita\n");
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
                essere distinta da quella normale. Infatti nel caso normale i processi figli ritornano al padre la lunghezza dell'utlima linea, che per specifica del testo risulta essere 
                minore di 250 caratteri, dunque diversa da 255 */
            printf("DEBUG-Sono il processo figlio di indice %d e con pid = %d e sto per generare un processo nipote che esegua il rev del file "
                    " %s a me associato\n", n, getpid(), argv[n + 1]); 

            /* Il generico processo figlio Pn si comporta da produttore nei confronti del padre, andando dunque a scrivere sul lato di lettura della pipe di 
                indice n. Alla luce di cio' e' possibile andare a chiudere tutti i lati delle pipe non utilizzati da questo processo */
            for (k = 0; k < N; k++)
            { 
                close(piped[k][0]); 

                if (k != n)
                { 
                    close(piped[k][1]); 
                }
            }

            /* Creazione delle pipe per la comunicazione col nipote */
            if (pipe(p) < 0)
            {
                printf("Errore: Creazione della pipe p di comunicazione col nipote di indice %d non riuscita\n", n); 
                exit(-1); 
            }

            /* Creazione del processo nipote */
            if ((pid = fork()) < 0)
            { 
                printf("Errore: fork di creazione del processo nipote di indice %d non riuscitan\n", n); 
                exit(-1); 
            }

            if (pid == 0)
            { 
                /* CODICE DEI NIPOTI */

                /* Per lo stesso ragionamento fatto per i processi figli, si decide che i nipoti ritornino il valore -1, differente dal normale valore 
                    di ritorno del comando rev */

                printf("DEBUG-Sono il processo nipote di indice %d e con pid = %d e sto per eseguire il comando rev sul file %s\n", n, getpid(), argv[n + 1]); 


                /* Chiusura del lato di scrittura della pipe n lasciato aperto dal figlio, dal momento che non verra' utilizzato dal nipote */
                close(piped[n][1]); 

                /* Implementazione del piping dei comandi in qualita' di primo comando (rev) nei confronti del processo figlio associato */
                close(1); +
                dup(p[1]); 

                /* A questo punto e' possibile chiudere entrambi i lati della pipe p, infatti: 
                    - p[0] non viene utilizzato dal nipote, infatti si comporta da consumatore nei confronti del padre 
                    - p[1] e' stato copiato come stdout tramite l'utilizzo della dup() */
                close(p[0]); 
                close(p[1]); 

                /* Esecuzione del comando rev da parte del processo nipote mediante la primitiva execlp. Si noti che si utilizza la versione della primitiva 
                    con la 'p' perche' il processo nipote va ad eseguire un comando di sistema, dunque il cui percorso e' riportato dentro la variabile di 
                    ambiente PATH */
                execlp("rev", "rev", argv[n + 1], (char *)0); 

                /* Se si arriva ad eseguire questa parte di codice significa che l'esecuzione del comando rev e' fallita e dunque per corretteza si riporta 
                    all'utente una stampa di errore mediante la perror, infatti non e' possibile utilizzare la printf avendo ridirezionato lo stdout */
                perror("Problemi con l'esecuzione del comando rev da parte di uno dei nipoti"); 
                exit(-1);
            }

            /* Ritorno al codice dei processi figli */

            /* Chiusura del lato di scrittura di p, dal momento che il figlio si comporta solamente da consumatore nei confronti del nipote */
            close(p[1]); 

            /* Inzializzazione del campo pidn della struct da comunicare al padre col pid del nipote creato */
            S.pidn = pid; 

            /* Si inizializza anche il valore della lunghezza dell'ultima riga a 0, spiegato dopo il motivo */
            S.len = 0; 

            /* Ciclo di lettura delle righe inviate dal nipote sulla pipe p */
            k = 0;      /* Inizializzazione a 0 dell'indice per il buffer */
            while (read(p[0], &(buffer[k]), 1) > 0)
            { 
                if (buffer[k] == '\n')
                { 
                    /* Trovata una riga, dunque si va a salvare la lunghezza di tale linea dentro il campo len della struttura S, cosi' una 
                        volta che si esce dal ciclo nella struttura sara' salvata la lunghezza dell'ultima linea  */
                    S.len = k + 1; 

                    /* Reset dell'indice k per la lettura della riga successiva */
                    k = 0; 
                }
                else 
                { 
                    k++; 
                }
            }

            /* A questo punto si effettua la scrittura della struttura al padre se e solo se effetivamente e' stata letta qualche riga dal nipote
                e per verificare tale cosa bastera' controllare che sia stata agiornata la variabile S.len, che inizalmente e' stata inizializzata 
                a 0 per questo motivo */
            if (S.len != 0)
            {    
                /* Copia dell'ultima riga letta dentro il buffer della struttura da passare al padre. Per effettuare questa azione viene utillizata la 
                    funzione memcpy */
                memcpy(S.linea, buffer, S.len); 

                /* Invio della struttura ora totalmente inizializzata al processo padre */
                nw = write(piped[n][1], &S, sizeof(S)); 

                /* Controllo della scrittura */
                if (nw != sizeof(S))
                {
                    printf("Errore: Il processo figlio di indice %d ha scritto un numero errato di byte sulla pipe di comunicazione col padre\n", n);
                    exit(-1);
                }
            }

            

            /* Sebbene il valore tornato dai nipote non serva ai figli, per correttezza si ritorna si attende il processo nipote creato */
            if ((pid = wait(&status)) < 0)
            { 
                printf("Errore: wait di attesa del nipote di indice %d non riuscita\n", n); 
                exit(-1); 
            }

            if ((status & 0xFF) != 0)
            { 
                printf("Il processo nipote di indice %d e con pid = %d e' terminato in modo anomalo\n", n, pid);
                exit(-1); 
            }
            else 
            { 
                printf("DEBUG-Il processo nipote di indice %d e con pid = %d che ha eseguito il rev sul file %s ha ritornato "
                        "il valore %d\n", n, pid, argv[n + 1], WEXITSTATUS(status)); 

                /* Inizializzazione del valore di ritorno, che se il nipote ha eseguito correttamente, si va ad assegnare la lunghezza dell'ultima riga 
                    letta, invece qualora il nipote avesse incontrato qualche tipo di errore si ritorna -1, infatti il valore contenuto in S.len sarebbe 
                    sicuramente un valore non inizializzato */
                if (WEXITSTATUS(status) == 0)
                { 
                    /* Come da specifica si ritorna al padre la lunghezza dell'ultima riga ma SENZA il terminatore di linea */
                    ritorno = S.len - 1; 
                }
                else
                { 
                    ritorno = -1; 
                }
            }

            exit(ritorno);
        }
    }

    /* CODICE DEL PADRE */

    /* Il processo padre si comporta da consumatore per tutti i processi figli, andando dunque a leggere da tutti i lati di lettura delle pipe. Alla luce 
        di tale osservazione e' dunque possibile chiudere tutti i lati di scrittura delle pipe */
    for (k = 0; k < N; k++)
    { 
        close(piped[k][1]);
    }

    /* Ciclo di recupero delle strutture inviate dai figli secondo l'ordine dei file */
    for (n = 0; n < N; n++)
    { 
        nr = read(piped[n][0], &S, sizeof(S)); 

        /* Si effettua la stampa solo ed esclusivamente se la lettura ha avuto successo e non abbia dunque tornato 0 */
        if (nr == sizeof(S))
        {
            /* Trasformazione dell'ultima linea in una stringa, mantenendo come da specifica che questa sia una linea */
            S.linea[S.len] = '\0'; 

            /* Stampa richiesta dal testo. Si noti che non viene messo il \n alla fine dal momento che la riga stampata necessariamente conterra' 
                questo carattere alla fine  */
            printf("Il processo nipote di indice %d e con pid = %d ha invertito l'ultima riga del file %s lunga %d caratteri, "
                    "e la linea ottenuta e': %s", n, S.pidn, argv[n + 1], S.len, S.linea);
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
