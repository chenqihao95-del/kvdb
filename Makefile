all: server client

server: server.c shm.c sem.c
	gcc -o server server.c shm.c sem.c -Wall

client: client.c
	gcc -o client client.c -Wall

clean:
	rm -f server client kvdb.data
