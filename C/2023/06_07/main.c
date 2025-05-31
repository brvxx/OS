/* Programma C main.c che risolve la parte C della prova di esame totale tenutasi il 7 giugno 2023 */

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

/*---------------- Variabili globali -----------------*/
/* Variabili con scope globale, affinche' possano essere utilizzate sia nel main che nella funzione finitof() */
int N;              /* Numero di figli passati per parametro, e dunque numero di process figli creati */
int *finito;        /* Array dinamico di N interi la cui semantica prevede che: 
                        - se l'elemento n vale 0, allora il processo figlio di indice n non e' ancora terminato 
                        - se l'elemento n vale 1, allora il prcesso figlio di indice n e' terminato 
                       Grazie al contenuto di questo array e la funzione finitof sara' possibile decretare quando terminare il ciclo 
                       di recupero dei messaggi inviati da pipe nel padre */
/*---------------------------------------------------*/

/* Definizione della funzione che mediante il controllo dell'array finito ritorna: 
    - 1 se tutti i processi figli sono terminati, dunque tutti gli elementi di finito sono uguali a 1 
    - 0 se esiste ancora almeno 1 figlio non terminato */
int finitof()
{ 
    int i;      /* Variabile indice per il ciclo */
    for (i = 0; i < N; i++)
    { 
        if (finito[i] == 0)
        {
            return 0; 
        }
    }
    /* Se tutti gli elementi di finito sono 1 allora si puo' finire il ciclo di recupero */
    return 1; 
}

int main (int argc, char **argv)
{
    /*---------------- Variabili locali -----------------*/
    int pid;            /* Identificatore dei processi figli creati con fork e attesi con wait */
    int n;              /* Indice dei processi figli */
    int k;              /* Indice generico per i cicli */
    char linea[250];    /* Buffer di caratteri per la memorizzazione della linea corrente nei processi figli. Sebbene nel testo non venga riportata alcuna 
                            indicazione sulla lunghezza massima delle righe e' ragionevole pensare che la lunghezza di queste non superi i 250 caratteri 
                            (compreso il terminatore '\n')*/
    pipe_t *pipe_fp;    /* Array dinamico di pipe per la comunicazione tra processi nel verso figli --> padre */
    pipe_t *pipe_pf;    /* Array dinamico di pipe per la comunicazione tra processi nel verso padre --> figli */
    int fdr;            /* File descriptor associato all'apertura dei file nei vari figli */
    int stampe;         /* Contatore di stampe effettuate da ogni processo figlio sullo stdout */
    char ctrl;          /* Carattere di controllo per la stampa dei figli, si decide che se: 
                            - ctrl == 'S' allora il figlio debba procedere con la scrittura 
                            - ctrl == 'N' allora il figlio NON debbe procedere con la scrittura */
    char car;           /* Carattere utilizzato dal padre per leggere via via quelli inviati dai figli */
    char car_max;       /* Carattere massimo trovato dal padre per ogni riga */
    int idx_max;        /* Indice del processo che per l'insieme corrente di caratteri ha trovato il carattere dal valore massimo */
    int status;         /* Variabile di stato per la wait */
    int ritorno;        /* Valore ritornato dai processi figli */
    int nr, nw;         /* Variabili di controllo per lettura/scrittura da/su pipe */
    /*---------------------------------------------------*/
    
    /* Controllo lasco del numero di parametri */
    if (argc < 3)
    { 
        printf("Errore: numero di parametri passati a %s errato - Attesi almeno 2 parametri [(program) filename1 filename2 ... ] - Passati: %d\n", argv[0], argc - 1); 
        exit(1); 
    }

    /* Calcolo del valore di N come numero totale di file passati */
    N = argc - 1; 

    /* Allocazione dinamica della memoria per gli array di pipe. Dal momento si deve implementare la comunicaizone sia nel verso padre->figli che figli->padre, che le pipe sono
        unidirezionali e che in totale si avranno N processi figli, sono necessari DUE array di N pipe ciascuno, uno per la comunicazione in un verso, l'altro array per l'altro veso */
    pipe_fp = (pipe_t *)malloc(N * sizeof(pipe_t)); 
    pipe_pf = (pipe_t *)malloc(N * sizeof(pipe_t)); 
    if ((pipe_fp == NULL) || (pipe_pf == NULL))
    {
        printf("Errore: allocazione dinmaica della memoria per gli array di pipe non riuscita\n");
        exit(2); 
    }

    /* Ciclo di creazione delle pipe */
    for (k = 0; k < N; k++)
    {
        if (pipe(pipe_fp[k]) < 0)
        { 
            printf("Errore: creazione della pipe di indice %d per la counicazione nel verso figli->padre non riuscita\n", k); 
            exit(3); 
        }

         if (pipe(pipe_pf[k]) < 0)
        { 
            printf("Errore: creazione della pipe di indice %d per la counicazione nel verso padre->figli non riuscita\n", k); 
            exit(4); 
        }
    }

    /* Allocazione dinamica della memoria per l'array finito */
    finito = (int *)malloc(N * sizeof(int)); 
    if (finito == NULL)
    { 
        printf("Errore: allocazione dinamica della memoria per l'array finito non riuscita\n"); 
        exit(5); 
    }

    /* Inizializzazione dell'array a tutti 0, ipotizzando dunque che inizialmente tutti i figli siano vivi */
    memset(finito, 0, N * sizeof(int)); 

    /* Ciclo di creazione de figli */
    for (n = 0; n < N; n++)
    { 
        if ((pid = fork()) < 0)
        { 
            printf("Errore: fork di creazione del processo figlio di indice %d non riuscita\n", n); 
            exit(6); 
        }

        if (pid == 0)
        { 
            /* CODICE DEI FIGLI */
            
            /* Si decide che in caso i processi figli incorrano in qualche tipo di errore, ritornino il valore -1 (255 senza segno), cosi' che la terminazione dovuta ad un errore, possa
                essere distinta da quella normale che per specifica risulta essere strettamente minore di 255 */
            
            printf("DEBUG-Sono il processo figlio di indice %d e con pid = %d e sto per leggere dal file %s\n", n, getpid(), argv[n + 1]); 

            /* Il generico processo figlio Pn: 
                - Si comporta da produttore nei confronti del padre, andando dunaue a scrivere sulla pipe di indice n di pipe_fp 
                - Si comporta da consumatore nei confronti dello stesso padre, andando a riceveere il carattere di controllo dalla pipe di indice n di pipe_pf 
               Alla luce di cio' e' dunque possibile chiudere tutti i lati delle pipe non utilizzati */
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
                printf("Errore: apertura in lettura del file %s da parte del figlio di indice %d non riuscita - file non leggibile oppure inesistente\n", argv[n + 1], n); 
                exit(-1); 
            }

            /* Inizializzazione dell'indice del buffer e del numero di scritture effettuate a 0 */
            k = 0;
            stampe = 0; 

            /* Ciclo di lettura del file carattere per carattere con salvataggio delle righe dentro il buffer linea */
            while (read(fdr, &(linea[k]), sizeof(char)) > 0)
            { 
                if (linea[k] == '\n')
                { 
                    /* Trovata una riga, dunque il processo figlio invia al padre l'ultimo carattere letto prima del terminatore */
                    nw = write(pipe_fp[n][1], &(linea[k - 1]), sizeof(char)); 

                    /* Controllo che la scrittura abbia avuto succeso */
                    if (nw != sizeof(char))
                    { 
                        printf("Errore: scrittura di un carattere su pipe da parte del figlio di indice %d non riuscita\n", n); 
                        exit(-1); 
                    }

                    /* Recupero del carattere di controllo inviato dal padre */
                    nr = read(pipe_pf[n][0], &ctrl, sizeof(char)); 

                    /* Controllo sulla lettura */
                    if (nr != sizeof(char))
                    {
                        printf("Errore: lettura da pipe del carattere di controllo da parte del figlio di indice %d non riuscita\n", n); 
                        exit(-1); 
                    }

                    /* Stampa di quanto richiesto dal testo se e solo se il carattere di controllo vale 'S' */
                    if (ctrl == 'S')
                    { 
                        /* Trasformazione della linea corrente in una stringa affiche' possa essere stampata con la printf. Si noti che il terminatore di riga viene mantenuto */
                        linea[k + 1] = '\0'; 

                        printf("Il processo figlio di indice %d e con pid = %d ha identificato per la riga corrente il carattere con valore massimo rispetto agli altri figli "
                                "ossia il carattere '%c'.\nLa linea corrente trovata e': %s", n, getpid(), linea[k - 1], linea);    

                        /* Si noti che la stampa non termina con il \n peche' avendolo mantenuto per la riga (che viene stampata alla fine), si andra' comunque a capo */

                        /* Incremento del contatore delle scritture effettuate */
                        stampe++; 
                    }

                    /* Se il carattere di controllo e' diverso da 'S' si continua tranquillamente la lettura, azzerando l'indice k per la prossima riga */
                    k = 0;
                }
                else 
                { 
                    /* Se non ci si trova sul \n allora ci si limita ad incrementare l'indice per la scrittura sul buffer */
                    k++; 
                }
            }

            exit(stampe); 
        }
    }

    /* CODICE DEL PADRE */

    /* Il processo padre: 
        - Si comporta da consumatore nei confronti di TUTTI i figli, andando dunque a leggere dai lati di lettura di tutte le pipe 
        - Si comporta da prdoduttore nei confronti di TUTTI i figli, anndano dunque a scrivere il carattere di controllo sul lato di scrittura di tutte le pipe 
       Alla luce di queste considerazioni e' possibile chiudere tutti i lati delle pipe non utilizzati dal padre */
    for (k = 0; k < N; k++)
    { 
        close(pipe_fp[k][1]); 
        close(pipe_pf[k][0]); 
    }
   
    /* Ciclo di recupero dei caratteri inviati dai figli, in ordine di riga e poi in ordine di file */
    while(!finitof())
    { 
        /* Inizializzazione del carattere massimo a -1, cosi' che qualsiasi carattere tornato dai figli (carttere ASCII) sara' tale da risultare maggiore di questo 
            e permettere l'aggiornamento del massimo */
        car_max = -1; 

        /* Primo ciclo di recupero dei caratteri, con aggiornamento del massimo e dell'array finito */
        for (n = 0; n < N; n++)
        {
            nr = read(pipe_fp[n][0], &car, sizeof(char)); 

            if (nr == sizeof(char))
            { 
                /* Lettura avvenuta con successo, dunque si effettuia il confronto del carattere letto con il valore max attuale */
                if (car > car_max)
                { 
                    /* Aggiornamento del massimo e salvataggio dell'indice del processo */
                    car_max = car; 
                    idx_max = n;
                }
            }
            else 
            { 
                /* lettura fallita, quindi verosimilmente il processo figlio Pn e' terminato, e' dunque possibile aggiornare l'array finito con un 1 in corrispondenza dell'elemento n */
                finito[n] = 1; 
            }
        }

        /* Secondo ciclo di invio dei caratteri di controllo, con accertamento di non inviare messaggi a figli terminati, cosi' da evitare il SIGPIPE */
        for (n = 0; n < N; n++)
        { 
            if (!finito[n])
            { 
                if (n == idx_max)
                { 
                    ctrl = 'S'; 
                }
                else
                { 
                    ctrl = 'N'; 
                }

                /* Stampa del carattere di controllo */
                nw = write(pipe_pf[n][1], &ctrl, sizeof(char)); 

                /* controllo sulla scrittura */
                if (nw != sizeof(char))
                { 
                    printf("Errore: scrittura da parte del padre del carattere di controllo al figlio di indice %d non riuscita\n", n); 
                    /*exit(7);      Si decide di non terminare il padre affinche' possa attendere tutti i figli */
                }
            }
        }
    }
        
    /* Ciclo di attesa dei processi figli con recupero e stampa del valore tornato */
    for (n = 0; n < N; n++)
    { 
        if ((pid = wait(&status)) < 0)
        {
            printf("Errore: wait di attesa di uno dei figli non riuscita\n");
            exit(8);
        }

        if (WIFEXITED(status))
        { 
            ritorno = WEXITSTATUS(status); 
            printf("Il processo figlio con pid = %d ha ritornato il valore %d (se 255 e' incorso in qualche errore durante l'esecuzione)\n", pid, ritorno); 
        }
        else 
        {
            printf("Il processo figlio con pid = %d e' terminato in modo anomalo\n", pid); 
        }
    }

    exit(0); 
}
