all: shell

shell: shell.o
	g++ -o shell shell.o

shell.o: shell.cpp
	g++ -c shell.cpp

clean:
	rm -f shell shell.o
