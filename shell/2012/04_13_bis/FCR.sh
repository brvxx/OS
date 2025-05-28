#!/bin/sh 
#file comandi ricorsivo relativo alla prima prova in itinere del 13 aprile 2012 - versione senza file temporaneo 


cd $1 	#ci spostiamo sulla gerarchia passata da esplorare 

count=`expr $2 + 1`	#variabile locale che ci segnala il livello della directory corrente nella gerarchia 

lvl_max=$count 		#variabile che tiene salva al suo interno il livello più elevato, partiamo ipotizzando che sia quello corrente, il valore verrà poi
			#aggiornato man mano che l'esplorazione procede 

#FASE B: è un po' strano andare a metterla sopra la fase A, ma questo è semplicemente per far si che la ricerca delle directory valide (fase B) avvenga in pre-order come siamo abituati e non in post-order (quello che accadrebbe se spostassimo sto blocco di codice sotto quello delle call ricorsive) 

if test $3 = B 	#effettuiamo un controllo sulla fase, questo blocco di codice deve esegure sse siamo nella fase B 
then 
	if test $count -eq $4 	#allora siamo su una directory del livello esplicitato dall'utente 
	then 
		echo "DIRECTORY TROVATA nel livello $4: $1, il cui contenuto è " 
		ls -l
		echo  
	fi 
fi 



#procediamo con l'esplorazione ricorsiva della gerarchia (comune a tutte e due le fasi) e nel mentre andiamo ad aggiornare il valore del livello massimo sfruttando il valore di ritorno di ogni invocazione del file comandi ricorsivo (FASE A) 

for i in *
do 
	if test -d $i -a -x $i 
	then 
		$0 $1/$i $count $3 $4
		
		ret=$? 	#salviamo il valore di ritorno dell'invocazione dentro una variabile, questo valore è sostanzialmente il valore massimo di livello 
			#trovato esplorando interamente la singola directory corrente nel ciclo 

		if test $ret -gt $lvl_max	#se abbiamo trovato un valore maggiore per il livello massimo, aggiorniamo dunque lvl_max 
		then 
			lvl_max=$ret 
		fi 
	fi 
done 

exit $lvl_max 
