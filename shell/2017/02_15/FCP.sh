#!/bin/sh 
#file comandi principale relativo alla parte di shell dell'esame del 15 febbraio 2017 

#controllo stretto del numero di parametri 

case $# in 
2) 	echo "DEBUG-FCP.sh: numero di parametri passato corretto, OK!"
	;; 
*)	echo "ERRORE: numero di parametri passato errato - Attesi 2 - Passati: $#" 
	exit 1;; 
esac 

#controllo che il primo parametro sia un nome assoluto di una directory traversabile 

case $1 in 
/*)	if test ! -d $1 -o ! -x $1 
	then 
		echo "ERRORE: $1 non è una directory traversabile" 
		exit 2 
	fi;; 
*) 
	echo "ERRORE: $1 non è un nome assoluto" 
	exit 3;; 
esac 

#controllo che il secondo parametro sia un numero intero strettamente positivo 

case $2 in 
*[!0-9]*) 	echo "ERRORE: $2 non è un valore numerico, oppure è un numero negativo o non intero" 
		exit 4;; 
*) 
	if test $2 -eq 0 
	then 
		echo "ERRORE: $2 non è un numero strettamente positivo in quanto nullo" 
		exit 5 
	fi;; 
esac 
echo "DEBUG-FCP.sh: tipo dei parametri passati corretto" 
echo 

#aggiorniamo la variabile di ambiente PATH e la esportiamo per compatibilità 

PATH=`pwd`:$PATH 
export PATH 


#nota che non dovendo andare ad effettuare un conteggio non è nemmeno necessario andare a creare un file temporaneo, infatti possiamo andare ad effettuare la stampa delle directory e dei file, direttamente nel file ricorsivo durante la ricerca 

#invochiamo il file comandi ricorsivo 

FCR.sh $*  

echo 
echo "DEBUG-FCP.sh: finito tutto!"
