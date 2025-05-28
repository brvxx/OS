#!/bin/sh 
#file comandi ricrosivo (FCR.sh) relativo alla parte di shell dell'esame del 25 gennaio 2023 

#definiamo qualche variabile, seguendo le indicazioni del testo 

count1=0 	#conta i file valdi con estensione .E1 ($2 dentro a questo file) 
count2=0 	#conta i file valdi con estensione .E2 ($3 dentro a questo file) 

file1= 		#variabile lista che si salva il nome relativo semplice di tutti i file di estensione .E1 
file2= 		#variabile lista che si salva il nome relativo semplice di tutti i file di estensione .E2

cd $1 		#ci spostiamo nella directory giusta 

for F in * 	#for che itera sui vari file F (seguiamo indicazione data sul nome nel testo) della directory corente 
do 
	if test -f $F -a -r $F 
	then 
		case $F in 
		*.$2) 	#file leggibile con estensione .E1, aumentiamo dunque il valore di count1 e salviamo il nome del file dentro la lista file1 
			count1=$(expr $count1 + 1) 
			file1="$file1 $F";; 
		*) ;; 	#file leggibile ma con estensione diversa da .E1 
		esac 

		#dal momento che stiamo già dentro ad un if che ci verifica di stare su un file leggibile, andiamo ora ad aggiungere la condizione di scrivibilità 

		if test -w $F 
		then 
			case $F in 
			*.$3) 	#file leggibile e scrivibile con estensione .E2, aumnetiamo il valroe di conunt2 e salvbiamo il nome del file dentro la lista file2
				count2=$(expr $count2 + 1) 
				file2="$file2 $F";; 
			*) ;;	#file leggibile e scrivibile ma con estensione diversa da .E2 
			esac 
		fi 
	fi 
done 

#adesso se la directory per la directory corrente le due count sono diverse da 0, andiamo a stampare il suo percorso assoluto e facciamo (sebbene a noi concretamente non serva) l'invocazione al programma in C

if test $count1 -ne 0 -a $count2 -ne 0 
then 
	echo "TROVATA una directory che soddisfa le specifiche e il suo nome assoluto è $(pwd)"
	
	#invochiamo ora la parte in C, ovviamente commentandola perché se no lo script proverebbe comunque a eseguirla anche se non esiste alcuna parte in C. Dunque per ogni file appartenente a file2 dobbiamo invocare la parte in C con tutti i file di file1, quindi ci basterà fare un for che gira su file2 
	
	for f2 in $file2 
	do 
		:	#comando nullo, serve per far sì che il for non sia vuoto, infatti un for vuoto da errori  
		# fileC $file1 $f2 
	done 
fi 


#passiamo ora ad effettuare le vere chiamate ricorsive dentro la directory attuale 

for i in * 
do 
	if test -d $i -a -x $i 
	then 
		$0 $1/$i $2 $3 
	fi
done 

