#!/bin/sh 
#file comandi ricorsivo (FCR.sh) relativo alla parte di shell dell'esame completo del 17 gennaio 2018

#prima cosa che facciamo è andare a spostarci nella gerarchia su cui effettuare la ricerca

cd $1 

# a questo punto andiamo a iterare su tutti i file leggibili della directory corrente, tenendo il conto dei file "validi" ossia che contengano un numero di linee pari a $3 e almeno un occorrenza del carattere 4$4, per farlo useremo una variabile counter chiamata count 

count=0

for i in * 
do 
	if test -f $i -a -r $i
	then 
		line_num=$(wc -l < $i 2>/dev/null) 	#si tratta di una variabile che va a contenere ad ogni iterazione il numero di righe del file corrente, per poter compararlo con $3 
							#nota che la ridirezione dello stderr non è strettamente necessaria, infatti dal momento che $i è sicuramente (per controlli) un file leggibile, non 
							#ci dovrebbero essere errori, ma prevenire è megio che curare

		if test $line_num -eq $3 && grep $4 $i > /dev/null 2>&1 	#qua la ridirezione invece è necessaria affinché non vengano stampate in output tutte le occorrenze 
		then 
			count=$(expr $count + 1) 
		fi 
	fi 
done 

#controlliamo ora se il nuemero di file validi (contenuto in count) sia uguale a $2, perché se così fosse, allora potremmo andare a stampare la directory corrente 

if test $count -eq $2 
then 
	echo "TROVATA directory che soddisfa le specifiche: $(pwd)" 
fi 

#passaimo alla parte ricorsiva, dove andremo ad invocrare ricorsivamente il file FCR.sh per ogni sottodir traversabile della dir corrente 

for i in * 
do 
	if test -d $i -a -x $i 
	then 
		$0 $(pwd)/$i $2 $3 $4
	fi 
done 

