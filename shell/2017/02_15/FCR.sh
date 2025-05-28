#!/bin/sh 
#file comandi ricorsivo relativo alla parte di shell dell'esame del 15 febbraio 2017

cd $1 	#ci spostiamo sulla gerarchia da esplorare 

#andiamo a cercare i file che rispettano la specifica e a salvarli dentro una variabile lista files, così che poi possiamo andarli a stampare e implicitamente possiamo usare quella variabile per controllare se la directory è valida o meno, infatti se non troviamo file validi, la variabile files sarà vuota e quindi la directory corrente non risulta valida 

files= 

for i in * 
do 
	if test -f $i -a -r $i 
	then 
		if test `wc -l < $i` -eq $2 
		then 
			if grep -v '[0-9]' $i > /dev/null 2>&1 	#se andiamo a trovare anche solo una linea che non contiene un carattere numerico allora si skippa 
			then 
				continue 
			else 
				files="$files $i" 
			fi 
		fi 
	fi 
done
 
#arrivati a questo punto la variabile files conterrà i nomi relativi dei file validi dentro la directory corrente, dunque se questa stringa (files) non è vuota possiamo andare a stampare in otuput la directory corrente insieme ai suoi files 

if test "$files"  	#necessario usare i doppi apici, affinchè venga interpetata come una sola stringa e non tante parole separate 
then 
	echo "TROVATA DIRECTORY: $1" 
	echo "FILES CONTENUTI che soddisfano la specifica: $files" 
	echo 
	
	#chiediamo all'utente se si vuole invocare la parte in C (roba relativa alla parte di esame che non vedremo) 
	echo -n "si vuolre procedere con l'invocazione della parte di C? " 
	read answer 
	
	case $answer in 
	si|SI|Si|yes|YES|Yes) 	echo "DEBUG-FCR.sh: a questo punto dovrebbe avvenire l'invocazione della parte in C con parametri \$files e \$2";; 
	*) 			echo "DEBUG-FCR.sh: non verrà invocata la parte in C";; 
	esac
	
	echo 
	echo 
fi 

#procediamo con la ricerca ricorsiva sulle sottodirectory della gerarchia 

for i in * 
do 
	if test -d $i -a -x $i 
	then 	
		$0 $1/$i $2 
	fi 
done 

