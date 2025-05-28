#!/bin/sh 
#file comandi ricorsivo relativo alla prima prova in itinere del 15 aprile 2016 

cd $1 	#ci spostiamo nella gerachia passata per parametro 

valid=true 	#variabile che metteremo a false nel caso in cui troviamo un file non valido o una sottodirectory 
fcount=0	#variabile che ci permette di verificare che la directory non sia vuota -> se vuota non sarà considerata valida 

for i in * 
do 
	fcount=`expr $fcount + 1`
	
 	if test -f $i -a -r $i 
	then 
		if test `wc -l < $i` -le $2 
		then 
			valid=false
			break 
		fi 
	else 	#caso in cui abbiamo una sottodirectory o comunque un file non leggibile 
		valid=false 
		break 
	fi 
done 

if test $valid = true -a $fcount -ne 0 
then 
	pwd >> $3 	#scritto in append per non sovrascrivere le informazioni del file 
fi 


#chiamate ricorsive per le directory traversabili 

for i in * 
do 
	if test -d $i -a -x $i 
	then 
		$0 $1/$i $2 $3
	fi 
done 
 	 
				




