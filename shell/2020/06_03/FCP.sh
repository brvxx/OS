#!/bin/sh 
#file comandi principale relativo alla prima prova in itinere del 03 giugno 2020

#controllo lasco dei parametri, N + 1 con N >= 2 -> vogliamo dunque minimo 3 parametri 

case $# in 
0|1|2) 	echo "ERRORE: numero di parametri errato - Attesi almeno 3 parametri - Passati: $#" 
	echo "Usage is: $0 char dirass1 dirass2 ..." 
	exit 1;; 
*) 	echo "DEBUG-FCP.sh: numero di parametri passato, OK!";; 
esac 

#controllo che il primo parametro sia un singolo carattere

case $1 in 
?) 	echo "DEBUG-FCP.sh: primo prametro singolo carattere, OK!";;
*) 	echo "ERRORE: $1 non è un carattere"
	exit 2;; 
esac 
echo 

#salviamo il valore del carattere dentro la variabile C per poter poi shiftare e usare $* come la lista delle sole gerarchie 

C=$1 
shift 

#controllo che le gerarchie passate siano dei nomi assoluti di directory traversabili 

for G 
do 
	case $G in 
	/*) 	if test ! -d $G -o ! -x $G 
		then 
			echo "ERRORE: $G non è una directory traversabile" 
			exit 3
		fi;; 
	*) 	echo "ERRORE: $G non è un nome assoluto" 
		exit 4;; 
	esac 
done 


#aggiorniamo la variabile di ambiente PATH, così da poter effettuare le invocazioni di FCR.sh da qualsiasi punto del file system 

PATH=`pwd`:$PATH 
export PATH 

#creiamo un file temporaneo vuoto per poter andare a salvarci globalmente tutti i percorsi assoluti delle directory valide, per crearlo utilizziamo la ridirezione a vuoto 

> /tmp/nomiassoluti 


#invochiamo ora il file ricorsivo FCR.sh N volte, una per ogni gerarchia passata per parametro 

for G 
do 
	FCR.sh $G $C /tmp/nomiassoluti 
done 


#arrivati a questo punto dovremmo avere un file che contiene globalmente tutte i nomi assoluti delle directory che rispettano le specifiche 

#stampiamo in output ora il numero globale delle directory valide trovate 

echo "numero totale di directory trovate: `wc -l < /tmp/nomiassoluti`" 
echo	 #stampiamo una riga vuota per leggibilità 

#ora chiediamo per ogni directory trovata se si vuole visualizzare il contenuto e in caso positivo lo andiamo a stampare su stdout 

for D in `cat /tmp/nomiassoluti` 
do 
	echo -n "ciao Elena, per caso vuoi visualizzare il contenuto della directory $D? "
	read answer 
	case $answer in
	si|Si|yes|Yes) 		echo ======================================
				ls -lA $D 
				echo ======================================
				echo 
				;;  
	*) 
		echo ======================================
		echo "non verrà mostrato il contenuto della directory"
		echo ======================================
		echo
		;; 
	esac 
done  	


#eliminiamo ora il file temporaneo utilizzato per salvarci le directory valide 

rm /tmp/nomiassoluti 



