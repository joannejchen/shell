shell: main.o commands.o
	gcc -o shell main.o commands.o

commands.o: commands.c
	gcc -Wall -c commands.c

main.o: main.c
	gcc -Wall -c main.c

clean:
	rm -rf *.o shell
	echo all clean
