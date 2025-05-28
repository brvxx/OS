#!/bin/sh
#file comandi ricorsivo relativo alla prima prova in itinere del 13 aprile 2012

cd $1	#necessariamente partiamo con lo spostarci sulla gerarchia da esplorare 

conta=`expr $2 + 1`	#creiamo una variabile LOCALE all'esecuzione corrente che tiene traccia del livello corrente, incrementando di 1 il livello della 
			#gerarchia "superiore", dalla quale è stato chiamato, ovviamente alla partenza $2 = 0, dunque la radice della gerarchia implicitamente
			#sarà considerata il lvl 1 

#a questo punto procediamo con la fase di aggiornamento del valore del file nel caso abbiamo trovato un livello maggiore di quello massimo corrente (fase A) 

if test $3 = A 
then 

	read lvl_tot < /tmp/conta_livelli 	#salviamo il valore corrente del numero totale di livelli dentro una variabile e confrontiamo di non trovarci 
						#ad un livello più in basso, perchè se così fosse dovremmo andare ad aggiornare il valore contenuto nel file 

	if test $conta -gt $lvl_tot 
	then 
		echo $conta > /tmp/conta_livelli
	fi 
fi 

#fase B, controlliamo se ci troviamo su una directory del livello esplicitato dall'utente, in caso affermativo stampiamo il suo percorso e il contenuto 
if test $3 = B 
then 
	if test $conta -eq $4 
	then 
		echo "TROVATA DIRECTORY del livello $4: $1, e il suo contenuto é: " 
		ls -l 
		echo
	fi
fi

#fase ricorsiva comune ad entrambe le fasi, quindi non necessitiamo di controlli sul tipo di fase 

for i in * 
do 
	if test -d $i -a -x $i 
	then 
		$0 $1/$i $conta $3 $4 	#mettiamo anche il quarto parametro per renderlo universale con la fase B, non dà problemi 
	fi 
done 	

