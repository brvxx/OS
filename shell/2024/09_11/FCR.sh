#!/bin/sh 
#file comandi ricorsivo relativo alla parte di shell dell'esame totale del 11 settembre 2024

cd $1 	#ci spostiamo sulla gerarchia da esplorare 

conta=`expr $3 + 1`  	#variabile locale che tiene traccia del livello della directory attuale 

if test $conta -eq $2 
then 
	#salviamo la directory corrente nel file temporaneo delle directory 

	pwd >> $4 

	for F in * 
	do
		if test -f $F -a -r $f 
		then 
			if test `wc -c < $F` -ne 0 	#caso in cui abbiamo un file non vuoto  		
			then 
				#salviamo il nome relativo semplice del file corrente dentro al file temporaneo dei nomi file 
				echo $F >> $5 
			fi 
		fi 
	done 
fi 

#invocazione ricorsiva del file FCR.sh su tutte le sottodirectory traversabili, così da esplorare per intero la gerarchia 

for i in * 
do 
	if test -d $i -a -x $i 
	then 
		$0 $1/$i $2 $conta $4 $5
	fi 
done
