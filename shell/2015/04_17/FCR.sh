#!/bin/sh 
#file comandi ricorsivo relativo alla prima prova in itinere del 17 aprile 2015

cd $1 	#ci spostiamo sulla gerarchia da esplorare 

#andiamo a salvare dentro il file contenuto in $3 tutti i nomi assoluti dei file che rispettano le specifiche date nel testo 

for i in * 
do 
	if test -f $i -a -r $i 
	then 
		if test `grep 't$' $i | wc -l` -ge $2 	#trovato un file valido, il suo numero di linee che terminano con t è almeno $2 ossia X  
		then 	
			echo "$1/$i" >> $3 
		fi 
	fi 
done 

#invochiamo il file ricorsivo per ogni sottodirectory trovata, così da esplorare per inero la gerarchia 

for i in * 
do 
	if test -d $i -a -x $i 
	then 
		$0 $1/$i $2 $3 
	fi 
done 


