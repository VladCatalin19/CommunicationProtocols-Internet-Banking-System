#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "errors.h"
#include "helper.h"

// Numărul maxim de clienți cu care poate opera server-ul
#define MAX_CLIENTS 100
// Dimensiune maximă buffer
#define BUFLEN 256
// Dimensiunea maximă a unui nume sau prenume
#define MAX_NAME_LEN 12
// Dimensiunea maximă a unei parole secrete
#define MAX_PASS_LEN 8

// Coduri pentru mesajele ce pot fi returnate de server prin protocolul UDP
#define UDP_PAROLA_SECRETA 1
#define UDP_CARD_DEBLOCAT 2

// Coduri pentru mesajele ce pot fi returnate de server prin protocolul TCP
#define TCP_WELCOME 1
#define TCP_LOGOUT 2
#define TCP_LISTSOLD 3
#define TCP_TRANS_CONF 4
#define TCP_TRANS_SUCC 5

// Structura client care conține date referitoare la un client
typedef struct {
    // Datele încărcate din fișier
    char nume[MAX_NAME_LEN];
    char prenume[MAX_NAME_LEN];
    int numar_card;
    int pin;
    char parola_secreta[MAX_PASS_LEN];
    double sold;

    // Boolean care reține dacă clientul este logat
    char logat;
    // Numărul de introduceri greșite ale PIN-ului
    char nr_esuari_logari;
    // Boolean care reține dacă cardul este blocat
    char card_blocat;
} CLIENT;

/*
 *      Citește datele din fișierul de intrare și le reține într-un vector
 *      alocat dinamic.
 *
 * Paramtri:
 *      clients = vectorul de clienți alocat dinamic returnat ca parametru
 *      clients_no = numărul de clienți din vectorul de clienți
 *      file_name = string cu numele fișierului de intrare
 *
 * Return:
 *      -1 dacă s-a întâmpinat eroare la citire sau alocare, 0 altfel
 */
int read_user_data(CLIENT **clients, int *clients_no, char* file_name) {

    FILE *users_data;

    users_data = fopen(file_name, "rt");
    if (users_data == NULL) {
        return -1;
    }

    fscanf(users_data, "%d\n", clients_no);
    *clients = (CLIENT *) malloc(*clients_no * sizeof(CLIENT));
    if (*clients == NULL) {
        fclose(users_data);
        return -1;
    }

    for (int i = 0; i < *clients_no; ++i) {
        memset(*clients + i, 0, sizeof(CLIENT));
        fscanf(users_data, "%s %s %d %d %s %lf\n",
            (*clients)[i].nume,
            (*clients)[i].prenume,
            &(*clients)[i].numar_card,
            &(*clients)[i].pin,
            (*clients)[i].parola_secreta,
            &(*clients)[i].sold);
    }

    fclose(users_data);
    return 0;
}

/*
 *      Afișează în terminal datele unui client.
 */
void print_client(CLIENT client) {
    printf("Nume:        %s\n", client.nume);
    printf("Prenume:     %s\n", client.prenume);
    printf("Numar card:  %d\n", client.numar_card);
    printf("PIN card:    %d\n", client.pin);
    printf("Parola:      %s\n", client.parola_secreta);
    printf("Sold:        %lf\n", client.sold);
    printf("Logat:       %c\n", (client.logat == 1) ? 'y' : 'n');
    printf("Nr esuari:   %d\n", client.nr_esuari_logari);
    printf("Card blocat: %c\n", (client.card_blocat == 1) ? 'y' : 'n');
}

/*
 *      Afișează în terminal datele tuturor clienților.
 */
void print_all_clients(CLIENT *clients, int clients_no) {
    for (int i = 0; i < clients_no; ++i) {
        print_client(clients[i]);
        printf("\n");
    }
}

/*
 *      Dezalocă memoria folosită în memoriarea datelor clienților.
 */
void free_user_data(CLIENT **clients, int *clients_no) {
    free(*clients);
    *clients = NULL;
    *clients_no = 0;
}

/*
 *      Caută dacă există client cu numărul de card respectiv.
 *
 * Parametri:
 *      nr_card = numărul de card căutat
 *      clients = vectorul de clienți în care se caută
 *      clients_no = numărul de clienți din vector
 *
 * Return:
 *      Un pointer către clientul respectiv, NULL dacă nu există client cu
 *      numărul de card respectiv
 */
CLIENT* find_with_card(int nr_card, CLIENT *clients, int clients_no) {
    for (int i = 0; i < clients_no; ++i) {
        if (clients[i].numar_card == nr_card) {
            return clients + i;
        }
    }

    return NULL;
}

/*
 *      Creează un string alocat dinamic cu mesajul ce va fi trimis prin UDP.
 *
 * Parametri:
 *      str_code = codul mesajului
 *
 * Return:
 *      Un string alocat dinamic cu mesajul generat
 */
char* create_udp_str(int str_code) {
    char head[BUFLEN] = "UNLOCK";
    char tail[BUFLEN];
    memset(tail, 0, BUFLEN);

    switch(str_code) {

        case UDP_PAROLA_SECRETA:
            strcpy(tail, "Trimite parola secreta");
            break;

        case UDP_CARD_DEBLOCAT:
            strcpy(tail, "Card deblocat");
            break;
    }

    char final_str[BUFLEN];
    sprintf(final_str, "%s> %s", head, tail);
    return strdup(final_str);
}

/*
 *      Creează un string alocat dinamic cu mesajul ce va fi trimis prin TCP.
 *
 * Parametri:
 *      str_code = codul mesajului
 *      c = structura client de unde se vor luat datele necesare
 *      suma = suma ce va fi transferată în cazul operației „transfer”
 *
 * Return:
 *      Un string alocat dinamic cu mesajul generat
 */
char* create_tcp_str(int str_code, CLIENT *c, double suma) {
    char head[BUFLEN] = "IBANK";
    char tail[BUFLEN];
    memset(tail, 0, BUFLEN);

    switch(str_code) {

        case TCP_WELCOME:
            sprintf(tail, "Welcome %s %s", c->nume, c->prenume);
            break;

        case TCP_LOGOUT:
            strcpy(tail, "Clientul a fost deconectat");
            break;

        case TCP_LISTSOLD:
            sprintf(tail, "%.2lf", c->sold);
            break;

        case TCP_TRANS_CONF:
            sprintf(tail, "Tranfer %.2lf catre %s %s? [y/n]", suma, c->nume,
                    c->prenume);
            break;

        case TCP_TRANS_SUCC:
            strcpy(tail, "Transfer realizat cu succes");
            break;

    }

    char final_str[BUFLEN];
    sprintf(final_str, "%s> %s", head, tail);
    return strdup(final_str);
}

/*
 *      Preia comanda primită de la un client prin protocolul UDP, verifică
 *      dacă comanda este validă și creează un string cu răspunsul la comanda
 *      respectivă.
 *
 * Parametri:
 *      command = string cu comanda primită
 *      clients = vectorul cu datele clienților
 *      clients_no = numărul de clienți din vector
 *
 * Return:
 *      Un string alocat dinamic cu mesajul ce va fi trimis clientului
 */
char* parse_from_client_udp(char *command, CLIENT *clients, int clients_no) {
    char original[BUFLEN];      // Copie a comenzii primite
    memset(original, 0, BUFLEN);
    memcpy(original, command, strlen(command));

    command = strtok(command, " ");


    if (strcmp(command, "unlock") == 0) {
        int nr_card;
        // Se garantează de la client că după „unlock” există un număr natural
        decode_nr_card(original, &nr_card);

        CLIENT *c = find_with_card(nr_card, clients, clients_no);

        if (c == NULL) {
            return get_error(ERR_NR_CARD_INEXISTENT, IBANK);
        }

        if (c->card_blocat == 0) {
            return get_error(ERR_OPERATIE_ESUATA, UNLOCK);
        }

        return create_udp_str(UDP_PAROLA_SECRETA);
    }


    // Comandă prin care se recunoaște ca se trimite parola secretă pentru
    // deblocarea cardului
    // Clientul trimite ”unlock_with_pass nr_card parola_secreta”
    if (strcmp(command, "unlock_with_pass") == 0) {
        int nr_card;
        char *parola;
        // Se garantează de la client că după „unlock_with_pass” există un număr
        // natural și un string
        decode_unlock_pass(original, &nr_card, &parola);

        // Se garantează de la client ca numărul de card există
        CLIENT *c = find_with_card(nr_card, clients, clients_no);

        if (strcmp(c->parola_secreta, parola) == 0) {
            c->card_blocat = 0;
            free(parola);
            return create_udp_str(UDP_CARD_DEBLOCAT);
        } else {
            free(parola);
            return get_error(ERR_DEBLOCARE_ESUATA, UNLOCK);
        }
    }

    return NULL;
}

/*
 *      Preia comanda primită de la un client prin protocolul TCP, verifică
 *      dacă comanda este validă și creează un string cu răspunsul la comanda
 *      respectivă.
 *
 * Parametri:
 *      command = string cu comanda primită
 *      clients = vectorul cu datele clienților
 *      clients_no = numărul de clienți din vector
 *
 * Return:
 *      Un string alocat dinamic cu mesajul ce va fi trimis clientului
 */
char* parse_from_client_tcp(char *command, CLIENT *clients, int clients_no) {
    char original[BUFLEN];
    memset(original, 0, BUFLEN);
    memcpy(original, command, strlen(command));

    command = strtok(command, " ");


    if (strcmp(command, "login") == 0) {
        int nr_card, pin;
        // Se garantează de la client ca după „login” există două numere
        // naturale
        decode_login(original, &nr_card, &pin);

        CLIENT *c = find_with_card(nr_card, clients, clients_no);
        if (c == NULL) {
            return get_error(ERR_NR_CARD_INEXISTENT, IBANK);
        }

        if (c->logat == 1) {
            return get_error(ERR_SESINE_DEJA_DESCHISA, IBANK);
        }

        if (c->pin != pin) {
            if (++(c->nr_esuari_logari) >= 3) {
                c->card_blocat = 1;
            } else {
                return get_error(ERR_PIN_GRESIT, IBANK);
            }
        }

        if (c->card_blocat) {
            return get_error(ERR_CARD_BLOCAT, IBANK);
        }

        c->logat = 1;
        return create_tcp_str(TCP_WELCOME, c, 0);
    } 


    if (strcmp(command, "logout") == 0) {
        int nr_card;
        // Se garantează de la clienți că după „logout” există un număr natural
        decode_nr_card(original, &nr_card);

        // Se garantează de la client ca numărul de card există
        CLIENT *c = find_with_card(nr_card, clients, clients_no);
        c->logat = 0;
        return create_tcp_str(TCP_LOGOUT, NULL, 0);
    }


    if (strcmp(command, "listsold") == 0) {
        int nr_card;
        // Se garantează de la clienți că după „logout” există un număr natural
        decode_nr_card(original, &nr_card);

        // Se garantează de la client ca numărul de card există
        CLIENT *c = find_with_card(nr_card, clients, clients_no);
        return create_tcp_str(TCP_LISTSOLD, c, 0);
    }


    if (strcmp(command, "transfer") == 0) {
        int nr_card_dest;
        double suma;
        int nr_card_sursa;
        // Se garantează de la client că după „transfer” există un număr
        // natural, un număr real pozitiv și un număr natural
        decode_transfer_server(original, &nr_card_dest, &suma, &nr_card_sursa);

        CLIENT *c_dest = find_with_card(nr_card_dest, clients, clients_no);
        if (c_dest == NULL) {
            return get_error(ERR_NR_CARD_INEXISTENT, IBANK);
        }

        // Se garantează de la client că numărul sau de card există
        CLIENT *c_sursa = find_with_card(nr_card_sursa, clients, clients_no);

        if (c_sursa->sold < suma) {
            return get_error(ERR_FONDURI_INSUFICIENTE, IBANK);
        }

        return create_tcp_str(TCP_TRANS_CONF, c_dest, suma);
    }


    // Comandă specială prin care se trimite confirmarea trasnferului
    // Clientul trimite aceiați parametrii ca cei de la comanda „transfer”
    if (strcmp(command, "y") == 0) {
        int nr_card_dest;
        double suma;
        int nr_card_sursa;
        // Se garantează de la client că după „transfer” există un număr
        // natural, un număr real pozitiv și un număr natural
        decode_transfer_server(original, &nr_card_dest, &suma, &nr_card_sursa);

        // Se garantează de la client că există clienții expeditor și receptor
        CLIENT *c_dest = find_with_card(nr_card_dest, clients, clients_no);
        CLIENT *c_sursa = find_with_card(nr_card_sursa, clients, clients_no);

        c_sursa->sold -= suma;
        c_dest->sold += suma;

        return create_tcp_str(TCP_TRANS_SUCC, NULL, 0);
    }

    return NULL;
}

int main(int argc, char **argv) {
    if (argc != 3) {
        exit_with_error("Utilizare : ./server port_server users_data_file!",
                -1, -1);
    }

    int port = atoi(argv[1]);
    if (port == 0) {
        exit_with_error("Port invalid!", -1, -1);
    }

    char *users_data_file = argv[2];

//---------------------------------------------------------------------------\\
//                          Creare structuri adrese                          \\
//---------------------------------------------------------------------------\\

    struct sockaddr_in server_sockaddr;
    struct sockaddr_in client_sockaddr;

    server_sockaddr.sin_family = AF_INET;
    server_sockaddr.sin_port = htons(port);
    server_sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);

    client_sockaddr.sin_family = AF_INET;
    client_sockaddr.sin_port = 0;
    client_sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);


//---------------------------------------------------------------------------\\
//                           Deschidere socket UDP                           \\
//---------------------------------------------------------------------------\\

    int udp_socket;

    udp_socket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (udp_socket < 0) {
        exit_with_error("Deschiderea socketului UDP a esuat!", -1, -1);
    }

    if (bind(udp_socket, (struct sockaddr *) &server_sockaddr,
            sizeof(struct sockaddr)) < 0) {

        exit_with_error("Legarea la socketul UDP a esuat!", udp_socket, -1);
    }

//---------------------------------------------------------------------------\\
//                           Deschidere socket TCP                           \\
//---------------------------------------------------------------------------\\

    int tcp_socket;

    tcp_socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (tcp_socket < 0) {
        exit_with_error("Deschiderea socketului TCP a esuat!", udp_socket, -1);
    }

    if (bind(tcp_socket, (struct sockaddr *) &server_sockaddr,
            sizeof(struct sockaddr)) < 0) {

        exit_with_error("Legarea la socketul TCP a esuat!", udp_socket, tcp_socket);
    }

    listen(tcp_socket, MAX_CLIENTS);

//---------------------------------------------------------------------------\\
//                    Setare variabile pentru multiplexare                   \\
//---------------------------------------------------------------------------\\

    fd_set read_fds;    // multimea de citire folosita de select()
    fd_set tmp_fds;     // multime folosita temporar
    int fdmax;          // valoare maxima file descriptor din multimea read_fds

    FD_ZERO(&read_fds);
    FD_ZERO(&tmp_fds);
    FD_SET(0, &read_fds);
    FD_SET(udp_socket, &read_fds);
    FD_SET(tcp_socket, &read_fds);
    fdmax = tcp_socket;

//---------------------------------------------------------------------------\\
//                   Incarcare fisier cu datele clientilor                   \\
//---------------------------------------------------------------------------\\

    int clients_no;
    CLIENT *clients;
    if (read_user_data(&clients, &clients_no, users_data_file) < 0) {
        exit_with_error("Citirea din fisier a esuat!", udp_socket, tcp_socket);
    }

//---------------------------------------------------------------------------\\
//                       Primire comenzi de la clienti                       \\
//---------------------------------------------------------------------------\\
    
    char buf[BUFLEN];
    char server_running = 1;    // Boolean care reține dacă serverul este pornit

    while (server_running) {
        tmp_fds = read_fds;

        if (select(fdmax + 1, &tmp_fds, NULL, NULL, NULL) < 0) {
            free_user_data(&clients, &clients_no);
            exit_with_error("Selectarea scoketilor a esuat!", udp_socket,
                    tcp_socket);
        }

        for (int i = 0; i <= fdmax && server_running; ++i) {
            if (FD_ISSET(i, &tmp_fds)) {

                if (i == 0) {
                    // Mesaj primit de la stdin
                    memset(buf, 0, BUFLEN);
                    fgets(buf, BUFLEN - 1, stdin);

                    if (strcmp(buf, "quit\n") == 0) {
                        server_running = 0;
                    }
                } else if (i == udp_socket) {
                    // Mesaj primit pe socketul UDP
                    memset(buf, 0, BUFLEN);
                    int len = sizeof(client_sockaddr);

                    if (recvfrom(udp_socket, buf, BUFLEN, 0,
                            (struct sockaddr *) &client_sockaddr, &len) < 0) {

                        free_user_data(&clients, &clients_no);
                        exit_with_error("Receptionarea mesajului prin " 
                                "UDP a esuat!", udp_socket, tcp_socket);
                    }

                    printf("Am primit prin UDP de la %s, port %d, "
                            "mesajul: %s\n",
                            inet_ntoa(client_sockaddr.sin_addr),
                            ntohs(client_sockaddr.sin_port), buf);


                    // Se garantează de la client că comanda este validă
                    char *str = parse_from_client_udp(buf, clients, clients_no);

                    if (str != NULL && 
                        (sendto(udp_socket, str, strlen(str), 0,
                                (struct sockaddr *) &client_sockaddr,
                                sizeof(struct sockaddr)) < 0)) {

                        free_user_data(&clients, &clients_no);
                        exit_with_error("Trimiterea mesajului prin UDP "
                                "a esuat!", udp_socket, tcp_socket);
                    }

                    free(str);

                } else if (i == tcp_socket) {
                    // S-a conectat un client nou
                    int sockaddr_size = sizeof(client_sockaddr);
                    int new_socket = accept(tcp_socket, (struct sockaddr *)
                            &client_sockaddr, &sockaddr_size);
                    if (new_socket < 0) {
                        free_user_data(&clients, &clients_no);
                        exit_with_error("Adaugarea unui nou socket a esuat!",
                                udp_socket, tcp_socket);
                    }

                    // Adaugam noul client in lista
                    FD_SET(new_socket, &read_fds);
                    if (new_socket > fdmax) {
                        fdmax = new_socket;
                    }

                    printf("Conexiune noua de la %s, port %d, socket %d\n",
                            inet_ntoa(client_sockaddr.sin_addr),
                            ntohs(client_sockaddr.sin_port), new_socket);

                } else {
                    // Am primit comenzi pe socketul TCP
                    memset(buf, 0, BUFLEN);

                    int bytes_recv = recv(i, buf, BUFLEN, 0);
                    if (bytes_recv <= 0) {
                        if (bytes_recv == 0) {
                            // Clientul s-a deconectat
                            printf("Conexiunea pe socketul %d s-a terminat.\n",
                                    i);
                        } else {
                            // Eroare 
                            free_user_data(&clients, &clients_no);
                            memset(buf, 0, BUFLEN);
                            sprintf(buf, "Receptioarea mesajului pe socketul"
                                " %d a esuat!", i);
                            exit_with_error(buf, udp_socket, tcp_socket);
                        }

                        close(i);
                        FD_CLR(i, &read_fds);
                    } else {
                        // Am primit mesaj de la client
                        printf ("Am primit de la clientul de pe socketul %d,"
                                " mesajul: %s\n", i, buf);

                        if (strcmp(buf, "quit") == 0) {
                            close(i);
                            FD_CLR(i, &read_fds);
                        } else {
                            // Se garantează de la client că comanda este validă
                            char *str = parse_from_client_tcp(buf, clients,
                                    clients_no);

                            if (str != NULL && 
                                    (send(i, str, strlen(str), 0) < 0)) {

                                free_user_data(&clients, &clients_no);
                                memset(buf, 0, BUFLEN);
                                sprintf(buf, "Receptionarea mesajului de pe "
                                        "socketul %d a esuat!", i);
                                exit_with_error(buf, udp_socket, tcp_socket);
                            }

                            free(str);
                        }
                    }
                }
            }
        }
    }

//---------------------------------------------------------------------------\\
//                               Oprire server                               \\
//---------------------------------------------------------------------------\\

    close(udp_socket);
    close(tcp_socket);
    free_user_data(&clients, &clients_no);

    return 0;
}
