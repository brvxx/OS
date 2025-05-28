#!/bin/sh 
#file comandi ricorsivo relativo alla prima prova in itinere del 22 aprile 2021 

cd $1 	#ci spostiamo sulla directory passata da esplorare 

#verifichiamo se esiste almeno un file con estensione .$2, in caso affermativo andiamo a salvare la directory corrente dentro al file, il cui percorso è contenuto in $3

for F in * 
do 
	if test -f $F 	#se siamo in presenza di un file 
	then 
		case $F in 
		*.$2) 	#trovato un file con estensione $2, la directory corrente è valida 
			pwd >> $3 	#aggiungiamo dunque la directory al file temporaneo
			break;; 	#trovato uno ci fermiamo, perché la specifica chiedeva che ce ne fosse ALMENO 1 
		*) 	;; 
		esac
	fi 
done 

#effettuiamo ora le varie invocazioni ricorsive di FCR.sh sulle directory traversabili 

for i in * 
do 
	if test -d $i -a -x $d 
	then 
		$0 $1/$i $2 $3 
	fi 
done 
