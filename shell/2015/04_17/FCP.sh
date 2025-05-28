#!/bin/sh 
#file comandi principale relativo alla prima prova in itinere del 17 aprile 2015

#controllo lasco del numero di parametri passato, N + 1 con N >= 2, quindi ci aspettiamo almeno 3 parametri 

case $# in 
0|1|2) 	echo "ERRORE: numero di parametri passato errato - Attesi almeno 3 - Passati $#" 
	echo "Usage is: $0 numpos dirass1 dirass2 [dirass3] ..." 
	exit 1;; 
*) 
	echo "DEBUG-FCP.sh: numero di parametri passato corretto, OK!";; 
esac 

#controllo che il primo paramtro sia un numero intero strettamente positivo (controllo col case) 

case $1 in 
*[!0-9]*)	echo "ERRORE: $1 non è un valore numerico, oppure è un numero negativo o non intero" 
		exit 2;; 
*) 
	if test $1 -eq 0 
	then 
		echo "ERRORE: $1 non è un valore strettamente positivo in quanto nullo" 
		exit 3
	fi;; 
esac 

#salviamo il valore numerico del primo parametro dentro la variabile X per poter effettuare uno shift e usare $* come lista di sole gerarchie 

X=$1 
shift 

for G  
do 
	case $G in 
	/*) 	if test ! -d $G -o ! -x $G 
		then 
			echo "ERRORE: $G non è una directory traversabile" 
			exit 4
		fi;; 
	*)	
		echo "ERRORE: $G non è un nome assoluto" 
		exit 5;; 
	esac 
done 
echo "DEBUG-FCP.sh: tipo dei parametri passati corretto, OK!" 
echo 	#stampa di una riga vuota per leggibilità 


#aggiorniamo la variabile d'ambiente PATH, aggiungendo la directory corrente e la esportiamo per compatibilità 

PATH=`pwd`:$PATH 
export PATH 


#creiamo un file temporaneo che al termine delle N fasi conterrà tutti i file trovati globalmente nelle gerarchie passate per parametro 

> /tmp/tmp$$ 


#invochiamo il file comandi ricorsivo per ogni gerarchia passata per parametro, passando anche il file temporaneo 

for G 
do 
	FCR.sh $G $X /tmp/tmp$$ 
done 


#arrivati a questo punto il file tmp$$ conterrà tutti i file validi trovati nella ricerca 
#stampiamo il numero totale di file trovati 

echo "NUMERO TOTALE di file validi trovati: `wc -l < /tmp/tmp$$`" 
echo 

#per ogni file andiamo a stamparne il nome assoluto e chiediamo all'utente un valore che corrisponde alla riga del file da stampare 

for F in `cat /tmp/tmp$$`
do 
	echo "- file trovato: $F" 
	echo -n "inserire un valore numerico strettamente positivo e strettamente minore di $X (1 <= K < $X): " 
	read K 
	
	case $K in 
	*[!0-9]*) 	echo "ERRORE: il valore immesso non è un valore numerico, oppure è negativo"
			rm /tmp/tmp$$	#rimuoviamo il file prima di uscire se no continuerebbe ad esistere in memoria 
			exit 6;; 
	*) 	if test $K -lt 1 -o $K -ge $X 
		then 
			echo "ERRORE: Il valore immesso non fa parte del range ammissibile di valori: [1 - $X)" 
			rm /tmp/tmp$$
			exit 7
		fi;; 
	esac 

	#stampiamo la K-esima riga del file corrente 
	
	echo
	echo "stampa della riga $K del file corrente:" 
	echo =============================================
	head -$K $F | tail -1 
	echo =============================================
	echo
	echo
done 

rm /tmp/tmp$$ 	#cancelliamo il file temporaneo 




