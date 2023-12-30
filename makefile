build: 
	gcc -pthread 7609.c -o semaphore
	
clean:
	rm -f semaphore