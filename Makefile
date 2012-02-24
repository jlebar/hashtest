all: compile test

compile: cityhash smhasher spookyhash testatoms link

cityhash:
	make -C cityhash all

smhasher:
	cmake smhasher
	make -C smhasher

spookyhash:
	make -C spookyhash

testatoms:
	g++ -O3 -Wall -Werror -c -Icityhash/src -Ismhasher -Ispookyhash testatoms.cpp

link: cityhash smhasher spookyhash testatoms
	g++ -O3 -Wall -Werror -o testatoms testatoms.o spookyhash/spooky.o cityhash/src/city.o smhasher/libSmhasherSupport.a

.PHONY: all compile test cityhash smhasher spookyhash testatoms

test: compile
	./testatoms atoms
