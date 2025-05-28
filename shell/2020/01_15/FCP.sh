#!/bin/sh 
#file comandi principale relativo alla parte di shell dell'esame totale del 15 gennaio 2020 

#controllo lasco del numero di parametri passati, W + 1 con W >= 2, dunque ci aspettiamo almeno 3 parametri

case $# in 
0|1|2) 	echo "ERRORE: numero di parametri passato errato - Attesi almeno 3 - Passati $#" 
	echo "Usage is: $0 numpos dirass1 dirass2 [dirass3] ... " 
	exit 1;; 
*) 	
	echo "DEBUG-FCP.sh: numero di parametri passato corretto, OK!";; 
esac 

#controllo che il primo parametro sia un numero intero strettamete postivo (controllo con case) 

case $1 in 
*[!0-9]*) 	echo "ERRORE: $1 non è un valore numerico, oppure è positivo o non intero" 
		exit 2;; 
*) 
	if test $1 -eq 0 
	then 
		echo "ERORRE: $1 non è strettamente positivo in quanto nullo" 
		exit 3
	fi;; 
esac 

#salviamo il valore del primo parametro dentro una variabile H, per poi effettuare uno shift e avere $* che contiene solo le gerarchie 

H=$1 
shift 

#effettuiamo il controllo sulle W gerachie, accertandoci che siano dei nomi assoluti di directory traversabili 

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
echo 	#stampa di una riga vuota per leggibilità 


#arrivati a questo punto siamo certi che il numero di parametri passato è corretto e pure il loro tipo

#aggiorniamo la variabile PATH, andando ad aggiungere la directory corrente tra i percorsi e la esportiamo per compatibilità 

PATH=`pwd`:$PATH 
export PATH 

#a questo punto andiamo a creare un file temporaneo all'interno del quale andremo a salvare il nome assoluto dei file trovati globalmente nell GERARCHIA CORRENTE, così da poter effettuare il conteggio e l'invocazione della parte in C. Nota che la creazione del file verrà effettuata direttamente dentro il for, così che ad ogni iterazione il file venga azzerato, in modo tale che possa essere riutilizzato dalla gerarchia successiva. 


#andiamo dunque a invocare il file comandi ricorsivo per ogni gerarchia passata come parametro 

for G  
do 
	> /tmp/conta$$

	echo "DEBUG-FCP.sh: stiamo per esplorare la gerarchia $G" 
	FCR.sh $G $H /tmp/conta$$ 

	echo 
	
	files=`wc -l < /tmp/conta$$` 	#salviamo dentro ad una variabile il numero trovati nella gerarchia corrente 

	echo "numero totale di file validi trovati nella gerarchia $G: $files" 

	if test `expr $files % 2` -eq 1 -o $files -eq 0
	then 
		echo "DEBUG-FCP.sh: non verrà invocata la parte in C poichè il numero di file trovati ($files) è dispari o nullo" 
	else 
		echo "DEBUG-FCP.sh: dal momento che il numero di file trovati ($files) è pari si procede con l'invocazione della parte C: fileC \`cat /tmp/conta\$\$\`"
		echo dove espanso il comando cat dà come risultato: `cat /tmp/conta$$` 
	fi 
	echo 

done 

rm /tmp/conta$$ 	#rimuoviamo il file temporaneo creato per il conteggio 

