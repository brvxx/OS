#!/bin/sh 
#file comandi ricorsivo relativo alla parte di shell dell'esame del 9 febbraio 2015

cd $1 	#ci spostiamo nella gerarchia da esplorare 

#scorriamo i file della gerarchia cercando quelli validi, dal momento poi che dovremmo anche stampare le directory che contengono almeno 1 di questi file, useremo una variabile booleana trovato 

trovato=false 

for i in * 
do 
	if test -f $i -a -r $i 
	then 
		if test `wc -l < $i` -eq $2 
		then 
			echo "$1/$i" >> $3 
			trovato=true
		fi 
	fi 
done 

#se abbiamo trovato almeno un file allora stampiamo la directory corrente 

if test $trovato = true 
then 
	echo "TROVATA DIRECTORY contenente almeno un file valido: `pwd`" 
fi 

#effettuiamo l'esplorazione ricorsiva della directory corrente invocando il file comandi FCR.sh sulle sottodir 

for i in * 
do
	if test -d $i -a -x $i 
	then 
		$0 $1/$i $2 $3 
	fi 
done 

