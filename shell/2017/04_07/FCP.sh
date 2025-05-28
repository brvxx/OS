#!/bin/sh 
# file comandi FCP.sh realtivo alla prima prova in itine tenuta nel 7 aprile 2017

#partiamo effettuando un controllo lasco dei parametri passati, infatti sicuramente neccessitiamo di 1 nome file, e di un numero variabile di parametri N, con N maggiore uguale a 2, il che significa che sicuramente (a prescindere dal tipo di questi) avremo a che fare con almeno 3 parametri. 

case $# in 
0|1|2) 	echo "ERRORE - numero di parametri passato errato: aspettati almeno 3 parametri, passati: $#" 
	echo "Usage is: $0 filerel dirass1 dirass2 ... " 
	exit 1;; 
*) 	echo "DEBUG-FCP.sh: numero di parametri passati ok: $#";; 
esac 

#controllo che il primo parametro sia stato passato in forma relativa semplice 

case $1 in 
*/*) 	echo "ERRORE - $1 non è in forma relativa semplice" 
	exit 2;; 
*)	;; 
esac 

#adesso bisogna effettuare il controllo sulle varie directory, bisogna verificare che siano in forma assoluta, e che siano delle directory traversabili, per farlo necessitiamo di un for che giri sulle directory, ma non possiamo usare $* senza opportune modifiche, perchè questa lista contiene anche il nome relativo semplice del file target, quindi effettuiamo uno shift, salvandoci però il contenuto del primo parametro 

F=$1 
shift 

for i 
do 
	case $i in 
	/*) 	
		if test ! -d $i -o ! -x $i 
		then 
			echo "ERRORE - $i non è una directory traversabile" 
			exit 3
		fi;; 
	*) 	
		echo "ERRORE - $i non è una directory in forma assoluta"
		exit 4;; 
	esac
done 

#arrivati a questo punto siamo sicuri di avere minimo 3 parametri, di cui un nome in forma relativa semplice che rappresenta il file target e almeno 2 directory traversabili in forma assoluta

#adesso non ci resta che creare un file temporaneo che ci permetta di andare a salvare tutti i risultati delle azioni di sorting compiute COMPLESSIVAMENTE dal file ricorsivo


> /tmp/conta$$ 	#creiamo il file temporaneo attraverso una ridirezione a vuoto, usiam $$ ossia il PID del processo corrente per questioni di debug, così che se per un qulche motivo ci serva controllare o conforntare le varie esecuzioni di FCP.sh, possiamo confrontarle attraverso i diversi file temporanei

#aggiorniamo il valore della variabile PATH per poter effettuare le chiamate ricorsive

PATH=$(pwd):$PATH 
export PATH 	#la esportiamo per compativbilità 


#iniziamo le chiamate al file ricorsivo: 

for i 
do 
	echo "DEBUG-FCP.sh: inizio della ricerca su: $i" 
	FCR.sh $i $F /tmp/conta$$ 	#passato al file ricorsivo la i-esima gerarchia, il file target e il file tmp su cui scrivere 
done 

echo "DEBUG-FCP.sh: terminata ricerca ricorsiva" 

#adesso quello che si fa è andare a contare quanti sono globalmente questi file sorted creati 

echo
echo "numero di file totali trovati = $(wc -l < /tmp/conta$$)" 
echo 

#stampa dei vari percorsi dei file sorted generati e della loro linea iniziale e finale 

for i in $(cat /tmp/conta$$) 
do 
	echo "creato il file: $i" 
	echo 
	echo "la prima linea è:"
	echo =============================
	head -1 $i 
	echo ============================= 
	echo 
	echo "l'ultima linea è:" 
	echo =============================
	tail -1 $i 
	echo =============================
	echo
	echo 
		#nota che i vari echo messi senza stringa stampano una linea vuota per leggibilità  
done 

#a questo punto dal momento che il file temporaneo non ci serve più possiamo andarlo a rimuovere, commenta eventualmente sta parte per poter effettuare del debug 

rm /tmp/conta$$ 	


