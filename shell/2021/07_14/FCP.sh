#!/bin/sh 
#file comandi principale relativo alla parte di shell dell'esame totale del 14 luglio 2021

#controllo lasco del numero di parametri, Q + 2 con Q >= 2, quindi ci aspettiamo almento 4 parametri passati 

case $# in 
0|1|2|3) 	echo "ERRORE: numero di parametri passati errato - Attesi almeno 4 - Passati $#" 
		echo "Usage is: $0 dirass1 dirass2 [dirass3] ... numpos1 numpos2" 
		exit 1;; 
*) 
	echo "DEBUG-FCP.sh: numero di parametri passato corretto, OK!";; 
esac 

#controlliamo il tipo dei parametri, andando poi ad inserire le Q gerarchie dentro ad una variabile GER, e gli ultimi due valori numerici rispettivamente dentro H e M. Utilizziamo una variabile num per tenere traccia di che parametro stiamo visualizzando e distinguere così gerarchie da valori numerici

GER= 
H= 
M= 

num=0

for i 
do 
	num=`expr $num + 1`
	
	if test $num -lt `expr $# - 1` 	#se non ci troviamo sugli utlimi due parametri allora siamo sulle gerarchie 
	then 
		case $i in 
		/*) 	if test ! -d $i -o ! -x $i 
			then 
				echo "ERRORE: $i non è una directory traversabile" 
				exit 2 
			fi;; 
		*) 
			echo "ERRORE: $i non è un nome assoluto" 
			exit 3;; 
		esac 

		#a questo punto la directory corrente è valida, quindi la inseriamo dentro GER 
		GER="$GER $i" 
	
	else 	#se ci troviamo sugli ultimi due parametri allora siamo in corrispondenza dei valori numerici 
		case $i in 
		*[!0-9]*) 	echo "ERRORE: $i non è un valore numerico, oppure è negativo o non intero" 
				exit 4;; 
		*) 	
			if test $i -eq 0
			then 
				echo "ERRORE: $i non è un valore strettamete postivo in quanto nullo" 
				exit 5
			fi;; 
		esac 

		#a questo punto che abbiamo un valore numerico intero strettamente positivo, dobbiamo capire se siamo in corrispondenza di H o M 

		if test $num -eq `expr $# - 1`	#ci troviamo sul penultimo parametro, dunque H 
		then 
			H=$i 
		else 	#ci troviamo sull'ultimo parametro, dunque M 
			M=$i 
		fi 
	fi 
done 
echo "DEBUG-FCP.sh: tipo dei parametri passato corretto, OK!" 
echo 

#aggiorniamo il valore della variabile di ambiente PATH, aggiungendo la directory corrente e esportandola per compatibilità 

PATH=`pwd`:$PATH 
export PATH 

#invochiamo il file comandi ricorsivo per ogni gerarchia passata per parametro, passandogli la i-esima gerarchia (Gi), H e M 

for G in $GER 
do 
	FCR.sh $G $H $M
	echo  
done 

#nota che non è necessario alcun file temporaneo in quanto non ci sono da effettuare conteggi post esplorazione 




