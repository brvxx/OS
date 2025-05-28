#!/bin/sh 
#file comandi ricorsivo relativo alla prima prova in itinere del 13 aprile 2022

cd $1 	#ci spostiamo sulla gerarchia da esplorare 

#cerchiamo i file LEGGIBILI che abbiano lunghezza in linee pari a $2 ossia X del FCP, una volta che ne troviamo uno possiamo andare a scriverlo in append sul file temporaneo 
for F in * 
do 
	if test -f $F -a -r $F 
	then 
		if test `wc -l < $F` -eq $2 
		then 
			echo "$1/$F" >> $3 
		fi 
	fi 
done 

#invochiamo ricorsivamente il file FCR.sh su tutte le sottodirectory così da esplorare fino a fondo la gerarchia 

for i in * 
do
	if test -d $i -a -x $i 
	then 
		$0 $1/$i $2 $3 
	fi 
done 

