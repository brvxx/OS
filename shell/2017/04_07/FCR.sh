#!/bin/sh 
#file comandi ricorsivo FCR.sh relativo alla prima prova parziale del 7 aprile 2017 

#il file riceve 3 parametri, il primo è la gerarchia su cui andare a cercare, sulla quale ci spostiamo subito, poi il file target per secondo parametro e il terzo è il percorso assulto del file temporaneo su cui salvarsi tutti i nomi assoluti dei file sorted creati 

#partiamo spostandoci sulla gerarchia su cui effettuare la ricerca 

cd $1 

#verifichiamo se la directory corrente contiene file con lo stesso nome del target e se questo è anche leggibile, se si andremo a scrivere il append sul file temporaneo i vari percorsi assoluti dei file sorted creati

if test -f $2 -a -r $2 
then 
	sort -f $2 > sorted 		#usiamo l'opzione -f per far si che il sorting sia case insensitive 
	echo "$(pwd)/sorted" >> $3 	#scriviamo in append sul file temporaneo il percorso assoluto del file sorted creato
fi 

#a questo punto non ci resta che fare le chiamate ricorsive 

for i in * 
do 
	if test -d $i -a -x $i 
	then 
		FCR.sh $(pwd)/$i $2 $3 	#chiamata ricorsiva vera e propria 
	fi 
done 

