#################################################
#
# Makefile FINALE progetto lcs09
#
#################################################

# ***** DA COMPLETARE ******  con i file da consegnare per il primo frammento
FILE_DA_CONSEGNARE=./ptree.c ./intervals.c ./ilist.c ./intervals.h ./ilist.h ./ptree.h


# Compiler flags
CFLAGS = -Wall -pedantic -g 


# Compilatore
CC = gcc


# Librerie
LIBDIR = ../lib
LIBS = -L $(LIBDIR)

# Nome libreria
LIBNAME = libztl.a

# Lista degli object files (** DA COMPLETARE ***)
OBJS = intervals.o ptree.o ilist.o test-libztl.o permserver.o

# nome eseguibile
exe = prova

# phony targets
.PHONY: clean testiniziale lib test1 test2 docu consegna 

# creazione libreria
lib:  $(OBJS)
	-rm  -f $(LIBDIR)/$(LIBNAME)
	ar -r $(LIBNAME) $(OBJS)
	cp $(LIBNAME) $(LIBDIR)

# eseguibile di test iniziale
$(exe): $(OBJS)
	$(CC) -o $@ $^ 

# dipendenze oggetto main di test
test-libztl.o: test-libztl.c intervals.h ptree.h ilist.h
	$(CC) $(CFLAGS) -c $<

# make rule per gli altri .o (***DA COMPLETARE***)

ptree.o: ptree.c ptree.h intervals.h ilist.h

ilist.o: ilist.c ilist.h intervals.h

intervals.o: intervals.c intervals.h

permserver.o: ptree.h




########### NON MODIFICARE DA QUA IN POI ################
# genera la documentazione con doxygen
docu: ../Doc/Doxyfile
	make -C ../Doc

#ripulisce
clean:
	-rm -f *.o *.~

# test con main iniziale
testiniziale: 
	make clean
	make $(exe)
	echo MALLOC_TRACE e\' $(MALLOC_TRACE)
	@echo MALLOC_TRACE deve essere settata a \"./.mtrace\"
	-rm -f ./.mtrace
	./$(exe)
	mtrace ./$(exe) ./.mtrace
	@echo -e "\a\n\t\t *** Test iniziale superato! ***\n"

# test unitario intervalli
test1:
	make clean
	make lib
	make -C ../Test/Test1 clean
	make -C ../Test/Test1 test
	@echo -e "\a\n\t\t *** Test unitario (1) intervalli superato! ***\n"

# test unitario alberi
test2:
	make clean
	make lib
	make -C ../Test/Test2 clean
	make -C ../Test/Test2 test
	@echo -e "\a\n\t\t *** Test unitario (2) ptree superato! ***\n"

# test script di mail
.PHONY: testmail consegna2
testmail:
	rm -fr multe/
	mkdir multe
	cp MAILDATA/anagrafe.txt anagrafe.txt
	cp MAILDATA/multe.log multe.log
	chmod 644 anagrafe.txt multe.log
	./testmailscript
	@echo -e "\a\n\t\t **** Test mailscript superato! ****\n"

# test progetto complessivo #########################################
.PHONY: testf1 testf2 testf3 testfinale consegna3

# test parsing
testf1: ztl permserver
	-killall -w permserver
	-killall -w ztl
	-rm -fr tmp/
	-mkdir tmp
	cp FINALDATA/perm.dat .
	cp FINALDATA/perm1.dat .
	chmod 644 ./perm.dat perm1.dat
	./testparse 
	@echo -e "\a\n\t\t *** Test finale 1 superato! ***\n"

# test funzionale 
testf2:  ztl permserver
	-killall -w permserver
	-killall -w ztl
	-rm -fr tmp/
	-mkdir tmp
	cp FINALDATA/multe?.sort.check .
	chmod 644 multe?.sort.check
	cp FINALDATA/perm1.dat .
	chmod 644 perm1.dat
	./testfunz 
	@echo -e "\a\n\t\t *** Test finale 2 superato! ***\n"

# test parallelo
testf3: ztl permserver
	-killall -w permserver
	-killall -w ztl
	-rm -fr tmp/
	-mkdir tmp
	cp FINALDATA/multe?.sort.check .
	chmod 644 multe?.sort.check
	cp FINALDATA/perm?.dat .
	chmod 644 perm?.dat
	./testsig
	@echo -e "\a\n\t\t *** Test finale 3 superato! ***\n"

testfinale: ztl permserver mailscript
	make clean	
	make lib
	make testiniziale
	make test1
	make test2
	make testmail
	make testf1
	make testf2
	make testf3
	@echo -e "\a\n\t\t *** Test finale superato! ***\n"

# target di consegna primo frammento
SUBJECT1="lcs09: consegna primo frammento"
consegna: 
	make testiniziale
	make test1
	make test2
	./gruppo-check.pl < gruppo.txt
	tar -cvf $(USER)-f1.tar ./gruppo.txt ./Makefile $(FILE_DA_CONSEGNARE)
	mpack -s $(SUBJECT1) ./$(USER)-f1.tar  susanna@di.unipi.it
	@echo -e "\a\n\t\t *** Frammento 1 consegnato  ***\n"

# target di consegna secondo frammento
SUBJECT2="lcs09: consegna secondo frammento"
FILE_DA_CONSEGNARE2=mailscript
consegna2: 
	make testmail
	./gruppo-check.pl < gruppo.txt
	tar -cvf $(USER)-f2.tar ./gruppo.txt $(FILE_DA_CONSEGNARE2)
	mpack -s $(SUBJECT2) ./$(USER)-f2.tar  susanna@di.unipi.it
	@echo -e "\a\n\t\t *** Frammento 2 consegnato  ***\n"

# target di consegna terzo frammento
SUBJECT3="lcs09: consegna progetto finale"
FILE_DA_CONSEGNARE3=$(FILE_DA_CONSEGNARE) $(FILE_DA_CONSEGNARE2) permserver.c ztlserv.c lcscom.c Makefile
consegna3: 
	make testfinale
	./gruppo-check.pl < gruppo.txt
	tar -cvf $(USER)-f3.tar ./gruppo.txt $(FILE_DA_CONSEGNARE3)
	mpack -s $(SUBJECT3) ./$(USER)-f3.tar  susanna@di.unipi.it
	@echo -e "\a\n\t\t *** Progetto finale consegnato  ***\n"

