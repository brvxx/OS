#!/bin/sh 
#file comandi principale (FCP.sh) relativo alla prova di esame completa del 17 gennaio 2018 

#il programma accetta esattamente 4 parametri, rispettivamente, un nome assoluto di directory traversabile, due valori numeri interni STRETTAMENTE POSITIVI e un singolo carattere.

#effettuiamo dunque un controllo stretto sul numero di parametri passati 

case $# in 
4) 	echo "DEBUG-FCP.sh: numero di parametri passati corretto, OK!";; 
*) 	echo "ERRORE: numero di parametri passati errato - Aspettati 4, passati: $#" 
	echo "Usage is: $0 dirass num1 num2 car" 
	exit 1;; 
esac 

#effettuiamo dunque un controllo sui vari tipi di caratteri: 

#controllo che la directory sia traversabile e sia passata in forma assoluta 

case $1 in 
/*) 	
	if test ! -d $1 -o ! -x $1 
	then 
		echo "ERRORE: $1 non è una directory traversabile" 
		exit 2
	fi;; 
*) 	echo "ERRORE: $1 non è un nome assoluto" 
	exit 3;; 
esac 
echo "DEBUG-FCP.sh: primo parametro nome assoluto di directory traversabile, OK!"

#controllo che il secondo e il terzo parametro siano dei numeri interi strettamente positivi, per farlo principalmente esistono 2 metodi, un non molto intuitivo ma estremamente furbo, che sfrutta il comando expr, e invece un altro che è molto intuitivo ma un pochetto complicato e usa le RegEx. In questo caso verranno mostrati entrambi i casi, per il secondo parametro infatti useremo il metodo con l'expr e per il terzo quello con le RegEx, ma di default dentro uno stesso file comandi conviene essere coerenti ed usare un solo metodo. 

#controllo del secondo parametro via expr 

expr $2 + 0 > /dev/null 2>&1 	#andiamo ad addizionare 0 al secondo parametro, nota che ridirezioniamo tutto l'output perchè non ci interessa il risultato effettivo 

#quello che ci interessa infatti è il valore di ritorno del comando, infatti expr ritorna: 
#0) risultato diverso da 0 
#1) risultato uguale a 0 
#2) parametro invalido, con parametro invalido si intende che si tratti di una stringa che non corrisponda ad un qualche numero intero, nel nostro caso sicuramnete 0 è un parametro valido, quindi se ci accertiamo che il risultato sia diverso da 2 è come se implicitamente stessimo dicendo che il nostro parametro è un numero intero (eventualmente negativo) 
#3) errore generico

#a noi basta dunque andare a veriifare che il valore di ritorno sia 0, infatti tutti gli altri casi comporterebbero un parametro sbagliato, tipo se l'esito fosse 0, signfica che necessariamente il parametro che abbiamo passato era 0 (perché l'unico numero che sommato a 0 dia 0 è 0), ma 0 non è strettamente positivo quindi non ci interessa, se invece fosse 2 significa che il parametro era di tipo sbagliato (cosa che non volevamo) e sicuramente non vogliamo che ci sia un errore generico 

if test $? -eq 0	# $? contiene l'esito dell'ultimo comando eseguito, nota che se entriamo nell'if sicuramente abbiamo un valore numerico intero diverso da zero, ma possibilmente ancora negativo, cosa che non ci va bene, per questo ora possiamo controllare con un altro test che il valore non sia negativo   
then 
	if test $2 -le 0 	#nota che potevamo usare -lt, anche perché sicuramente non varrà 0 $2 per quanto detto sopra, ma prevenire è meglio che curare 
	then 
		echo "ERRORE: $2 è un valore negativo - Aspettato un valore strattamente positivo" 
		exit 4
	fi 
else 
	echo "ERRORE: $2 o è 0, oppure non rappresenta un valore numerico intero - Aspettato un valore strettamente positivo" 
	exit 5
fi 
echo "DEBUG-FCP.sh: secondo parametro valore numerico intero strettamente postivo, OK!" 

#controllo del terzo parametro attraverso una RegEx

case $3 in 
*[!0-9]*) 	echo "ERRORE: $3 non è un valore numerico strettamente positivo"
		exit 6;; 
*) 
	if test $3 -eq 0 
	then 
		echo "ERRORE: $3 è 0 - Aspettato un valore strettamente positivo"
		exit 7
	fi;; 
esac 
echo "DEBUG-FCP.sh: terzo parametro valore numerico intero strettamente postivo, OK!" 

#effettuiamo ora l'ultimo controllo sul tipo del quarto parametro che deve risultare un singolo carattere, usiamo dunque il metacarattere ? che fa pattern matching con singoli caratteri non nulli

case $4 in 
?) 	echo "DEBUG-FCP.sh: quarto parametro carattere singolo, OK!";; 
*) 	echo "ERRORE: $4 non è un singolo carattere - Aspettato un singolo carattere" 
	exit 8;; 
esac 
echo 

#a questo punto quello che rimane da fare è aggiornare la variabile PATH per permettere le chiamate ricorsive di FCR.sh a prescindere da dove ci si trovi nel file system

PATH=$(pwd):$PATH 
export PATH	#per compatibilità 

#chiamata al file ricorsivo, invocato passandogli tutti i parametri passati a FCP.sh 

echo "DEBUG-FCP.sh: inizio esplorazione ricorsiva della gerarchia $1"
echo 
FCR.sh $* 

echo 
echo "DEBUG-FCP.sh: FINITO TUTTO!" 


