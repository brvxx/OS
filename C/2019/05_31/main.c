/* prgoramma C main.c che risolve la seconda prova in itinere tenutasi il 31 maggio 2019 (testo 1) */ 

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h> 
#include <sys/wait.h>



/* Definizione del TIPO pipe_t che rappresenta un array di due interi, il quale ci permettera' di gestire facilmente le pipe usate per la comunicazione */
typedef int pipe_t[2]; 

/* Definizione del TIPO tipo_s che rappresenta una struttura dati atta al passaggio di informazioni tra figli e padre */ 
typedef struct
{
	int pid_n; 
	int len;
	char linea[250];
} tipo_s; 


int main (int argc, char **argv)
{ 
	/*------------------ Variabili Locali --------------------*/ 
	int N; 			/* numero di file passati e dunque anche di processi figli creati */ 
	int i, j; 		/* indici per i cicli */
	int pid; 		/* identificatore dei processi creati con la fork e attesi con la wait */ 
	pipe_t *piped; 		/* arrady dinamico per contenere le N pipe create dal processo padre per la comunicazione coi processi filgi: 
				Ogni figlio e' associato ad una singola pipe, quella dallo stesso indice, e su questa si comporta come scrittore 
				nei confronti del padre */ 
	pipe_t p; 		/* pipe per la comunicazione tra figli e nipoti */ 
	tipo_s msg; 		/* Struttura che conterra' le informazioni che ogni figlio passa al padre */ 
	int nr; 		/* variabile per il controllo dei byte letti dalle pipe */ 
	int status; 		/* variabile di stato della wait */ 
	int ritorno; 		/* valore ritornato dai vari processi figli e nipoti */ 
	/*--------------------------------------------------------*/ 

	
	/* controllo lasco numero di parametri. Questi devono essere N, con N >= 3, dunque argc sara' almeno 4 */ 
	if (argc < 4) 
	{ 
		printf("Errore: numero di parametri passato errato - Attesi almeno 3 - Passati %d\n", argc - 1); 
		exit(1); 
	} 
	
	N = argc - 1; 	/* numero di file ottenuto sottraendo al numero totale di parametri 1, che rappresenta il nome dell'eseguibile stesso */

	/* allocazione memoria per l'array di pipe */ 
	if ((piped = (pipe_t *)malloc(N * sizeof(pipe_t))) == NULL) 
	{ 
		printf("Errore: non e' stato possibile allocare memoria per l'array di pipe con la malloc\n"); 
		exit(2); 
	}

	/* creazione delle N pipe per la comunicazione tra padre e figli */ 
	for (i = 0; i < N; i++)
	{ 
		if (pipe(piped[i]) < 0) 
		{ 
			printf("Errore: non e' stato possibile creare la pipe %d\n", i); 
			exit(3); 
		} 
	} 

	/* ciclo di creazione dei processi figli */ 
	for (i = 0; i < N; i++)
	{ 
		if ((pid = fork()) < 0) 
		{ 
			printf("Errore: non e' stato possibile creare il figlio di indice %d con la fork\n", i); 
			exit(4); 
		}

		if (pid == 0) 
		{ 
			/* CODICE DEL FIGLIO */ 

			/* per i vari casi di errore riscontrati dal figlio ritorneremo il valore -1 che corrisponde a 255 e sara' dunque possibile distinguerlo 
			dai valori di ritorno del sort che ritorna in caso di errore dei valori piccoli positivi */ 

			printf("DEBUG-Sono il processo figlio con pid = %d e indice %d e sto per andare a creare un processo nipote\n", getpid(), i); 
			
			/* chisura dei lati di lettura di TUTTE le pipe create dal padre e dei lati di letture di tutte le pipe tranne quella dello stesso indice 
			del figlio corrente (i) */ 
			for (j = 0; j < N; j++) 
			{ 
				close(piped[j][0]); 

				if (j != i) 
				{ 
					close(piped[j][1]); 
				} 
			} 

			/* creazione della pipe per la comunicazione col processo nipote */ 
			if (pipe(p) < 0)
			{ 
				printf("Errore: il figlio di indice %d non e' riuscito a creare la pipe di comunicazione col nipote\n", i);
				exit(-1);
			} 

			/* creazione del processo nipote */ 
			if ((pid = fork()) < 0) 
			{				
				printf("Errore: non e' stata possibile la creazione del processo nipote con la fork da parte del figlio %d\n", i); 
				exit(-1); 
			} 

			if (pid == 0) 
			{ 
				/* CODICE DEL PROCESSO NIPOTE */ 
				
				/* cosi' come per i processi figli vale lo stesso principio per i valori di ritorno negativi */ 

				printf("DEBUG-Sono il processo nipote con pid = %d e sono stato creato dal figlio di indice %d e sto per "
					"andare ad ordinre il file %s\n", getpid(), i, argv[i + 1]); 

				
				/* chisura dell'ultimo lato di scrittura della pipe usata dal figlio creatore del processo nipote per comunicare 
				col padre */ 
				close(piped[i][1]); 

				/* implementiamo la ridirezione dello standard output nello stesso modo del piping dei comandi, cosi' che il processo 
				figlio possa ricevere sulla pipe p l'output del comando sort eseguito dal nipote */ 
				close(1); 
				dup(p[1]); 

				/* a questo punto lo standard output del processo nipote sara' collegato al lato di scrittura della pipe creata dal rispettivo
				figlio, dunque possaimo andare a chiudere ENTRAMBI i lati della pipe p:
				- quello di lettura perche' il processo nipote si comporta da consumatore nei confronti del figlio 
				- quello di scrittura perche' il suo riferimento oramai si trova nella posizione 1 della TFA dopo la dup */ 	
				close(p[0]); 
				close(p[1]); 

				/* esecuzione del comando sort tramite l'exec in versione l perche' tanto i parametri da passare al comando sono fissi per tutti 
				i vari processi nipote, e p dal momento che andiamo ad eseguire un comando di sistema, il cui percorso si trova nella variabile 
				PATH di sistema */ 
				
				/* nota che andiamo ad eseguire il comando sort in forma normale e non filtro, perche' in questo caso non comporterebbe alcun 
				vantaggio usare la versione filtro rispetto a quella normale, se non tutte le difficolta' varie per implementare la ridirezione 
				dello standard output */ 

				execlp("sort", "sort", "-f", argv[i + 1], (char *) 0); 
				
				/* se si arriva ad eseguire questa parte di codice allora significa che la exec ha avuto un qualche problema, per riportare tale 
				errore dovremmo necessariamente scrivere sullo stderr perche' lo stdout lo abbiamo gia' ridirezionato */ 

				perror("Errore: non e' stato possibile eseguire il comando sort con la execlp\n"); 
				exit(-1); 
			} 

			/* ritorno al codice del processo figlio */ 
			
			/* chisura del lato di scrittura della pipe p */ 
			close(p[1]); 

			/* inizializziamo il campo pid_n della struct che i processi figli devono passare al padre */ 
			msg.pid_n = pid; 

			/* andiamo poi ad inizializzare il campo len a 0, in modo tale che se per caso l'esecuzione del sort fallisce e dunque non verra' stampato 
			nulla sulla pipe, e dunque non si entrera' nel while successivo, quanto meno abbiamo dato un valore nullo alla lunghezza piuttosto che 
			lasciarla non inizializzata ad un qualche valore inutile */ 
			msg.len = 0; 

			/* leggiamo la prima riga che il processo nipote ha stampato sulla pipe p, andando a stampare tutti i caratteri letti fino al primo \n 
			direttamente dentro l'array di caratteri presente dentro la struttura msg */ 
			j = 0; 		
			while (read(p[0], &(msg.linea[j]), 1) > 0) 
			{ 	
				if (msg.linea[j] == '\n') 	/* siamo alla fine della prima riga */ 
				{ 
					/* aggiorniamo il campo lunghezza della linea della struttura */ 
					msg.len = j + 1; 	/* incrementiamo di 1 perche' j rappresenta l'indice corrente che parte da 0 e non 1 */  
					break; 
				} 
				else
				{ 
					j++;
				} 
			} 


			/* spediamo dunque la struttura inizializzata al processo padre */ 
			write(piped[i][1], &msg, sizeof(msg)); 
			
			/* attendiamo il processo nipote creato. Nota che non si tratta di una sola formalita' ma e' importante che questo avvenga onde 
			evitare che il processo nipote che sta potenzialmentea ancora scrivendo sulla pipe non venga terminato dal SIGPIPE */ 
			if ((pid = wait(&status)) < 0) 
			{ 
				printf("Errore: il processo figlio di indice %d non e' riuscito ad attndere il nipote con la wait\n", i); 
				exit(-1); 
			} 

			if (WIFEXITED(status)) 
			{ 
				ritorno = WEXITSTATUS(status); 
				printf("DEBUG-Il processo nipote con pid = %d  associato al figlio di indice %d che ha eseguito il sort sul file %s "
					"ha ritornato %d\n", pid, i, argv[i + 1], ritorno); 
			}
			else
			{ 
				printf("Il processo nipote con pid = %d associato al figlio di indice %d e' terminato in modo anomalo\n", pid, i);

				/* mentre nel processo padre in caso di arresto anomalo non terminiamo, perche' ha vari figli associati, in questo caso 
				il corretto funzionamento del processo figlio dipende dalla corretta esecuzione del suo solo figlio (il processo nipote), 
				dunque in questo caso terminiamo il figli con errore */ 
				exit(-1); 
			} 

			if (msg.len != 0) 
			{ 
				exit(msg.len - 1); 	/* ritorniamo la lunghezza della riga privata del terminatore di linea */ 
			} 
			else
			{ 
				exit(msg.len); 	/* nel caso in cui il file era vuoto oppure inesistente e dunque non ci sono state stampe sulla pipe la lunghezza e' ha 0 
						dunque non va decrementata di 1, se no si otterrebbe -1, scambiabile come errore */ 
			} 
		}
	} /* fine for */ 

	/* CODICE DEL PADRE */ 

	/* chisura dei lati di scrittura di tutte le pipe */ 
	for (i = 0; i < N; i++) 
	{ 
		close(piped[i][1]);
	} 

	/* recupero delle strutture inviate dai processi figli seguendo l'ordine dei file */ 
	for (i = 0; i < N; i++) 
	{ 
		/* lettura della struttura inviata dall'i-esimo processo dall'i-esima pipe */ 
		nr = read(piped[i][0], &msg, sizeof(msg)); 

		/* controllo sul numero di byte letti */ 
		if (nr == sizeof(msg)) 
		{ 
			/* trasformiamo la linea contenuta in msg.linea in una stringa aggiungendo il termintore \0. Nota che non c'e' scritto 
			nel testo che il terminatore di linea vada troncato, quindi lo dobbiamo mantenere, andando a scrivere proprio all'indice len*/ 
			msg.linea[msg.len] = '\0';

			/* stampa delle informazioni contenute nella struttura */ 
			printf("Il processo figlio di indice %d, che ha dato vita al nipote di pid = %d, ha letto dal file %s ordinato la sua prima riga, ossia: "
				"'%s', la quale ha lunghezza di %d caratteri (compreso il terminatore di riga)\n", i, msg.pid_n, argv[i + 1], msg.linea, msg.len);
		} 
	} 

	/* attesa dei processi figli con recupero dei valori tornati */
	for (i = 0; i < N; i++) 
	{ 
		if ((pid = wait(&status)) < 0) 
		{ 
			printf("Errore: non e' stato possibile attendere uno dei processi figli con la wait\n"); 
			exit(5); 
		} 
		
		if (WIFEXITED(status)) 
		{ 
			ritorno = WEXITSTATUS(status); 
			printf("Il processo figlio con pid = %d ha ritornato il valore %d (se 255 problemi nel figlio o rispettivo nipote)\n", pid, ritorno); 
		} 
		else 
		{ 
			printf("Il processo figlio con pid = %d e' terminato in modo anomalo\n", pid); 
		} 
	} 

	exit(0); 
} 
