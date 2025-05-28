#!/bin/sh 
#file comandi ricorsivo relativo alla prima prova parziale del 11 aprile 2014 

cd $1 	#ci spostiamo sulla directory da esplorare 

#la strategia a questo punto è andare a considerare un file come valido se non è vuoto (nuemro di righe diverso da 0) e non contiene righe che NON iniziano con a 

for F in * 
do 
	if test -f $F -a -r $F  	#se si tratta di un file leggibile, allora andiamo ad effettuare gli altri controlli, che necessitano che $F sia un file per funzionare 
	then 
		if test `wc -l < $F` -ne 0 
		then
			if grep -v '^a' < $F > /dev/null 2>&1 	#se troviamo una riga che NON inizia con il carattere 'a' (grazie all'opzione -v), allora passiamo al file successivo 
			then 
				continue  
			else 	#abbiamo un file leggibile non vuoto, in cui non esistono righe che non inizino con 'a' -> file valido -> lo andiamo ad inserire dentro il file temporaneo 
				echo "`pwd`/$F" >> $2
			fi 
		fi 
	fi 
done 

#a questo punto andiamo ad effettuare le chiamate ricorsive dentro la gerachia attuale

for i in * 
do 
	if test -d $i -a -x $i 
	then 
		$0 $1/$i $2 
	fi 
done 

		
