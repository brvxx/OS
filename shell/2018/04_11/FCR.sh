#!/bin/sh 
#file comandi ricorsivo relativo alla prima prova in itinere del 11 aprile 2018 

cd $1 	#ci spostiamo sulla directory da esplorare 

for i in * 
do 
	if test -f $i -a -r $i 
	then 
		if test `wc -l < $i` -ge $2 
		then 
			if test `wc -l < $i` -ge 5 
			then 
				head -5 $i | tail -1 > $i.quinta 
				echo "$1/$i.quinta" >> $3 
			else 
				> $i.NOquinta 
				echo "$1/$i.NOquinta" >> $3
			fi 
		fi 
	fi 
done 


#effettuiamo ora l'esplorazione ricorsiva della directory corrente, invocando FCR.sh su tutte le sottodir traversabili 

for i in * 
do 
	if test -d $i -a -x $i 
	then 
		$0 $1/$i $2 $3 
	fi 
done 
 
				
