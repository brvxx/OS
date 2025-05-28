#!/bin/sh 
#file comandi principale realtivo alla prima prova in itinre del 22 aprile 2021, si tratta di un'altra versione in cui andiamo a fare un loop che continua a richiedere il valore numerico X fintanto che non viene fornito giusto.   

#controllo lasco dei parametri, poichè questi devono essere Q + 2 e Q >= 2, necessariamente dovremo avere almeno 4 parametri 

case $# in 
0|1|2|3) 	echo "ERRORE: Numero di parametri passato errato - Parametri attesi: almeno 4 - Passati: $#" 
		echo "Usage is: $0 num string dirass1 dirass2 [dirass3] ..." 
		exit 1;; 
*) 	echo "DEBUG-FCP.sh: Numero di parametri passato corretto, OK!";; 
esac 

#controllo che il primo parametro sia un numero intero strettamente positivo, salviamo poi il parametro dentro una variabile W 

case $1 in 
*[!0-9]*) 	echo "ERRORE: $1 non è un numero, oppure e' un numero negativo o decimale" 
		exit 2;; 
*) 	if test $1 -eq 0 
	then 
		echo "ERRORE: $1 non valido in quanto è nullo e non strettamente positivo" 	
		exit 3
	else 
		echo "DEBUG-FCP.sh: $1 è un numero strettamente positivo, OK!" 
		W=$1
		shift  
	fi;; 
esac 
 
#controllo che il secondo parametro sia una stringa non contenente alcuno /, infatti trattandosi del carattere usato come separatore nei percorsi file, potrebbe dare problemi nella fase di pattern matching con i file validi

case $1 in 
*/*) 	echo "ERRORE: $1 contiene un carattere /, non valido come nome di un estensione" 
	exit 4;; 
*) 	
	S=$1 	#salviamo il valore della stringa dentro a S, così da poter effettuare uno shift e avere $* che contennga solo le Q gerarchie 
	shift;; 
esac 

#controllo che le Q gerarchie siano passate come nomi assoluti di directory traversabili 

for G 
do
	case $G in 
	/*) 	if test ! -d $G -o ! -x $G 
		then 
			echo "ERRORE: $G non è una directory traversabile" 
			exit 5
		fi;; 
	*) 	
		echo "ERRORE: $G non è un nome assoluto"
		exit 6;; 
	esac 
done 

#arrivati a questo punto sappiamo di avere almeno 4 parametri di tipo corretto, andiamo dunque ad aggiorare la PATH e ad esportarla per compatibilità 

PATH=`pwd`:$PATH 
export PATH 


#creiamo un file temporaneo per andare a salvare globalmente i nomi di tutte le directory valide, lo creiamo attraverso la ridirezione a vuoto 

> /tmp/nomiAssoluti 


#effettuiamo le Q invocazioni al file ricorsivo FCR.sh, una per ogni gerarchia passata per parametro, andando a passare anche il file temporaneo creato, anche se non strettamente necessario, in quanto il file non contiene il valore $$ valido solo all'interno del processo corrente -> lo facciamo per continuità con gli altri es 

for G 
do 
	FCR.sh $G $S /tmp/nomiAssoluti 
done 


#a questo punto dovremmo avere un file nomiAssoluti che contiene in forma assoluta tutti i nomi delle directory valide trovate globalmente a questo dunque come da consegna andiamo a riportare il numero totale di queste directory 

TOT=`wc -l < /tmp/nomiAssoluti` 	#salviamo il numero di directory valide dentro ad una variabile, per poterla poi confrontare con W successivamente 	 

echo 	#stampa una riga vuota per leggibilità 
echo "numero totale di directory valide trovate: $TOT" 
echo 

if test $TOT -ge $W 	#se il numero di directory trovate risulta maggiore del parametro W 
then
	MAX_ATTEMPTS=5
	i=0 
	
	while test $i -lt $MAX_ATTEMPTS 
	do 
		echo -n "ciao `whoami`, fornisci per favore un valore intero compreso tra 1 e $W:  " 
		read X 
		
		case $X in 
		*[!0-9]*) 	echo "ERRORE: valore passato non numerico, oppure negativo o decimale, riprovare!"
				;; 
		*) 	if test $X -ge 1 -a $X -le $W 
			then 
				break
			else
				echo "ERRORE: valore passato fuori dal range (1 - $W), riprovare!"
			fi;; 
		esac 
		
		i=`expr $i + 1`	#incrementiamo il counter di tentativi 
		echo "numero di tentativi rimanenti: `expr $MAX_ATTEMPTS - $i`"
		echo  
	done  

	if test $i -ne $MAX_ATTEMPTS 
	then 
		echo -n "la directory relativa al valore $X e': "
		head -$X < /tmp/nomiAssoluti | tail -1  
	else 
		echo "hai terminato il numero di tentativi! :("
		rm /tmp/nomiAssoluti 	#ricorda che se si esce prima, andrà anche elimiato il file temporaneo 
		exit 7
	fi
 
else 
	echo "numero totale di directory valide insufficiente, trovate $TOT, necessarie almeno $W" 
fi 

rm /tmp/nomiAssoluti 	#eliminiamo il file temporaneo 

		

