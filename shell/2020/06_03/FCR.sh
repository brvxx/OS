#!/bin/sh 
#file comandi ricorsivo relativo alla prima prova in itinere del 03 giugno 2020 

#ci spostiamo sulla gerarchia passata per parametro 

cd $1 

#a questo punto controlliamo che la directory corrente sia valida con un case 

case $1 in 
*/$2?$2) 	#trovata una directory valida, allora andiamo andiamo a scrivere il suo percorso assoluto in append sul file temporaneo passato per parametro 
		pwd >> $3;; 
*) ;; 
esac 

#invocazione ricorsiva sulle sottodirectory della gerarchia stessa 

for i in *
do 
	if test -d $i -a -x $i
	then  
		$0 $1/$i $2 $3 
 	fi 
done 
