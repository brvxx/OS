#!/bin/sh 
#file comandi principale relativo alla parte shell dell'esame totale del 11 settembre 2024

#controllo lasco del numero di parametri passato, Q + 1 con Q >= 2, dunque ci aspettiamo almeno 3 parametri 

case $# in 
0|1|2) 	echo "ERRORE: numero di parametri passato errato - Attesi almeno 3 - Passati $#" 
	echo "Usage is: $0 numpos dirass1 dirass2 [dirass3] ... " 
	exit 1;; 
*) 
	echo "DEBUG-FCP.sh: numero di parametri passato corretto, OK!" 
esac 

#controllo che il primo parametro sia un numero intero strettamente positivo, se così è, allora salviamo il suo valore dentro una variabile X, così da poter effettuare uno shift dei parametri e andare a lavorare su $* in quanto lista di sole gerarchie 

case $1 in 
*[!0-9]*) 	echo "ERRORE: $1 non e' un parametro numerico, oppure e' un valore negativo o non intero" 
		exit 2;; 
*) 
	if test $1 -eq 0 
	then 
		echo "ERRORE: $1 non e' un valore numerico strettamente positivo in quanto nullo" 
		exit 3
	fi;; 
esac 

X=$1 
shift 

#controllo delle Q gerarchie passate per parametro, si controlla che queste siano dei nomi assoluti di directory traversabili 

for G 
do 
	case $G in 
	/*) 	if test ! -d $G -o ! -x $G 
		then 
			echo "ERRORE: $G non e' una directory traversabile" 
			exit 4
		fi;; 
	*) 
		echo "ERRORE: $G non e' un nome assoluto" 
		exit 5;; 
	esac 
done 
echo "DEBUG-FCP.sh: tipo dei parametri passato corretto, OK!" 
echo 	#stampa di una riga vuota per leggibilità 


#aggiornamento della variabile PATH aggiungendo la directory corrente ed esportando per compatibilità 

PATH=`pwd`:$PATH 
export PATH 

#creiamo due file temporanei uno per le directory trovate, in cui verranno inserite tutte i nomi assoluti delle directory trovate a livello X durante l'esplorazione delle gerarchie, e uno per i file, salvati in forma relativa semplice rispetto alla loro directory di appartenenza 

> /tmp/nomiDirectory$$ 
> /tmp/nomiFile$$

#invochiamo ora il file comandi ricorsivo per ognuna delle Q gerarchie passate per parametro, passando anche una variabile conta inizializzata a 0 che servirà per tenere traccia dei livelli durante l'esplorazione delle gerarchie 

for G 
do 
	conta=0 

	FCR.sh $G $X $conta /tmp/nomiDirectory$$ /tmp/nomiFile$$ 
done 

#stampiamo il numero totale di directory trovate al livello X nelle Q gerarchie e poi, anche se non scritto nel testo, per chiarezza andiamo pure a stampare tutti i nomi assoluti delle directory trovate 

echo "numero totale di directory trovate al livello $X: `wc -l < /tmp/nomiDirectory$$`" 
echo "DEBUG-FCP.sh: tali directory sono: " 
cat /tmp/nomiDirectory$$
echo 
echo 

#a questo punto procediamo con l'invocazione della parte di C, che prevede che venga chiamata da dentro le directory trovate e alla quale deve essere passato il nome assoluto di tale directory e i nomi relativi semplici di tutti i file validi trovati 

echo 'DEBUG-FCP.sh: invocazioni della parte in C, con sintassi: fileC $d `cat /tmp/nomiFile$$`'
echo 

for d in `cat /tmp/nomiDirectory$$`
do 
	cd $d 	#come da specifica la parte C va invocata da dentro queste directory trovate 

	#a questo punto avverrebbe l'invocazione della parte in C, della quale però noi andremo solo a fare echo 
	echo fileC $d `cat /tmp/nomiFile$$` 
	echo 
done 

#rimuoviamo i 2 file temporanei creati 

rm /tmp/nomiDirectory$$
rm /tmp/nomiFile$$ 
