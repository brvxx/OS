#!/bin/sh 
#file comandi principale relativo alla prima prova in itinere del 13 aprile 2012

#controllo stretto del numero dei parametri, in quanto abbiamo 1 solo parametro, gli mettiamo dentro anche il controllo sul tipo 

case $# in 
1) 	echo "DEBUG-FCP.sh: numero di parametri passato corretto, OK!" 

	case $1 in 
	/*) 	if test ! -d $1 -o ! -x $i 
		then 
			echo "ERRORE: $1 non è una directory traversabile" 
			exit 1
		fi;; 
	*) 
		echo "ERRORE: $1 non è un nome assoluto" 
		exit 2;; 
	esac;; 
*) 	
	echo "ERORRE: numero di parametri passato errato - Attesi 1 - Passati $#" 
	exit 3;; 
esac 
echo "DEBUG-FCP.sh: tipo del parametro passato corretto, OK!" 
echo 	#stampa di una linea vuota per leggibilità dell'output 

#aggiorniamo la variabile PATH aggiungendo la directory corrente e la esportiamo per compatibilità 

PATH=`pwd`:$PATH 
export PATH 

#invochiamo il file comandi ricorsivo per la fase A, dunque definiamo una variabile fase che useremo dentro al file ricorsivo per distinguere i due casi e invochiamo FCR.sh, nota che il file ricorsivo necessiterà pure di una variabile contatore (conta), che gli permetta di tenere traccia del livello corrente.

#nota che con livelli della gerarchia intendiamo i vari "piani" del grafo, per esempio la radice sarà il livello 1, tutte le sottodirectory directory di questa saranno considerate le directory del livello 2, le loro sottodirectory a loro volta quelle del livello 3 e via dicendo 

fase=A 
conta=0 

#inizializziamo poi un file che fungerà tipo da variabile globale dentro al processo ricorsivo che terrà traccia del massimo livello raggiunto, che rappresenta dunque il numero totale di livelli della gerarchia. Ovviamente all'inizio del processo questa verrà inizializzata a 0, attraverso il valore di conta 

echo $conta > /tmp/conta_livelli 	#non lo passeremo a FCR.sh in quanto non dipende da un valore $$ differente nei processi differenti 

FCR.sh $1 $conta $fase 


#terminata la fase A, il file conta_livelli adesso contiene il livello massimo (ossia il numero totale di livelli) della gerarchia, quindi lo andiamo a mettere dentro una variabile e lo stampiamo 

read lvl_tot < /tmp/conta_livelli 	#nota che potevamo usare analogamente `cat /tmp/conta_livelli`
echo "numero totale di livelli della gerarchia $1: $lvl_tot" 
echo 

rm /tmp/conta_livelli 	#andiamo a rimuovere il file temporaneo che non ci serve per la fase B 

#passiamo alla fase B, dove chiediamo all'utente un numero intero pari compreso tra 1 e lvl_tot, così che nella seconda invocazione del file ricorsivo andremmo a stampare tutte le informazioni (-l) relative ai file presenti nelle directory che si trovano al livello specificato dall'utente 

fase=B 

echo -n "inserire un valore numerico intero PARI compreso tra 1 e $lvl_tot: " > /dev/tty	#la ridirezione serve per assicurarci che si vada a dialogare con il terminale in caso di eventuali ridirezioni dello stdout 
read lvl 

#effettuiamo un controllo sul valore immesso da terminale

case $lvl in 
*[!0-9]*) 	echo "ERRORE: il valore immesso $lvl non è un valore numerico, oppure è negativo, dunque fuori dal range ammissibile" 
		exit 4;; 
*) 
	if test $lvl -ge 1 -a $lvl -le $lvl_tot		#valore passato dentro il range  
	then 	
		if test `expr $lvl % 2` -eq 0 
		then 
			echo "DEBUG-FCP.sh: valore inserito accettato!" 
			echo 
		else 
			echo "ERRORE: $lvl non è un numero pari" 
			exit 5 
		fi 
	else 
		echo "ERRORE: $lvl non rientra nel range di valori ammissibili [1 - $lvl_tot]"
		exit 6 
	fi;;
esac 


#arrivati a questo punto siamo sicuri che il valore immesso dall'utente è corretto e quindi procediamo con la fase B 

FCR.sh $1 $conta $fase $lvl 	#qui invochiamo passando anche il valore dato dall'utente 

