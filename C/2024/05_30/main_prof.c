/* Soluzione della Seconda Prova in Itinere del 30 Maggio 2024 (solo ParteC): con ricerca PID Nipote */
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#define PERM 0644       /* in ottale per diritti UNIX */

typedef int pipe_t[2];

int main(int argc, char **argv)
{
  /* -------- Variabili locali ---------- */
  int pid;                        /* process identifier per le fork() del padre e del figlio */
  int N;                          /* numero passato sulla riga di comando, che saranno il numero di processi figli e quindi nipoti da creare */
  /* nome indicato nel testo */
  int status;                     /* variabile di stato per la wait */
  pipe_t *piped;                  /* array dinamico di pipe descriptors per comunicazioni figli-padre  */
  pipe_t p;                      	/* una sola pipe per ogni coppia figlio-nipote: chiaramente complessivamente saranno N pipe una per ogni coppia */
  int n, j;                       /* indici per i cicli */
  /* n nome indicato nel testo */
  int outfile;                    /* fd per creazione file da parte del padre */
  char buffer[250];		/* array di caratteri usato sia dal figlio per la sprintf e sia dal padre per ricevere dai figli */
  /* nome indicato dal testo */
  int ritorno;                    /* variabile che viene ritornata da ogni figlio al padre */
  /* ------------------------------------ */

        /* Controllo sul numero di parametri: STRETTO! */
        if (argc != 3 ) /* Ci vogliono solo due parametri */
        {
                printf("Errore nel numero dei parametri dato che argc = %d, mentre ci vogliono esattamente due parametri\n", argc);
                exit(1);
        }

        /* Calcoliamo il numero passato corrispondente al primo parametro e facciamo il controllo che sia strettamente positivo */
        N = atoi(argv[1]); /* usiamo la funzione di libreria atoi: se si riesce a convertire in un numero strettamente positivo lo consideriamo un controllo sufficiente */
	if (N <= 0)
	{
		printf("Errore: Il primo parametro %s non e' un numero strettamente maggiore di 0\n", argv[1]);
		exit(2);
	}
 
	printf("DEBUG-Sono il processo padre con pid %d e creero' %d processi figli\n", getpid(), N);

	/* Per prima cosa il processo padre deve creare un file con il nome passato come secondo parametro */
        if ((outfile = creat(argv[2], PERM)) < 0 )
	/* ERRORE se non si riesce a creare il file */
	{       printf("Errore nella creazione file per %s dato che outfile = %d\n", argv[2], outfile);
		exit(3);
	}

        /* Allocazione dell'array di N pipe descriptors */
        piped = (pipe_t *) malloc (N*sizeof(pipe_t));
        if (piped == NULL)
        {
                printf("Errore nella allocazione della memoria\n");
                exit(4);
        }

        /* Creazione delle N pipe figli-padre */
        for (n=0; n < N; n++)
        {
                if (pipe(piped[n]) < 0)
                {
                        printf("Errore nella creazione della pipe figli-padre per l'indice %d\n", n);
                        exit(5);
                }
        }

        /* Le N pipe figli-nipoti deriveranno dalla creazione di una pipe in ognuno dei figli che poi creeranno un nipote */

        /* Ciclo di generazione dei figli */
        for (n=0; n < N; n++)
        {
                if ((pid = fork()) < 0)
                {
                        printf("Errore nella fork di indice %d\n", n);
                        exit(6);
                }

                if (pid == 0)
                {
                        /* codice del figlio */
                        /* in caso di errori nei figli o nei nipoti decidiamo di tornare -1 che corrispondera' al valore 255 che non puo' essere un valore accettabile di ritorno */

			printf("DEBUG-Sono il figlio %d di indice %d\n", getpid(), n);

                        /* Chiusura delle pipe non usate nella comunicazione con il padre */
			/* ogni figlio scrive solo su piped[n] */
                        for (j=0; j < N; j++)
                        {
                                close(piped[j][0]);
                                if (n != j) close(piped[j][1]);
                        }

                        /* prima creiamo la pipe di comunicazione fra figlio e nipote */
                        if (pipe(p) < 0)
                        {
                                printf("Errore nella creazione della pipe figlio-nipote per l'indice %d\n", n);
                                exit(-1); /* si veda commento precedente */
                        }

                        if ( (pid = fork()) < 0) /* poi ogni figlio crea un nipote */
                        {
                                printf("Errore nella fork di creazione del nipote\n");
                                exit(-1); /* si veda commento precedente */
                        }
                        if (pid == 0)
                        {
                                /* codice del nipote */
                                /* in caso di errori nei figli o nei nipoti decidiamo di tornare -1 che corrispondera' al valore 255 che non puo' essere un valore accettabile di ritorno */

                                printf("DEBUG-Sono il processo nipote con pid %d e sono stato generato dal figlio di indice %d e pid %d sto per eseguire il comando ps\n", getpid(), n, getppid()); 
                                /* chiusura della pipe rimasta aperta di comunicazione fra figlio-padre che il nipote non usa */
                                close(piped[n][1]);

                                /* ogni nipote deve simulare il piping dei comandi nei confronti del figlio e quindi deve chiudere lo standard output e quindi usare la dup sul lato di scrittura della pipe p */
                                close(1);
                                dup(p[1]);
                                /* ogni nipote adesso puo' chiudere entrambi i lati della pipe: il lato 0 NON viene usato e il lato 1 viene usato tramite lo standard output */
                                close(p[0]);
                                close(p[1]);

                                /* Il nipote diventa il comando ps: bisogna usare le versioni dell'exec con la p in fondo in modo da usare la variabile di ambiente PATH  */
                                execlp("ps", "ps", (char *)0);

                                /* Non si dovrebbe mai tornare qui!!*/
                                /* usiamo perror che scrive su standard error, dato che lo standard output e' collegato alla pipe */
                                perror("Problemi di esecuzione del ps da parte del nipote");
                                exit(-1); /* si veda commento precedente */
                        }
                        /* codice figlio */
                        /* ogni figlio per prima cosa deve calcolare la stringa corrispondente al pid del nipote o al suo pid o a quello del padre (a seconda delle combinazioni dei testi): le combinazioni non previste in questa soluzione vengono riportate in commento */
			sprintf(buffer, "%d", pid);	/* PID nipote */
			/* sprintf(buffer, "%d", getpid()); PROPRIO PID */
			/* sprintf(buffer, "%d", getppid()); PID DEL PADRE: NOTA IN QUESTO CASO LE LINEE INDIVIDUATE DA TUTTI I FIGLI SARANNO LA STESSA LINEA! */
			printf("DEBUG-Sono il figlio %d di indice %d e ho ricavato dal pid del nipote la stringa %s\n", getpid(), n, buffer);
                        /* ogni figlio deve simulare il piping dei comandi nei confronti del nipote e poi nei confronti del padre: */
			/* quindi prima deve chiudere lo standard input e quindi usare la dup sul lato di lettura della pipe p */
			close(0);
			dup(p[0]);
			/* quindi dopo deve chiudere lo standard output e quindi usare la dup sul lato di scrittura della pipe piped[n] */
                        close(1);
                        dup(piped[n][1]);
				
                        /* ogni figlio adesso puo' chiudere entrambi i lati della pipe p e la scrittura di piped[n] */
                        close(p[0]);
                        close(p[1]);
                        close(piped[n][1]);

			/* ogni figlio diventa il comando-filtro grep: bisogna usare le versioni dell'exec con la p in fondo in modo da usare la variabile di ambiente PATH  */
                        execlp("grep", "grep", buffer, (char *)0);

                        /* Non si dovrebbe mai tornare qui!!*/
                        /* usiamo perror che scrive su standard error, dato che lo standard output e' collegato alla pipe */
                        perror("Problemi di esecuzione del grep da parte del figlio");
                        exit(-1); /* si veda commento precedente */
             	}
        }

	/* Codice del padre */
	/* Il padre chiude i lati delle pipe che non usa */
        for (n=0; n < N; n++)
                close(piped[n][1]);

	/* Il padre recupera le informazioni dai figli: in ordine di indice */
        for (n=0; n<N; n++)
        {
                j=0; /* inizializziamo l'indice della linea per la singola linea inviata da ogni figlio */
		/* per ogni figlio si devono leggere le informazioni che il figlio con la grep ha scritto sul suo standard output che corrisponde al lato di scrittura della pipe piped[n]; il padre legge, a questo punto, dal lato di lettura di questa pipe */
              	while (read(piped[n][0], &(buffer[j]), 1))
                {
                	/* dato che arriva una sola linea leggiamo tutti i caratteri dalla pipe */
                        j++; /* incrementiamo l'indice della linea */
              	}
		/* scriviamo sul file, creato precedentemente, il numero di caratteri corretti e quindi anche con il terminatore di linea! N.B. Il valore di j e' stato incrementato e quindi e' giusto! */
		write(outfile, buffer, j);	
         }

        /* Il padre aspetta i figli */
        for (n=0; n < N; n++)
        {
                pid = wait(&status);
                if (pid < 0)
                {
                	printf("Errore in wait\n");
                	exit(7);
                }

                if ((status & 0xFF) != 0)
                	printf("Figlio con pid %d terminato in modo anomalo\n", pid);
                else
                { 	ritorno=(int)((status >> 8) & 0xFF);
			if (ritorno != 0)
                  		printf("Il figlio con pid=%d ha fallito l'esecuzione del comando-filtro grep oppure se 255 ha avuto dei problemi\n", pid);	/* se tutto e' stato svolto correttamente dal figlio questa eventualita' NON potra' MAI CAPITARE */
			else
				printf("Il figlio con pid=%d ha ritornato %d, quindi tutto OK!\n", pid, ritorno);
        	}
        }
        exit(0);
}
