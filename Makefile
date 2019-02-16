CC=gcc
VALG = valgrind
LIBSOCKET=-lnsl
CCFLAGS=-Wall -g -O2
SRV=server
CLT=client

PORT_SERVER=5002
USERS_DATA_FILE="users_data_file"

IP_SERVER=127.0.0.1
#IP_SERVER=10.16.4.70

all: $(SRV) $(CLT)

$(SRV):$(SRV).c
	$(CC) -o $(SRV) $(LIBSOCKET) $(SRV).c

$(CLT):	$(CLT).c
	$(CC) -o $(CLT) $(LIBSOCKET) $(CLT).c

run_server: $(SRV)
	./$(SRV) $(PORT_SERVER) $(USERS_DATA_FILE)

valgrind_server: $(SRV)
	$(VALG) ./$(SRV) $(PORT_SERVER) $(USERS_DATA_FILE)

run_client: $(CLT)
	./$(CLT) $(IP_SERVER) $(PORT_SERVER)

valgrind_client: $(CLT)
	$(VALG) ./$(CLT) $(IP_SERVER) $(PORT_SERVER)

clean:
	rm -f *.o *~
	rm -f $(SRV) $(CLT)
