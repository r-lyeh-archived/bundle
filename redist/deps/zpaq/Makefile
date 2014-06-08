zpaq: zpaq.cpp libzpaq.cpp libzpaq.h divsufsort.c divsufsort.h
	gcc -O3 -c -DNDEBUG divsufsort.c
	g++ -O3 -Dunix zpaq.cpp libzpaq.cpp divsufsort.o -pthread -o zpaq
	rm divsufsort.o
