#!/bin/sh 
#file comandi principale relativo alla prima prova in itinere del 13 apile 2022

#controllo lasco del numero di parametri, Q + 1 con Q >= 2, quindi dovremmo avere almeno 3 parametri passati a questo file comandi 

case $# in 
0|1|2) 	echo "ERRORE: numero di parametri passati errato - Attesi almeno 3 - passati $#" 
	echo "Usage is: $0 numpos dirass1 dirass2 [dirass3] ... " 
	exit 1;; 
*) 	echo "DEBUG-FCP.sh: numero di parametri passato corretto, OK!" 
	;; 
esac 

#controllo sul primo parametro (numero intero strettamente positivo) 

case $1 in 
*[!0-9]*) 	echo "ERRORE: $1 non è un valore numerico, oppure è un numero negativo o decimale" 
		exit 2;; 
*) 	
	if test $1 -eq 0 
	then 
		echo "ERRORE: $1 non è strettmente positivo in quanto nullo" 
		exit 3 
	fi;; 
esac 

X=$1 	#salviamo il valore del parametro numerico dentro una variabile, per poter effettuare uno shift e ottenere dentro $* tutte e sole le gerarchie 
shift 

#controllo che le Q nomi passati per parametro siano nomi assoluti di directory traversabili 

for G 
do 
	case $G in 
	/*) 	if test ! -d $G -o ! -x $G 
		then 
			echo "ERRORE: $G non è una directory traversabile" 
			exit 4 
		fi;; 
	*) 
		echo "ERRORE: $G non è un nome assoluto" 
		exit 5;; 
	esac 
done 
echo "DEBUG-FCP.sh: tipo dei parametri passati corretto, OK!" 
echo 

#aggiorniamo la variabile di ambiente PATH, aggiungendo la directory corrente 

PATH=`pwd`:$PATH 
export PATH	#per compatibilità 

#andiamo ad effettuare le Q invocazioni al file comandi ricorsivo FCR.sh, una per ogni gerarchia passata per parametro, passando ad ogni iterazione un nuovo file temporaneo, così da ottenere Q file ognuno dei quali contiene i file validi relativi alla rispettiva gerarchia, così da poter effettuare il controllo alla fine 

n=1 	#variabile che useremo per differenziare i vari file temporanei e per contare le singole fasi dentro il ciclo 

for G 
do 
	> /tmp/nomiAssoluti_$n

	echo "DEBUG-FCP.sh: invocazione del file FCR.sh relativa alla fase $n sulla gerarchia $G" 
	FCR.sh $G $X /tmp/nomiAssoluti_$n
	
	#stampiamo il numero di file validi trovati nella gerarchia appena esplorata 
	echo "numero totale di file di $X righe trovati nella gerarchia $G: `wc -l < /tmp/nomiAssoluti_$n`" 
	echo 
	#incrementiamo il valore del counter 
	n=`expr $n + 1`
done 
echo 

#arrivati a questo punto abbiamo Q file temporanei, uno per ogni gerarchia passata, all'interno dei quali sono contenuti tutti i file validi trovati dentro alla rispettiva gerarchia, a questo punto dobbiamo andare a confrontare tutti i file della PRIMA GERARCHIA, con TUTTI i file di TUTTE le altre gerarchie e dire se il contenuto è uguale o meno 
	

for file1 in `cat /tmp/nomiAssoluti_1` 	#for che gira sui file della prima gerarchia passata 
do 
	echo "- file considerato della gerarchia 1: $file1" 
	echo

	n=1 	#variabile counter che ci tiene traccia della gerarchia con la quale confrontiamo la prima e ci permette di accedere al relativo file 
		
	for ger 	#for che gira sui parametri del file, dunque le gerarchie, ATTENZIONE! la prima compresa
	do 
		if test $n -ne 1 	#se nel secondo for siamo sulla prima gerarchia salta il controllo perché non avrebbe senso 
		then	
			echo "CONFRONTO COI FILE DELLA GERARCHIA $n" 
			echo 

			for file2 in `cat /tmp/nomiAssoluti_$n`		#for che gira sui file dell'n-esima gerarchia da confrontare con la prima
			do
				echo "stiamo per confrontare il contenuto dei file: $file1 e $file2" 

				if diff $file1 $file2 > /dev/null 2>&1 	#comando diff ritorna 0 (successo) se i due file hanno lo stesso contenuto 
				then 
					echo "i due file ammettono lo STESSO CONTENUTO"  
				else 
					echo "i due file ammettono CONTENUTO DIFFERENTE"
				fi
			done
			echo  
		fi
	
		n=`expr $n + 1`
	done 
	echo
	echo
done 
