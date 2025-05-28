#!/bin/sh 
#file comandi principale (FCP.sh) realtivo all'esame totale del 18 gennaio 2017

#controllo lasco sul numero di parametri passati, vogliamo che siano passati esattamente 2 parametri 

case $# in 
2) 	echo "DEBUG-FCP.sh: numero di parametri pasatto corretto: $#";; 
*) 	echo "ERRORE: numero di parametri passato errato - aspettati 2 parametri, passati: $#" 
	echo "Usage is: $0 hierarchy directory" 
	exit 1;; 
esac 

#controllo che la gerarchia (prima directory) sia scritta in forma assoluta e sia una directory traversabile attraverso un case 

case $1 in 
/*) 	
	if test ! -d $1 -o ! -x $1 
	then 
		echo "ERRORE: la gerarchia $1 non è una directory traversabile" 
		exit 2
	fi;;
*) 
	echo "ERRORE: la gerarchia $1 non è stata passata con il suo nome assoluto" 
	exit 3;; 
esac 
echo "DEBUG-FCP.sh: $1 è il nome assoluto di una directory traversabile, OK" 

#controllo che la il nome della directory target sia stato passato in forma relativa semplice 

case $2 in 
*/*) 	echo "ERRRORE: la directory $2 target non è stata passata con il suo nome relativo semplice" 
	exit 4;; 
*) 	;;
esac
echo "DEBUG-FCP.sh: $2 è un nome relativo semplice, OK" 

#controllo sui parametri terminato, se siamo arrivati fino a qua significa che sono stati passati correttamente 2 parametri, di cui il primo è una gerarchia, ossia un nome assoluto di una directory traversabile e il secondo il nome relativo semplice di una directory "target" 

#adesso andiamo a creare un file temporaneo che ci permetterà poi alla fine della ricerca ricorsiva di contare tutti i file che contengono almeno un occorrenza di un carattere numerico 

> /tmp/tmp$$	#mettiamo $$ per far si che ogni volta che si esegua questo file comandi non si vada a sovrascrivere il vecchio file tmp, per andare a fare debug sulle varie esecuzioni di FCP.sh 


#aggiorniamo il valore della variabile PATH, aggiungendo ai percorsi in cui cercare i comandi anche la directory attuale, così che si possano effettuare le call ricorsive in qualunque punto del file system 

PATH=$(pwd):$PATH 
export PATH 	#per compatibilità 


#chiamata al file ricorsivo FCR.sh che andrà ad effettuare la ricerca e la stampa in output delle directory dentro la gerarchia che rispettano le richieste 

echo "DEBUG-FCP.sh: inizio esplorazione gerarchia" 
echo	#stampa di una linea vuota per leggibilità  
FCR.sh $* /tmp/tmp$$ 	#passiamo tutti i parametri passati a FCP.sh e il file temporaneo per salvarci i file trovati che rispettano la specifica

echo 
echo "DEBUG-FCP.sh: esplorazione gerarchia terminata" 
echo 

#a questo punto non ci resta che stampare in output il numero di file trovati e il loro nome assoluto

echo "numero totale di file validi trovati: $(wc -l < /tmp/tmp$$)" 
echo "nome assoluto dei vari file individuati: "
cat /tmp/tmp$$

rm /tmp/tmp$$ 	#andiamo a cancellare il file temporaneo tanto non ci serve conretamente, se mai si dovesse fare del debug, si vada a commentare questo comando 




