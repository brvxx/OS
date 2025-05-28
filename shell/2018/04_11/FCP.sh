#!/bin/sh 
#file comandi principale relativo alla prima prova in itinere del 11 aprile 2018 

#controllo lasco sul numero di parametri, N + 1 con N >= 2, quindi ci aspettiamo almeno 3 parametri 

case $# in 
0|1|2) 	echo "ERRORE: numero di parametri passato errato - Attesi almeno 3 - Passati $#" 
	echo "Usage is: num dirass1 dirass2 [dirass3] ..." 
	exit 1;; 
*) 	echo "DEBUG-FCP.sh: numero di parametri passato corretto, OK!";; 
esac 

#controllo che il primo parametro sia un valore numerico intero strettamente positivo 

case $1 in 
*[!0-9]*) 	echo "ERRORE: $1 non è un valore numerico, oppure è un numero negativo o decimale" 
		exit 2;; 
*) 	
	if test $1 -eq 0 
	then 
		echo "ERRORE: $1 non è strettamente positivo in quanto nullo" 
		exit 3 
	fi;; 
esac 

Y=$1 	#salviamo il primo parametro dentro una variabile così da poter usare $* come una lista di sole gerarchie 
shift 


#controllo che le N gerarchie siano dei nomi assoluti di directory traversabili 

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
echo	#stampa di una riga vuota per leggibilità

#mettiamo la directory corrente dentro la PATH, effettuando poi l'export per compatibilità 

PATH=`pwd`:$PATH 
export PATH 

#creiamo col la ridirezione a vuoto un file temporaneo che servirà per contenere tutti i nomi assoluti dei file creati dal file ricorsivo durante le esplorazioni 

> /tmp/conta$$ 


#invochiamo il file comandi ricorsivo FCR.sh per ognuna delle gerarchie passate come parametro, contenute in $* 

for G 
do 
	FCR.sh $G $Y /tmp/conta$$ 
done 


#arrivati a questo punto il file conta, conterrà i nomi assoluti di tutti i file creati, andiamo quindi a stampare il nuemero totale di file trovati e poi stampando per ogni file il suo nome assoluto e il contenuto 

echo "numero totale di file creati globalmente: `wc -l < /tmp/conta$$`"
echo "elenchiamo di seguito la lista dei file creati assieme al loro contenuto" 
echo 

for F in `cat /tmp/conta$$` 
do	
	echo "creato il file $F" 
	
	case $F in 
	*.NO*) 	echo "il file è vuoto, in quanto il file originale non conteneva almeno 5 linee"
		echo
		echo
		;; 
	*) 	
		echo "il cui contenuto e':" 
		echo =========================================
		cat $F 
		echo =========================================
		echo
		echo 
		;;
	esac 
done 

rm /tmp/conta$$		#andiamo ad eliminare il file temporaneo 
