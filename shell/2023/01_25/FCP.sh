#!/bin/sh
#file comandi principale (FCP.sh) relativo alla parte di shell dell'esame del 25 gennaio 2023 

#controllo stretto del numero di parametri attraverso un case 

case $# in 
3) 	echo "DEBUG-FCP.sh: numero di parametri passato corretto, OK!";; 
*) 	echo "ERRORE: numero di parametri passato errato - Attesi 3, passati: $#" 
	echo "Usage is: $0 dirass ext1 ext2"
	exit 1;;  
esac 

#controllo sul primo parametro, che deve essere il nome assoluto di una gerarchia dentro al file system, quindi dovremo verificarne l'esistenza e implicitamente ci viene chiesto di controllarne la traversabilità, infatti se no non ci potremmo spostare dentro per dare via alla ricerca 

case $1 in 
/*) 	if test ! -d $1 -o ! -x $1 
	then 
		echo "ERRORE: $1 non è una directory traversabile" 
		exit 2
	fi;;
*) 	echo "ERRORE: $1 non è un nome assoluto" 
	exit 3;; 
esac 


#sulle estensioni in realtà non ci sarebbe molto da controllare, infatti non esiste concretametne in concetto di estensione su UNIX, in quanto la tipologia del file non è determinata a partire dall'estensione ma dal contenuto stesso del file, quindi qualsiasi tipo di stringa potrebbe essere considerata un estensione. Noi sappiamo che sostanzialmente i parametri che vengono passati al file comandi sono delle stringhe, quindi a livello di tipo non ci sono problemi, unica cosa degna di nota è andare eventualmente a controllare che queste stringhe non contengano il carattere '/', in quanto non possiamo avere un estensione di questo tipo, se così fosse verrebbe interpretata dall'OS come uno switch tra directory.

#per comodità (evitare di riscrivere lo stesso codice per i due parametri $2 e $3, andiamo a salvarci la gerarchia dentro la variabile G (nome dato nel testo) per poi effettuare uno shift sulla lista di parametri, in modo tale da iterare su entrambi i parametri dentro un for, senza dover riscrivere il codice

G=$1 
shift 

for i 
do 
	case $i in 
	*/*) 	echo "ERRORE: la stringa $i non è valida come estensione perché contiene il carattere /" 
		exit 4;; 
	*) 	echo "DEBUG-FCP.sh: stringa $i valida come estensione, OK!";; 
	esac 
done 

# a questo punto, sebbene non sia esplicitato nel testo, possiamo andare ad effettuare un controllo tra le 2 stringhe e verificare che non siano uguali, infatti l'esercizio ha senso solo nella misura in cui le due estensioni siano diverse, tipo se avessimo E1 = E2, quello che accadrebbe è che non troveremo mai file che soddisfino la seconda condizione, perché per essere leggibile e scrivibile e avere estensione .E2, necessariamente deve essere leggibile e avere estensione .E1 = .E2, quindi soddisfa la condizione 1. Insomma... si creerebbero dei comportamenti inaspettati.  

if test $1 = $2 
then 
	echo "ERRORE: le stringhe passate come estensione sono identiche, potrebbero generare un comportamento inaspettato del programma" 
	exit 5
fi 
echo 

#aggiornamento della variabile PATH -> inseriamo la directory corrente per permettere le chiamate ricorsive di FCR.sh da qualunque punto del file system 

PATH=$(pwd):$PATH 
export PATH 
#invochiamo il file ricorsivo FCR.sh passando tutti i parametri passati inizialmente a FCP.sh, nota che il comportamento del programma è organizzato in una singola fase, dunque effettueremo una sola chiamata ricorsiva

FCR.sh $G $*

echo 
echo "DEBUG-FCP.sh: FINITO TUTTO!" 


