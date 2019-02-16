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

// Dimensiune maximă nume fișier log
#define MAX_NAME_LEN 20
// Dimensiune maximă buffer
#define BUFLEN 256

char is_send_secret_str(char *str) {
    return (strcmp(str, "UNLOCK> Trimite parola secreta") == 0);
}

char is_unlock_card_str(char *str) {
    return (strncmp(str, "UNLOCK> Card deblocat",
            strlen("UNLOCK> Card deblocat")) == 0);
}

char is_welcome_str(char *str) {
    return (strncmp(str, "IBANK> Welcome", strlen("IBANK> Welcome")) == 0);
}

char is_err_str(char *str) {
    return (strncmp(str, "IBANK> -", strlen("IBANK> -")) == 0)
            || (strncmp(str, "UNLOCK> -", strlen("UNLOCK> -")) == 0);
}

/*
 *      Creează un string alocat dinamic care va conține una din comenzile
 *      „logout”, „listsold”, „unlock” și un număr de card.
 */
char* create_logout_listsold_unlock_str(char *str, int nr_card) {
    char buf[BUFLEN];
    memset(buf, 0, BUFLEN);
    sprintf(buf, "%s %d", str, nr_card);
    return strdup(buf);
}

/*
 *      Creează un string alocat dinamic care va conține comanda „transfer”
 *      numărul de card al destinației, suma care va fi transferată și
 *      numărul de card al expeditorului.
 */
char* create_transfer_str(char *str, int nr_card_dest, double suma,
        int nr_card_sursa) {

    char buf[BUFLEN];
    memset(buf, 0, BUFLEN);
    sprintf(buf, "%s %d %.2lf %d", str, nr_card_dest, suma, nr_card_sursa);
    return strdup(buf);
}

/*
 *      Preia comanda primită de la tastatură, verifică dacă comanda este
 *      validă pentru UNLOCK și creează un string cu mesajul care va fi trimis
 *      către server sau cu eroarea care va fi asișată în terminal.
 *
 * Parametri:
 *      command = string cu comanda primită
 *      nr_card = ultimul număr de card introdus de la tastară
 *      wait_secret_pas = boolean care indică dacă se așteaptă parola secretă
 *          după introducerea comenzii „unlock”
 *      ret_err = specifică dacă string-ul returnat de funcție este mesaj de
 *          eroare sau nu
 *
 * Return:
 *      Un string alocat dinamic cu rezultatul prelucrării comenzii
 */
char* parse_from_terminal_udp(char *command, int nr_card,
        char wait_secret_pass, char *ret_err) {

    char buf[BUFLEN];

    char original[BUFLEN];
    memset(original, 0, BUFLEN);
    strncpy(original, command, strlen(command) - 1);

    char *first_word = strtok(command, " ");
    if (first_word == NULL) {
        *ret_err = 1;
        return get_error(ERR_FUNCTIE, IBANK);
    }


    if (wait_secret_pass) {

        // Sterge caracterul de linie nouă de la finalul șirului
        first_word = strtok(first_word, "\n");

        memset(buf, 0, BUFLEN);
        sprintf(buf, "unlock_with_pass %d %s", nr_card, first_word);
        *ret_err = 0;
        return strdup(buf);
        
    }


    if (strcmp(first_word, "unlock\n") == 0) {

        memset(buf, 0, BUFLEN);
        sprintf(buf, "unlock %d", nr_card);
        *ret_err = 0;
        return strdup(buf);
    }

    *ret_err = 1;
    return NULL;
}

/*
 *      Preia comanda primită de la tastatură, verifică dacă comanda este
 *      validă pentru IBANK și creează un string cu mesajul care va fi trimis
 *      către server sau cu eroarea care va fi asișată în terminal.
 *
 * Parametri:
 *      command = string cu comanda primită
 *      is_logged = boolean care reține dacă clientul este logat
 *      client_running = boolean care reține dacă programul rulează
 *      nr_card = ultimul număr de card introdus de la tastară
 *      nr_card_auten = numărul de card al al clientului autentificat
 *      wait_transf_conf = boolean care indică dacă se așteaptă confirmarea
 *          efectuarii transferului bancar
 *      nr_card_dest = numărul de card al clientului destinatar al transferului
 *      suma = suma care va fi virată prin transfer bancar
 *      ret_err = specifică dacă string-ul returnat de funcție este mesaj de
 *          eroare sau nu
 *
 * Return:
 *      Un string alocat dinamic cu rezultatul prelucrării comenzii
 */
char* parse_from_terminal_tcp(char *command, char *is_logged, 
        char *client_running, int *nr_card, int *nr_card_auten,
        char *wait_transf_conf, int *nr_card_dest, double *suma,
        char *ret_err) {

    char original[BUFLEN];
    memset(original, 0, BUFLEN);
    strncpy(original, command, strlen(command) - 1);

    char *first_word = strtok(command, " ");
    if (first_word == NULL) {
        *ret_err = 1;
        return get_error(ERR_FUNCTIE, IBANK);
    }


    if (*wait_transf_conf && strcmp(first_word, "y\n") != 0) {
        *wait_transf_conf = 0;
        *ret_err = 1;
        return get_error(ERR_OPERATIE_ANULATA, IBANK);
    }


    if (strcmp(first_word, "login") == 0) {
        if (*is_logged) {
            *ret_err = 1;
            return get_error(ERR_SESINE_DEJA_DESCHISA, IBANK);
        }

        int pin;
        if (decode_login(original, nr_card, &pin) < 0) {
            *ret_err = 1;
            return get_error(ERR_FUNCTIE, IBANK);
        }

        *ret_err = 0;
        return strdup(original);
    }


    if (strcmp(first_word, "logout\n") == 0) {
        if (!(*is_logged)) {
            *ret_err = 1;
            return get_error(ERR_NEAUTENTIFICAT, IBANK);
        }

        *is_logged = 0;
        *ret_err = 0;
        return create_logout_listsold_unlock_str("logout", *nr_card_auten);
    }


    if (strcmp(first_word, "listsold\n") == 0) {
        if (!(*is_logged)) {
            *ret_err = 1;
            return get_error(ERR_NEAUTENTIFICAT, IBANK);
        }

        *ret_err = 0;
        return create_logout_listsold_unlock_str("listsold", *nr_card_auten);
    }


    if (strcmp(first_word, "transfer") == 0) {
        if (!(*is_logged)) {
            *ret_err = 1;
            return get_error(ERR_NEAUTENTIFICAT, IBANK);
        }

        if (decode_transfer(original, nr_card_dest, suma) < 0) {
            *ret_err = 1;
            return get_error(ERR_FUNCTIE, IBANK);
        }

        *wait_transf_conf = 1;
        *ret_err = 0;
        return create_transfer_str("transfer", *nr_card_dest, *suma,
                *nr_card_auten);
    }


    if (strcmp(first_word, "y\n") == 0 && *wait_transf_conf) {
        *wait_transf_conf = 0;

        *ret_err = 0;
        return create_transfer_str("y", *nr_card_dest, *suma,
                *nr_card_auten);
    }


    if (strcmp(first_word, "quit\n") == 0) {
        *client_running = 0;

        if (*is_logged) {
            *is_logged = 0;
            *ret_err = 0;
            return create_logout_listsold_unlock_str("logout", *nr_card_auten);
        }

        *ret_err = 0;
        return strdup("quit");
    }


    *ret_err = 1;
    return NULL;
}

/*
 *      Preia mesajul primit de la server prin protocolul TCP și modifică
 *      variabilele clientului după acesta.
 *
 * Parametri:
 *      msg_recv = string cu mesajul primit de la server
 *      is_logged = variabila care reține dacă clientul este logat
 *      nr_card_auten = variabila care reține numărul cardului clientului
 *          autentificat
 *      nr_card_curent = variabila care reține ultimul număr de card valid
 *          introdus de la tastatură
 *      wait_transf_conf = variabila care reține dacă se așteaptă confirmarea
 *          unui transfer bancar
 */
void parse_from_server_tcp(char *msg_recv, char *is_logged, int *nr_card_auten, 
        int *nr_card_curent, char* wait_transf_conf) {

    if (is_welcome_str(msg_recv)) {
        *is_logged = 1;
        *nr_card_auten = *nr_card_curent;
    }


    if (is_err_str(msg_recv)) {
        *wait_transf_conf = 0;
    }
}

/*
 *      Preia mesajul primit de la server prin protocolul UDP și modifică
 *      variabilele clientului după acesta.
 *
 * Parametri:
 *      msg_recv = string cu mesajul primit de la server
 *      wait_secret_pass = variabila care reține dacă se așteaptă parola
 *          secretă pentru deblocarea unui card
 */
void parse_from_server_udp(char *msg_recv, char *wait_secret_pass) {
    if (is_send_secret_str(msg_recv)) {
        *wait_secret_pass = 1;
    }


    if (is_unlock_card_str(msg_recv) || is_err_str(msg_recv)) {
        *wait_secret_pass = 0;
    }
}

/*
 *      Trimite un mesaj pe socket-ul TCP. Dacă utilizatorul este logat. iar
 *      acesta trimite „quit”, se vor trimite 2 mesaje, unul de tip „logout”
 *      și unul de tip „quit”.
 *
 * Parametri:
 *      tcp_socket = socket-ul tcp pe care se trimite mesajul
 *      udp_socket = socket-ul udp care se va închide în caz de eroare
 *      to_send = string-ul care conține mesajul
 *      input = comanda primită de la tastatură
 */
void send_tcp_msg(int tcp_socket, int udp_socket, char *to_send, char *input) {

    if (send(tcp_socket, to_send, strlen(to_send), 0) < 0) {
        exit_with_error("Trimiterea mesajului pe socket TCP a esuat!",
                udp_socket, tcp_socket);
    }

    if (strcmp(input, "quit\n") == 0 && strncmp(to_send, "logout", 6) == 0) {
        if (send(tcp_socket, "quit", strlen("quit"), 0) <0) {

            exit_with_error("Trimiterea mesajului pe socket TCP a esuat!",
                    udp_socket, tcp_socket);
        }
    }
}

/*
 *      Trimite un mesaj pe socket-ul UDP la adresa specificată din parametru.
 *
 ^ Parametri:
 *      tcp_socket = socket-ul tcp care se va închide în caz de eroare
 *      udp_socket = socket-ul udp pe care se trimite mesajul
 *      to_send = string-ul care conține mesajul
 *      erver_sockaddr = adresa server-ului
 */
void send_udp_msg(int tcp_socket, int udp_socket, char *to_send,
        struct sockaddr_in server_sockaddr) {

    if (sendto(udp_socket, to_send, strlen(to_send), 0,
            (struct sockaddr *)&server_sockaddr, sizeof(struct sockaddr)) < 0) {

        exit_with_error("Trimiterea mesajului pe socket TCP a esuat!",
                udp_socket, tcp_socket);
    }
}


int main(int argc, char **argv) {
    if (argc != 3) {
        exit_with_error("Utilizare : ./client port_server users_data_file!",
                -1, -1);
    }

    char *ip_server = argv[1];

    int port = atoi(argv[2]);
    if (port == 0) {
        exit_with_error("Port invalid!", -1, -1);
    }

//---------------------------------------------------------------------------\\
//                          Creare structură adresă                          \\
//---------------------------------------------------------------------------\\

    struct sockaddr_in server_sockaddr;

    server_sockaddr.sin_family = AF_INET;
    server_sockaddr.sin_port = htons(port);
    if (inet_aton(ip_server, &server_sockaddr.sin_addr) < 0) {
        exit_with_error("Adresa IP invalida!", -1, -1);
    }

//---------------------------------------------------------------------------\\
//                           Deschidere socket UDP                           \\
//---------------------------------------------------------------------------\\

    int udp_socket;

    udp_socket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (udp_socket < 0) {
        exit_with_error("Deschiderea socketului UDP a esuat!", -1, -1);
    }

//---------------------------------------------------------------------------\\
//                           Deschidere socket TCP                           \\
//---------------------------------------------------------------------------\\

    int tcp_socket;

    tcp_socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (tcp_socket < 0) {
        exit_with_error("Deschiderea socketului TCP a esuat!", udp_socket, -1);
    }

    if (connect(tcp_socket, (struct sockaddr*) &server_sockaddr,
            sizeof(server_sockaddr)) < 0) {

        exit_with_error("Conectarea la server a esuat!", udp_socket, -1);
    }

//---------------------------------------------------------------------------\\
//                             Creare fisier log                             \\
//---------------------------------------------------------------------------\\

    FILE *log_file;
    char log_file_name[MAX_NAME_LEN];

    memset(log_file_name, 0, MAX_NAME_LEN);
    sprintf(log_file_name, "client-%d", getpid());

    log_file = fopen(log_file_name, "wt");
    if (log_file == NULL) {
        exit_with_error("Crearea fisierului de log a esuat!", udp_socket,
                tcp_socket);
    }

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
//                       Trimitere comenzi catre server                      \\
//---------------------------------------------------------------------------\\

    char buf[BUFLEN];

    // Boolean care reține dacă programul clientului este pornit
    char client_running = 1;
    // Boolean care reține dacă clientul este autentificat
    char is_logged = 0; 
    // Boolean care reține dacă se așteaptă confirmarea pentru transferul bancar
    char wait_transf_conf = 0;
    // Boolean care reține dacă se așteaptă parola secretă pentru deblocarea
    // contului
    char wait_secret_pass = 0;
    // Numărul de card cu care este autentificat clientul
    int nr_card_auten = 0;
    // Ultimul număr de card introdus de la tastatură
    int nr_card_curent = 0;
    // Numărul de card care reprezintă destinatarul ultimului transferului bancar
    int nr_card_dest = 0;
    // Suma ce va trimisă prin trasfer bancar
    double suma = 0.0f;

    while (client_running) {
        tmp_fds = read_fds;

        if (select(fdmax + 1, &tmp_fds, NULL, NULL, NULL) < 0) {
            exit_with_error("Selectarea scoketilor a esuat!", udp_socket,
                    tcp_socket);
        }

        for (int i = 0; i <= fdmax && client_running; ++i) {
            if (FD_ISSET(i, &tmp_fds)) {

                if (i == 0) {
                    // Am primit mesaj de la stdin
                    memset(buf, 0, BUFLEN);

                    fgets(buf, BUFLEN - 1, stdin);
                    fprintf(log_file, "%s", buf);

                    // Boolean care reține dacă din procesarea mesajului care
                    // trebuie trimis prin TCP a rezultat o eroare
                    char ret_err_tcp = 0;
                    // String cu rezultatul procesării mesajului ce va fi trimis
                    // prin TCP
                    char *ret_str_tcp;

                    // Boolean care reține dacă din procesarea mesajului care
                    // trebuie trimis prin UDP a rezultat o eroare
                    char ret_err_udp = 0;
                    // String cu rezultatul procesării mesajului ce va fi trimis
                    // prin UDP
                    char *ret_str_udp;


                    if (wait_transf_conf) {
                        // Dacă se așteaptă confirmarea transferului, următorul
                        // mesaj va fi trimis sigur prin TCP și procesarea sa
                        // pentru UDP este inutilă
                        ret_str_tcp = parse_from_terminal_tcp(buf, &is_logged, 
                            &client_running, &nr_card_curent, &nr_card_auten,
                            &wait_transf_conf, &nr_card_dest, &suma,
                            &ret_err_tcp);
                    } else if (wait_secret_pass) {
                        // Dacă se așteaptă parola secretă, următorul mesaj va
                        // fi trimis sigur prin UDP și procesarea sa pentru TCP
                        // este inutilă
                        ret_str_udp = parse_from_terminal_udp(buf,
                                nr_card_curent, wait_secret_pass,
                                &ret_err_udp);
                    } else {
                        // Dacă nu se așteaptă nimic, se procesează comanda
                        // pentru TCP și UDP
                        ret_str_tcp = parse_from_terminal_tcp(buf, &is_logged, 
                            &client_running, &nr_card_curent, &nr_card_auten,
                            &wait_transf_conf, &nr_card_dest, &suma,
                            &ret_err_tcp);

                        ret_str_udp = parse_from_terminal_udp(buf,
                                nr_card_curent, wait_secret_pass,
                                &ret_err_udp);
                    }


                    // Dacă nu s-a identificat nicio comandă pentru TCP sau UDP
                    // atunci comanda este invalidă și se afișează eroare
                    if (ret_str_tcp == NULL && ret_str_udp == NULL) {
                        char *err = get_error(ERR_FUNCTIE, IBANK);
                        printf("%s\n\n", err);
                        fprintf(log_file, "%s\n\n", err);
                        free(err);
                    }


                    // Se identifică ce fel de comandă a fost introdusă și se
                    // verifică dacă parametri sunt valizi. În caz afirmativ,
                    // se trimite mesaj către server, în caz negativ se afișează
                    // eroarea
                    if (ret_str_tcp != NULL) {
                        if (ret_err_tcp) {
                            printf("%s\n\n", ret_str_tcp);
                            fprintf(log_file, "%s\n\n", ret_str_tcp);
                        } else {
                            send_tcp_msg(tcp_socket, udp_socket, ret_str_tcp,
                                    buf);
                        }
                    } else if (ret_str_udp != NULL) {
                        if (ret_err_udp) {
                            printf("%s\n\n", ret_str_udp);
                            fprintf(log_file, "%s\n", ret_str_udp);
                        } else {
                            send_udp_msg(tcp_socket, udp_socket, ret_str_udp,
                                    server_sockaddr);
                        }
                    }

                    free(ret_str_tcp);
                    free(ret_str_udp);


                } else if (i == udp_socket) {
                    // Mesaj primit pe socketul UDP
                    memset(buf, 0, BUFLEN);

                    int len = sizeof(struct sockaddr);
                    int bytes_recv = recvfrom(udp_socket, buf, BUFLEN - 1, 0, 
                            (struct sockaddr *) &server_sockaddr, &len);

                    if (bytes_recv < 0) {
                        exit_with_error("Receptionarea mesajului prin " 
                                "UDP a esuat!", udp_socket, tcp_socket);

                    } else if (bytes_recv == 0) {
                        client_running = 0;
                        printf("Conexiunea inchisa de server!\n");

                    } else {
                        printf("%s\n\n", buf);
                        fprintf(log_file, "%s\n\n", buf);
                        parse_from_server_udp(buf, &wait_secret_pass);
                    }


                } else if (i == tcp_socket) {
                    // Mesaj primit pe socketul TCP
                    memset(buf, 0, BUFLEN);

                    int bytes_recv = recv(tcp_socket, buf, BUFLEN - 1, 0);
                    if (bytes_recv < 0) {
                        exit_with_error("Receptionarea mesajului prin " 
                                "TCP a esuat!", udp_socket, tcp_socket);

                    } else if (bytes_recv == 0) {
                        client_running = 0;
                        printf("Conexiunea inchisa de server!\n");

                    } else {
                        printf("%s\n\n", buf);
                        fprintf(log_file, "%s\n\n", buf);

                        parse_from_server_tcp(buf, &is_logged, &nr_card_auten, 
                                &nr_card_curent, &wait_transf_conf);
                    }
                }
            }
        }
    }

//---------------------------------------------------------------------------\\
//                               Oprire client                               \\
//---------------------------------------------------------------------------\\

    fclose(log_file);
    close(udp_socket);
    close(tcp_socket);

    return 0;
}
