#!/bin/sh 
#file comandi ricorsivo relativo alla prima prova in itinere del 5 aprile 2019 (turni 5 e 6) 

cd $1 	#ci spostiamo sulla gerarchia da esplorare 

#nota che non è necessario andare a fare un for che scorra tutti i file e cerca "quelli" validi dentro la directory corrente, infatti CE NE SARA' AL MASSIMO 1, non ci possono essere dentro la stessa directory 2 file con lo stesso nome, dunque al posto di usare un for con il pattern matchin, ci basterà usare un semplice if col test 

if test -f $2.txt -a -r $2.txt -a -w $2.txt 	#se nella directory corrente esiste il file S.txt leggibile e scrivibile 
then 
	echo "`wc -c < $2.txt` `pwd`/$2.txt" >> $3 	#andiamo a metterli sulla stessa linea così che per contare il numero totale di file non dovremo fare / 2 
fi 

#esploriamo attraverso la ricorsione tutte le sottodir della directory corrente 

for i in * 
do 
	if test -d $i -a -x $i 
	then 
		$0 $1/$i $2 $3 
	fi 
done
