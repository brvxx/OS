#!/bin/sh 
#file comandi principale relativo alla prima prova in itinere del 15 aprile 2016

#controllo lasco del numero di parametri passati, N + 1 con N >= 2, quindi avremo bisogno di almeno 3 parametri passati

case $# in 
0|1|2) 	echo "ERRORE: numero di parametri passato errato - Attesi almeno 3 - Passati $#" 
	echo "Usage is: $0 dirass1 dirass2 [dirass3] ... num" 
	exit 1;; 
*) 	echo "DEBUG-FCP.sh: numero di parametri passato corretto, OK!"
	;; 
esac 

#andiamo a mettere le N gerarchie passate dentro una variabile GER fatta a lista e l'ulitmo valore numrico dentro la variabile X, per farlo sfrutteremo una variabile conteggio chiamata count, inizializzata a 0, che ad ogni parametro trovato incrementa di 1 il suo valore 

#sfrutteremo questo processo per andare anche a controllare che le N gerarchie passate per parametro siano nomi assoluti di directory traversabili e che il valore X sia un numero intero strettametne positivo

count=0

X= 
GER= 

for i 
do 
	count=`expr $count + 1`
	
	if test $count -ne $#
	then 
		case $i in 
		/*)	if test ! -d $i -o ! -x $i 
			then 
				echo "ERRORE: $i non è una directory traversabile" 
				exit 2
			fi;; 
		*) 	echo "ERRROE: $i non è un nome assoluto" 
			exit 3;; 
		esac 

		#passato questo controllo avremo sicuramente una directory traversabile in forma assoluta, la mettiamo dentro GER 
		GER="$GER $i"  
	else 
		case $i in 
		*[!0-9]*) 	echo "ERRORE: $i non è un valore numerico, oppure è un numero decimale o negativo" 
				exit 4;; 
		*)	
			if test $i -eq 0 
			then 
				echo "ERRORE: $i non è strettamente positivo in quanto nullo" 
				exit 5
			fi;; 
		esac 

		#arrivati a questo punto avremo sicuramente un valore numerico intero strettamente postivo, possiamo metterlo dentro a X 
		X=$i 
	fi 
done 
echo "DEBUG-FCP.sh: tipo dei parametri corretto, OK!" 
echo	#stampa di una riga vuota per leggibilità 

#aggiorniamo la variabile di ambiente PATH e la esportiamo per compatibiità 

PATH=`pwd`:$PATH 
export PATH 

#creiamo un file temporaneo che servirà per andare a salvare tutte le directory valide trovate duraente l'esplorazione delle gerarchie. Lo creiamo con la ridirezione a vuoto 

> /tmp/conta$$ 


#invochiamo il file comandi ricorsivo N volte, una per ogni gerarchia passata per parametro 

for G in $GER 
do 
	FCR.sh $G $X /tmp/conta$$
done 


#arrivati a questo punto avremo un file conta$$ che contiene tutte le directory valide in forma assoluta, andiamo dunque a stampare il numero totale di directory trovate

echo "Numero totale di directory valide trovate: `wc -l < /tmp/conta$$`" 
echo 
echo

#per ogni directory trovata si va ora a stamparne il nome assoluto e i vari file assieme alla loro X-esima riga partendo dal fondo 

for D in `cat /tmp/conta$$`
do 
	cd $D
	echo "contenuto directory valida $D : " 
	echo 
	
	for F in * 
	do 
		echo "- $D/$F"
		echo
		echo "contenuto della riga $X del file partendo dal fondo: " 
		echo ======================================================
		tail -$X $F | head -1 
		echo ======================================================
		echo 
	done
	echo  
done 

rm /tmp/conta$$ 	#rimuoviamo il file temporaneo  
		



