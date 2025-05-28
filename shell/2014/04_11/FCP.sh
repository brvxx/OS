#!/bin/sh
#file comandi principale relativo alla primo paraziale del 11 aprile 2014 

#strategia: andremo ad effettuare il controllo sui parametri e poi dal momento che per ogni gerarchia passata per parametro dobbiamo andare a contare un numero di file, quello che faremo sarà andare a creare N file temporanei, 1 per gerarchia, dentro ognuno dei quali andremo ad elencare tutti i file validi relativi a quella gerarchia. 
#il processo ricorsivo andrà a scrivere questi file in forma assoluta dentro i vari tmp, mentre questo file principale servirà appunto per stampare il loro contenuto seguendo l'indicazione dell'utente. 

#controllo lasco dei parametri

case $# in 
0 | 1) 	echo "ERRORE: numero di parametri passato errato - Attesi: 2 o più - Passati: $#" 
	echo "Usage is: $0 Ger1 Ger2 [Ger3] ... "
	exit 1;; 
*) 	echo "DEBUG-FCP.sh: nuermo di parametri passato corretto, OK!";; 
esac 


#controllo che le N gerarchie siano nomi assoluti di directory traversabili

for G 
do 
	case $G in 
	/*) 	if test ! -d $G -o ! -x $G 
		then 
			echo "ERRORE: $G non è una directory traversabile" 
			exit 2 
		fi;; 
	*) 	echo "ERRORE: $G non è un nome assoluto" 
		exit 3;; 
	esac 
done 
echo	#stampa di una riga vuota per leggibilità  

#aggiorniamo la variabile di ambiente PATH, così da potere andare ad effettuare le invocazioni di FCR.sh da qualsiasi punto del file system

PATH=`pwd`:$PATH 
export PATH 	#per compatibilità 

#effettuiamo le N invocazioni al file ricorsivo, ognuna per una singola gerarchia, ricordandoci di creare un file temporaneo diverso per ognuna di queste, così da poterli differenziare, per farlo sfrutteremo una variabile counter. 

n=0 

for G 
do 
	n=`expr $n + 1`
	> /tmp/conta$$_$n 	#creazione del file con la ridirezione a vuoto, usando $$ per distinguere file di esecuzioni diverse e $n per distinguere file dentro la stessa esecuzione 
	FCR.sh $G /tmp/conta$$_$n	#singola invocazione di FCR.sh 
done 


#a questo punto andiamo a stampare le varie gerarchie con il numero di file validi, e per ogni file chiediamo all'utente quante linee vuole visualizzarne 

n=0	#riutilizzeremo questa variabile per fare riferimento ad ogni gerarchia al suo file  

for G 
do 
	n=`expr $n + 1`
	echo "directory: $G, numero di file validi trovati `wc -l /tmp/conta$$_$n`"
	echo 

	#adesso andiamo a fare un for che gira sui singolo nomi di file dentro il temp conta, e stampando le prime X righe volute dall'utente

	for F in `cat /tmp/conta$$_$n` 
	do 
		echo "trovato il file $F"
		echo -n "indicare il numero X di righe che si vogliono visualizzare di questo file: "	#il -n serve per evitare di andare a capo, così che l'utente risponda in-line 
		read X 
		echo 
		
		case $X in 
		*[!0-9]*) 	echo "ERRORE: parametro passato non numerico - non verrà effettuata la stampa di questo file"	#controllaimo che X sia un valore numerico valido  
				continue;; 
		*) ;; 
		esac 

		echo "DEBUG-FCP.sh: effettuiamo la stampa delle prime $X righe del file" 
		echo =================================================================
		head -$X < $F 
		echo =================================================================
		echo 	

	done 
done 

rm /tmp/conta$$_* 	#trucco semplice per evitare di usare un for per eliminare i file uno per uno con un for 
