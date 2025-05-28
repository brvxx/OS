#!/bin/sh 
#file comandi ricorsivo relativo alla prima prova in itinere del 15 gennaio 2020 

cd $1 	#ci spostiamo nella gerarchia da esplorare 

#scorriamo la directory corrente cercando file validi, in caso positivo andiamo a salvarne il nome assoluto dentro il file contenuto in $3 

for F in * 
do 
	if test -f $F -a -r $F 	#la leggibilità non è una condizione esplicitata nel testo ma senza il controllo del numero di caratteri darebbe errore 
	then 
		if test `wc -c < $F` -eq $2 
		then 
			echo "TROVATO FILE: $1/$F" 	#stampiamo il nome assoluto del file appena trovato 
			echo "$1/$F" >> $3 		#salviamo il suo nome assoluto nel file temporaneo 
		fi 
	fi
done 

#procediamo con la ricerca ricorsiva vera e propria, invocando ricorsivamente il file FCR.sh su tutte le directory traversabili sottodir della dir corrente 

for i in * 
do 
	if test -d $i -a -x $i 
	then 
		$0 $1/$i $2 $3 
	fi 
done 



