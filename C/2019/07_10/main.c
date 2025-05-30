/* Programma C main.c che risolve la parte C della prova di esame totale tenutasi il 10 luglio 2019 */

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
    int pid; 			/* identificatore dei processi figli creati con fork ed attesi con la wait */
    int N; 				/* numero di file passati per parametro */
    char Cz; 			/* carattre da cercare */
    char c; 			/* variabile per memorizzare il carattere corrente */
    int n; 				/* indice dei processi figli */
    int k; 				/* indice generico per i cicli */
    int fdr; 			/* file descriptor associato all'apertura dei file nei vari processi figli */
    pipe_t *pipe_fs; 	/* array dinamico di pipe per la comunicazione tra processi accoppiati nel verso First->Second */
    pipe_t *pipe_sf; 	/* array dinamico di pipe per la comunicazione tra processi accoppiati nel verso Second->First */	
    int occ; 			/* contatore di occorrenze */
    long int pos;       /* posizione dell'occorrenza di Cz trovata */
    long int pos_letta; /* posizione dell'occorrenza trovata dal processo accoppiato */
    int status; 		/* variabile di stato per la wait */
    int ritorno; 		/* valore ritornato dai processi figli */
    int nr, nw; 		/* variabili di controllo per lettura/scrittura da/su pipe */
    /*---------------------------------------------------*/
    
    /* Controllo lasco numero di parametri */
    if (argc < 4)
    {
        printf("Errore: numero di parametri passati a %s errato - Attesi almeno 3 parametri [(program) filename1 filename2 ... Cz] - Passati: %d\n", argv[0], argc - 1); 
        exit(1);
    }

    /* Calcolo del valore di N */
    N = argc - 2; 

    /* Controllo sul carattere passato come ultimo parametro */
    if (strlen(argv[argc - 1]) != 1)
    { 
        printf("Errore: il parametro %s non e' un singolo carattere\n", argv[argc - 1]); 
        exit(2); 
    }

    /* Isolamento del carattere nella variabile Cz */
    Cz = argv[argc - 1][0]; 

    /* Allocazione dinamica della memoria per i due array di pipe. Si noti che sono necessarie 2 pipe per la comunicazione di OGNI coppia, infatti ogni processo per l'accoppiato si comporta 
        sia da lettore (quando deve ricevere la posizione) sia da scrittore (quando deve mandare la propria posizione). Dal momento che le coppie di processi sono esattamente N, si necessitera'
        di due array di N pipe */
    pipe_fs = (pipe_t *)malloc(N * sizeof(pipe_t)); 
    pipe_sf = (pipe_t *)malloc(N * sizeof(pipe_t));  

    if ((pipe_fs == NULL) || (pipe_sf == NULL))
    { 
        printf("Errore: allocazione della memoria per gli array di pipe NON riuscita\n"); 
        exit(3); 
    }

    /* Ciclo di creazione delle pipe */
    for (k = 0; k < N; k++)
    { 
        if (pipe(pipe_fs[k]) < 0)
        { 
            printf("Errore: creazione della pipe di indice %d per la comunicazione nel verso first->second non riuscita\n", k);
            exit(4);  
        }

        if (pipe(pipe_sf[k]) < 0)
        { 
            printf("Errore: creazione della pipe di indice %d per la comunicazione nel verso second->first non riuscita\n", k);
            exit(5);  
        }
    }

    /* Ciclo di creazione dei figli. Si noti che dal momento che si ha una COPPIA di processi associata ad ognuno degli N file, i processi figli da crare saranno 2*N */
    for (n = 0; n < 2*N; n++)
    { 
        if ((pid = fork()) < 0)
        { 
            printf("Errore: fork() di creazione del processo figlio di indice %d non riuscita\n", n); 
            exit(6); 
        }

        if (pid == 0)
        { 
            /* CODICE DEI FIGLI */

            /* Si decide che in caso i processi figli incorrano in qualche tipo di errore, ritornino il valore 0, a prescindere che si tratti del primo o secondo figlio di una coppia,
                 cosi' che la terminazione dovuta ad un errore, possa ssere distinta da quella normale. Infatti i processi figli normalmente ritornano il numero di occorrenze trovate 
                 di Cz, ma nel testo viene riportato che questo valore e' minore O UGUALE a 255 (dunque non conviene tornare -1), ma viene anche detto che il numero minimo di occorrenze 
                 e' 2, una per figlio. Quindi il valore minimo tornato dai figli in caso normale e' 1, quindi il valore 0 e' utilizzabile come ritorno in caso di errore */
            
            /* Divisione del condice dei figli a seconda che siano i primi o secondi di una coppia */
            if (n < N)
            { 
                /* Codice dei figli primi delle varie coppie */
                printf("DEBUG-Sono il processo figlio di indice %d con pid = %d, primo figlio della coppia C%d, e sto per contare le occorrenze del carattere '%c' "
                        "all'interno del file %s\n", n, getpid(), n, Cz, argv[n + 1]); 

                /* Il generico primo figlio Pn: 
                    - Si comporta da scrittore per la pipe pipe_fs di indice n, spedendo al processo accoppiato la posizione delle sue occorrenze trovate
                    - Si comporta da lettore per la pipe pipe_sf di indice n, ricevendo dal processo accoppiato la posizione delle sue occorrenze trovate 
                   Alla luce di cio' sara' possiibile andare a chiudere tutti i lati delle pipe NON utilizzati da questo processo */
                for (k = 0; k < N; k++)
                { 
                    close(pipe_fs[k][0]); 
                    close(pipe_sf[k][1]); 

                    if (k != n)
                    { 
                        close(pipe_fs[k][1]); 
                        close(pipe_sf[k][0]);  
                    }
                }

                /* Apertura del file associato */
                if ((fdr = open(argv[n + 1], O_RDONLY)) < 0)
                { 
                    printf("Errore: apertura del file %s da parte del processo figlio di indice %d, primo figlio della coppia C%d non riuscita - "
                            "file non leggibile o inesistente\n", argv[n + 1], n, n); 
                    exit(0); 
                }

                /* Inizializzazione a 0 del contatore di occorrenze per il ciclo di lettura */
                occ = 0; 

                /* Ciclo di lettura del file carattere per carattere. Infatti in quanto il primo processo della coppia e' quello che inizia la "staffetta" si puo' iniziare 
                    subito con la ricerca del carattere Cz senza attendere null dal secondo processo */
                while (read(fdr, &c, sizeof(char)) > 0)
                { 
                    if (c == Cz)
                    { 
                        /* Trovota un occorrenza del carattere Cz nel file */

                        /* Incremento del contatore di occorrenze */
                        occ++; 

                        /* Calcolo della posizione dell'occorrenza come posizione corrente del file pointer -1, infatti una volta letto il carattere il file pointer si sposta al 
                            carattere successivo, dunque per poter spedire al secondo processo la posizione dell'occorrenza trovata dobbiamo prima decrementare di 1 questa pos */
                        pos = lseek(fdr, 0L, SEEK_CUR) - 1; 

                        printf("DEBUG-Trovata un occorenza del carattere '%c' da parte del primo processo della coppia C%d nella posizione %ld\n", Cz, n, pos);

                        /* Invio al processo accoppiato della posizione trovata */
                        nw = write(pipe_fs[n][1], &pos, sizeof(pos)); 

                        /* Controllo sul numero di byte inviati */
                        if (nw != sizeof(pos)) 
                        { 
                            printf("Errore: il processo figlio di indice %d, primo processo della coppia %d, ha scritto sulla pipe un numero errato di byte\n", n, n);
                            exit(0); 
                        }

                        /* Ricezione della posizione inviata dal secondo processo */
                        nr = read(pipe_sf[n][0], &pos_letta, sizeof(pos_letta)); 

                        /* Controllo sul numero di byte letti */
                        if (nr != sizeof(pos_letta)) 
                        { 
                            /* Questa volta il controllo sul numero di byte letti e' FONDAMENTALE, infatti se non si e' riusciti a leggere nulla dalla pipe questo puo' significare 
                                soltanto che il secondo processo non ha trovato ulteriori occorrenze di Cz ed e' dunque terminato, quindi non ha senso andare avanti con la ricerca 
                                e si puo' dunque uscire dal ciclo di lettura con un break */
                            break; 
                        }

                        /* Se invece la lettura e' riuscita correttamente, si sposta il file pointer di pos_letta + 1 dala partenza del file per far si' che la prossima lettura avvenga 
                            dal carattere successivo rispetto all'occorrenza traovata dal secondo processo */
                        lseek(fdr, pos_letta + 1L, SEEK_SET); 
                    }

                    /* Se il carattere trovato non e' quello cercato non bisogna fare nulla e proseguire con la lettura */
                }
            }
            else 
            { 
                /* Codice dei secondi primi delle varie coppie */
                printf("DEBUG-Sono il processo figlio di indice %d con pid = %d, secondo figlio della coppia C%d, e sto per contare le occorrenze del carattere '%c' "
                        "all'interno del file %s\n", n, getpid(), 2*N - n - 1, Cz, argv[2*N - n]); 

                /* Il generico secondo figlio Pn: 
                    - Si comporta da scrittore per la pipe pipe_sf di indice 2N - n - 1, spedendo al processo accoppiato la posizione delle sue occorrenze trovate
                    - Si comporta da lettore per la pipe pipe_fs di indice 2N - n - 1, ricevendo dal processo accoppiato la posizione delle sue occorrenze trovate 
                   Alla luce di cio' sara' possiibile andare a chiudere tutti i lati delle pipe NON utilizzati da questo processo */
                for (k = 0; k < N; k++)
                { 
                    close(pipe_fs[k][1]); 
                    close(pipe_sf[k][0]); 

                    if (k != 2*N - n - 1)
                    { 
                        close(pipe_fs[k][0]); 
                        close(pipe_sf[k][1]);  
                    }
                }

                /* Apertura del file associato. Si noti che il file associato alla coppia di indice i e' quello con indice i + 1 nell'array argv, questo + 1 va sostanzialmente 
                    ad annullare l'effetto del -1 che viene usato per ricavare gli indici giusti dei secondi figli */
                if ((fdr = open(argv[2*N - n], O_RDONLY)) < 0)
                { 
                    printf("Errore: apertura del file %s da parte del processo figlio di indice %d, secondo figlio della coppia C%d non riuscita - "
                            "file non leggibile o inesistente\n", argv[2*N - n], n, 2*N - n - 1); 
                    exit(0); 
                }

                /* Inizializzazione a 0 del contatore di occorrenze per il ciclo di lettura */
                occ = 0; 

                /* Dal momento che il secondo processo prima di poter effettuara la ricerca deve attenere una prima posizione dal processo figlio, e' opportuno che prima di 
                    entrare nel ciclo di lettura riceva questo valore */
                nr = read(pipe_fs[2*N - n - 1][0], &pos_letta, sizeof(pos_letta));

                /* Controllo del numero letto di caratteri. A questo punto infatti, dal momento che per specifica in ogni file ci sono almeno 2 occorrenze del carattere Cz, 
                    dovremmo sempre ricevere una posizione valida, corrispondente a quella della prima occorrenza. Se cosi' non accade e la read torna 0 allora significa che 
                    il numero di occorrenze era necessariamente 0, dunque c'e' stato qualche problema a monte ed e' dunque possibile terminare il figlio con uscita 0 */
                if (nr != sizeof(pos_letta))
                {
                    printf("Errore: il processo figlio di indice %d, secondo processo della coppia %d, ha letto dalla pipe un numero errato di byte alla prima lettura\n", n, 2*N - n - 1); 
                    exit(0); 
                }
                
                /* Spostamento del file pointer al carattere immediatamente successivo a quello dell'occorrenza traovata dal primo processo */
                lseek(fdr, pos_letta + 1L, SEEK_SET);

                /* A questo punto e' possibile iniziare un ciclo di lettura identico a quello del primo figlio */
                while (read(fdr, &c, sizeof(char)) > 0)
                { 
                    if (c == Cz)
                    { 
                        /* Trovota un occorrenza del carattere Cz nel file */

                        /* Incremento del contatore di occorrenze */
                        occ++; 

                        /* Calcolo della posizione dell'occorrenza come posizione corrente del file pointer -1, infatti una volta letto il carattere il file pointer si sposta al 
                            carattere successivo, dunque per poter spedire al secondo processo la posizione dell'occorrenza trovata dobbiamo prima decrementare di 1 questa pos */
                        pos = lseek(fdr, 0L, SEEK_CUR) - 1; 
                        
                        printf("DEBUG-Trovata un occorenza del carattere '%c' da parte del secondo processo della coppia C%d nella posizione %ld\n", Cz, 2*N - n - 1, pos);

                        /* Invio al processo accoppiato della posizione trovata */
                        nw = write(pipe_sf[2*N - n -1][1], &pos, sizeof(pos)); 

                        /* Controllo sul numero di byte inviati */
                        if (nw != sizeof(pos)) 
                        { 
                            printf("Errore: il processo figlio di indice %d, secondo processo della coppia %d, ha scritto sulla pipe un numero errato di byte\n", n, 2*N - n - 1);
                            exit(0); 
                        }

                        /* Ricezione della posizione inviata dal primo processo */
                        nr = read(pipe_fs[2*N - n - 1][0], &pos_letta, sizeof(pos_letta)); 

                        /* Controllo sul numero di byte letti */
                        if (nr != sizeof(pos_letta)) 
                        { 
                            /* Questa volta il controllo sul numero di byte letti e' FONDAMENTALE, infatti se non si e' riusciti a leggere nulla dalla pipe questo puo' significare 
                                soltanto che il primo processo non ha trovato ulteriori occorrenze di Cz ed e' dunque terminato, quindi non ha senso andare avanti con la ricerca 
                                e si puo' dunque uscire dal ciclo di lettura con un break */
                            break; 
                        }

                        /* Se invece la lettura e' riuscita correttamente, si sposta il file pointer di pos_letta + 1 dala partenza del file per far si' che la prossima lettura avvenga 
                            dal carattere successivo rispetto all'occorrenza trovata dal primo processo */
                        lseek(fdr, pos_letta + 1L, SEEK_SET); 
                    }

                    /* Se il carattere trovato non e' quello cercato non bisogna fare nulla e proseguire con la lettura */
                } 
            }

            exit(occ); 
        }
    }
    
    /* CODICE DEL PADRE */

    /* Il processo padre e' esterno a tutti gli schemi di comunicazione, puo' dunque andare a chiudere TUTTI i lati di TUTTE le pipe */
    for (k = 0; k < N; k++)
    { 
        close(pipe_fs[k][0]); 
        close(pipe_fs[k][1]); 
        close(pipe_sf[k][0]); 
        close(pipe_sf[k][1]); 
    }
        
    /* Ciclo di attesa dei processi figli con recupero e stampa del valore tornato. Si noti che anche per questo ciclo, l'indice k dovra' arrivare fino a 2N - 1 e non N - 1  */
    for (k = 0; k < 2*N; k++)
    { 
        if ((pid = wait(&status)) < 0)
        {
            printf("Errore: wait di attesa di uno dei figli non riuscita\n"); 
            exit(7); 
        }

        if (WIFEXITED(status))
        {   
            ritorno = WEXITSTATUS(status);
            printf("Il processo figlio con pid = %d ha ritornato il valore %d (se 0 allora e' incorso in qualche errore durante l'esecuzione!)\n", pid, ritorno);
        }
        else 
        { 
            printf("Il processo figlio con pid = %d e' terminato in modo anomao\n", pid); 
        }
    }

    exit(0); 
}
