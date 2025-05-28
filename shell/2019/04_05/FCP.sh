#!/bin/sh 
#file comandi principale relativo alla prima prova in itinere del 5 aprile 2019 (turni 5 e 6) 

#controllo lasco del numero di parametri, N + 1 con N >= 2, quindi al file comandi principale dovranno essere passati almeno 3 parametri 

case $# in 
0|1|2) 	echo "ERRORE: numero di parametri passato errato - Attesi almeno 3 - Passati $#" 
	echo "Usage is: $0 dirass1 dirass2 [dirass3] ... string" 
	exit 1;; 
*) 	
	echo "DEBUG-FCP.sh: numero di parametri passato corretto, OK!" 
	;; 
esac 

#controllo sulle gerarchie (nomi assoluti di directory traversabili) e la stringa (non contenente / per evitare ambiguità nei percorsi), durante il controllo andremo a salvare le prime N gerarchie dentro ad una variabile lista GER, mentre la stringa in S, per poter effettuare questo processo necessitiamo di una variabile counter count, che inizializziamo a 0 e per ogni parametro considerato verrà incrementata di 1

count=0 

S= 
GER= 

for i 
do 
	count=`expr $count + 1`

	if test $count -ne $# 	#caso in cui non siamo sull'ultimo parametro (dunque quando siamo sulle gerarchie) 
	then 
		case $i in 
		/*) 	if test ! -d $i -o ! -x $i 
			then 
				echo "ERRORE: $i non è una directory traversabile" 
				exit 2
			fi;; 
		*) 	
			echo "ERRORE: $i non è un nome assoluto"
			exit 3;; 
		esac 

		GER="$GER $i" 

	else 	#caso in cui ci troviamo sull'ultimo parametro (la stringa) 
		case $i in 
		*/*) 	echo "ERRORE: $i contiene il carattere /, proibito nei nomi di file" 
			exit 4;; 
		*) 	;; 
		esac 
		
		S=$i 
	fi 
done 
echo "DEBUG-FCP.sh: tipo dei parametri passato corretto, OK!" 
echo "debug $GER"  
echo

#arrivati a questo punto abbiamo un numero corretto di parametri, le N gerarchie sono correttametne dei nomi assoluti di directory traversabili e sono listate dentro la variabile GER, mentre la stringa passata come ultimo parametro si trova dentro la variabile S 


#aggiorniamo la variabile PATH inserendo tra i percorsi quello della directory corrente contenente FCR.sh e poi la esportiamo per compatibilità 

PATH=`pwd`:$PATH 
export PATH 


#creiamo un file temporaneo in cui andare a salvare per ogni file trovato valido trovato globalmente durante l'esplorazione delle gerarchi la lunghezza in caratteri e il nome assoluto 

> /tmp/conta$$ 

#invochiamo il file comandi ricorsivo N volte, una per ogni gerarchia passata per parametro

for G in $GER 
do 
	FCR.sh $G $S /tmp/conta$$
done 

#arrivati a questo punto abbiamo un file conta$$ che contiene prima il numero di caratteri di un certo file e in seguito il suo nome assoluto
#stampiamo in output il numero totale di file trovati 

echo "numero totale di file trovati globalmente: `wc -l < /tmp/conta$$`" 
echo "di seguito andremo ad elencare per ogni file il numero di caratteri seguito dal suo nome assoluto" 
echo
echo 

count=0 	#riutilizziamo la variabile count per determinare se ci troviamo su una lunghezza in caratteri (posizione dispari) o su un nome (posizione pari) 

for i in `cat /tmp/conta$$`
do
	count=`expr $count + 1`
	
	if test `expr $count % 2` -eq 0		#caso in cui ci troviamo su un nome 
	then 
		echo "nome assoluto del file trovato: $i" 
		echo -n "si vuole procedere con l'ordinamento del file? (si/no)  " 
		read answer 	#variabile che contiene la risposta dell'utente  
		
		case $answer in 
		si|Si) 	echo
			echo "file ordinato in ordine alfabetico senza differenziare maiuscole e minuscole:" 
			echo =====================================
			sort -f $i 
			echo =====================================
			echo
			;; 
		*) 	
			echo "non verrà mostrato il contenuto ordinato del file, come da richiesta"
			echo 
			;; 
		esac 
	else 	#caso in cui ci troviamo sulla lunghezza in caratteri
		echo "numero di caratteri del file: $i" 
	fi 
done 

rm /tmp/conta$$ 	#cancelliamo il file temporaneo

