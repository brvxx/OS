#!/bin/sh 
#file comandi principale relativo alla parte di shell dell'esame del 9 febbraio 2015 

#controllo stretto sul numero di parametri passato 

case $# in 
2) 	echo "DEBUG-FCP.sh: numero di parametri pasatto corretto, OK!";; 
*) 	echo "ERRORE: numero di parametri passato errato - Aspettati 2 - Passati $#"
	echo "Usage is: $0 dirass numpos" 
	exit 1;; 
esac 

#conotrllo che il primo parametro sia un nome assoluto di una directory traversabile 

case $1 in 
/*) 	if test ! -d $1 -o ! -x $1  
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
		echo "ERRORE: $2 non è un valore strettamente postivo in quanto nullo" 
		exit 5 
	fi;; 
esac 
echo "DEBUG-FCP.sh: tipo dei parametri passato corretto, OK!" 
echo 
echo 

#aggiorniamo la variabile PATH aggiungendo la directory corrente e la esportiamo per compatibilità 

PATH=`pwd`:$PATH 
export PATH 

#creiamo un file temporaneo che ci servirà per andare a salvare il nome assoluto di tutti i file validi trovati sui quali dovremo poi tornare a lavore nella parte principale 

> /tmp/nomiAssoluti$$ 

#invochiamo il file comandi ricorsivo 

FCR.sh $* /tmp/nomiAssoluti$$ 


#arrivati a questo punto avremo un file temporaneo che contiene tutti i file validi della gerarchia esplorata, dunque dobbiamo andare a chiedere per ognuno di questi file un valore minore o ugule di K e invocare poi la parte in C con tutti i file intervallati dal valore passato dall'utente, per fare ciò avremo bisogno di una variabile lista che chiameremo param che conterrà un nome file e il relativo numero associato e via dicendo 

param=

echo
for F in `cat /tmp/nomiAssoluti$$` 
do
	param="$param $F" 
	echo -n "trovato file $F, immettere un valore intero strettamente positivo minore uguale di $2: " 
	read answer 
	
	case $answer in 
	*[!0-9]*) 	echo "ERRORE: valore immesso non accettato in quanto non è un valore numerico, oppure è negativo"
			rm /tmp/nomiAssoluti$$	#se usciamo dobbiamo anche eliminarlo, perché l'eliminazione effettiva avverrebbe alla fine del file  
			exit 6;; 
	*) 	if test $answer -eq 0 -o $answer -gt $2 
		then 
			echo "ERRORE: valore immesso nullo, oppure maggiore di $2, range ammesso [1 - $2]"
			rm /tmp/nomiAssoluti$$ 
			exit 7 
		else
			param="$param $answer" 
		fi;; 
	esac 
done 
echo 

#invocazione della parte in c

echo "DEBUG-FCP.sh: qui ci dovrebbe essere l'invocazione della parte in C, tipo: fileC \$param, con param = $param" 

rm /tmp/nomiAssoluti$$ 	#cancelliamo il file temporaneo creato  
