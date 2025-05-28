#!/bin/sh 
#file comandi ricorsivo relativo alla parte in shell dell'esame totale del 14 luglio 2021

cd $1 	#ci spostiamo sulla directory da esplorare 

cont=0	#variabile counter richiesta nel testo per contare il numero di file validi nella directory corrente 

files= 	#variabile lista che useremmo per invocare l'ipotetica parte di C 

for F in * 
do 
	if test -f $F -a -r $F 	#non è chiesta in modo esplicito la leggibilità ma per contare le linee è necessaria 
	then 
		case $F in 
		??) 	if test `wc -l < $F` -eq $3
			then 
				cont=`expr $cont + 1`
				files="$files $F" 
			fi;; 
		*) ;; 
		esac 
	fi 
done 

#controlliamo che il numero di file validi trovati sia strettamente minore di H ($2) ma maggiore uguale di 2 

if test $cont -lt $2 -a $cont -ge 2 
then 
	echo "TROVATA DIRECTORY con $cont file validi: $1" 

	#invochiamo la parte in C 
	echo "questa sarebbe la parte di invocazione della parte in c, del tipo file.c \$files \$3, con files = $files" 
	echo 
fi 

#invochiamo il file comandi ricorsivo su tutte le sottodirectory della gerarchia per esplorarla per intero 

for i in * 
do 
	if test -d $i -a -x $i 
	then 
		$0 $1/$i $2 $3
	fi 
done

