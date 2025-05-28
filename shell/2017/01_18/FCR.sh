#!/bin/sh 
#file comandi ricorsivo relativo alla prova di esame completa (parte shell) del 18 gennaio 2017 

cd $1 	#partiamo col spostarci sulla directory che vogliamo esplorare 

#per considerare valida una certa directory dobbiamo andare a verificare che questa abbia nome relativo semplice corrispondente a $2 e che contenga almeno un file contente l'occorrenza di almeno 1 carattere numerico. Il problema di questo controllo nasce dal fatto che va effettuato anche sulla directory che rappresenta la gererachia stessa, quindi si usa un case che vada a fare un controllo sul nome assoluto della directory corrente, e si guarda se questo termini con $2 

#si noti inoltre che vogliamo evitare di andare a stampare più volte la stessa directory nel caso questa contenga più file "validi", quindi usufruiamo di una variabile "booleana", 'found' che ci indicherà se la directory corrente contiene o meno un file valido, così che al termine del for possiamo stamparne il percorso assoluto su stdout

found=false
 
case $1 in 
*/$2) 		#caso in cui la directory corrente ha nome relativo semplice uguale a $2, vogliamo stamparla sse contiene almeno un file con 1 carattere numerico 
	for i in * 
	do 
		if test -f $i -a -r $i 	#controlliamo anche che sia leggibile perché dovremo utilizzare il grep per vedere se contiene un carattere numerico 
		then 
			if grep '[0-9]' $i > /dev/null 2>&1 	#se troviamo un occorrenza allora ci salviamo il nome assoluto del file e settiamo trovato a true
								#nota che ridirezioniamo tutto l'output verso il device null perché non vogliamo stampe inutili  
			then 
				echo "$(pwd)/$i" >> $3
				found=true
			fi 
		fi 
	done;; 
*) 	;; 	#se la directory corrente non ha $2 come nome relativo semplice, sicuramente found sarà false ma non dobbiamo fare niente di più 
esac

#quello che facciamo ora è andare a stampare su stdout il pathname assoluto della directory corrente se la variabile found è true 

if test $found = true 
then 
	echo "TROVATA UNA DIR che verifica le specifiche: $(pwd)" 
fi 

#effettuiamo ora la chiamata ricorsiva sulle sottodirectory della dir corrente: 

for i in * 
do 
	if test -d $i -a -x $i 
	then 
		$0 $1/$i $2 $3
	fi 
done 
		




