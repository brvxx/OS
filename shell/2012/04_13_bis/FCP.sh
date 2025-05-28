#/bin/sh 
#file comandi principale relativo alla prima prova in itinere del 13 aprile 2012 - versione che non usa il file temporaneo come counter per il lvl_max, infatti in questa soluzione andremo ad usare il valore di ritorno delle varie invocazioni di FCR.sh per salvare il livello massimo, che implicitamente rappresenta il numero totale di livelli della gerarchia 

#controllo stretto dei parametri con tatno di controllo del tipo 

case $# in 
1) 	echo "DEBUG-FCP.sh: numero di parametri passato corretto, OK!" 
	
	case $1 in 
	/*) 	if test ! -d $1 -o ! -x $1 
		then 
			echo "ERRORE: $1 non è una directory traversabile" 
			exit 1 
		fi;; 
	*) 
		echo "ERRORE: $1 non è un nome assoluto" 
		exit 2;; 
	esac;; 
*) 	
	echo "ERRORE: numero di parametri passati errato - Attesi 1 - Passati $#" 
	echo "Usage is: $0 dirass" 
	exit 3;; 
esac 

#aggiorniamo il valore della variabile PATH 

PATH=`pwd`:$PATH 
export PATH 

#fase A: invochiamo la prima volta il file comandi ricorsivo per andare a contare quanti livelli sono presenti nella gerarchia passata per parametro 

fase=A 
count=0 	#counter del livello corrente, inizializzato a 0, verrà incrementato ogni volta che si SCENDE di un livello nell'esplorazione 

FCR.sh $1 $count $fase 	#questa volta l'invocazione avviene con 3 parametri perchè ancora non abbiamo il valore passato dall'utente, nota che nel file FCR.sh
			#$4 sarà una stringa nulla senza dare nessun tipo di errore 	

lvl_tot=$? 	#il valore totale di livelli della gerarchia sarà proprio il valore di uscita del file comandi ricorsivo per come l'abbiamo costruito 

echo "numero di livelli della gerarchia $1: $lvl_tot" 
echo 

#fase B: chiediamo all'utente un valore pari compreso tra 1 e $lvl_tot, questo sarà il livello target, nel quale andremo a cercare, attraverso un ulteriore incvocazione del file comandi ricorsivo, le directory con il loro contenuto 

fase=B 

#chiediamo all'utente il livello che si voglia visualizzare 

echo -n "dammi il numero del livello che vuoi visualizzare, valore numerico PARI compreso tra 1 e $lvl_tot, estremi compresi: " > /dev/tty 
read lvl 


#controllo della validità del valore inserito dall'utente 

case $lvl in 
*[!0-9]*) 	echo "ERRORE: $lvl non è un valore numerico, oppure è negativo e duneque non appartiene al range [1 - $lvl_tot]" 
		exit 4;; 
*) 
	if test $lvl -ge 1 -a $lvl -le $lvl_tot		#se il valore è contenuto nel range 
	then 
		if test `expr $lvl % 2` -eq 0 		#se il valore è pari 
		then  
			echo "DEBUG-FCP.sh: valore inserito accettato!" 
			echo
		else 
			echo "ERRORE: $lvl dispari - Atteso un valore pari"
			exit 5 
		fi 
	else 
		echo "ERRORE: $lvl non è contenuto nel range di valori ammissibili [1 - $lvl_tot]" 
		exit 6 
	fi;; 
esac 

#invochiamo nuovamente il file comandi ricorsivo sta volta passando pure $lvl 

FCR.sh $1 $count $fase $lvl 

echo "DEBUG-FCP.sh: FINITO TUTTO!" 










