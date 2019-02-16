/*  Biblioteca contine funcții care sunt folosite de server și clienți pentru a
    decodifica mesajele trimise.*/

#ifndef HELP_LIB
#define HELP_LIB

// Dimensiune maximă buffer
#define BUFLEN 256

/*
 *      Preia dintr-un string care conține comanda „login” numărul cardului și
 *      PIN-ul acestuia.
 *
 * Paramteri:
 *      str = șir de caractere care conține cuvântul „login” și doua numere
 *          naturale separate prin spațiu
 *      nr_card = numărul cardului returnat ca parametru
 *      pin = PIN-ul cardului returnat ca parametru
 *
 * Return:
 *      0 dacă șirul conține după „login” două numere naturale, -1 altfel
 */
int decode_login(char *str, int *nr_card, int *pin) {

    char copy[strlen(str) + 1];
    char *ptr = copy;
    memset(copy, 0, strlen(copy));
    strcpy(copy, str);

    if ((ptr = strtok(ptr, " ")) == NULL) {
        return -1;
    }

    if ((ptr= strtok(NULL, " ")) == NULL) {
        return -1;
    }
    *nr_card = atoi(ptr);

    if ((ptr = strtok(NULL, " \n")) == NULL) {
        return -1;
    }
    *pin = atoi(ptr);

    return 0;
}

/*
 *      Preia dintr-un string care conține comanda „transfer” numărul cardului și
 *      suma ce se dorește a fi trimisă.
 *
 * Paramteri:
 *      str = șir de caractere care conține cuvântul „transfer”, un număr natural
 *          și un număr real pozitiv cu cel mult 2 zecimale separate prin spațiu
 *      nr_card = numărul cardului returnat ca parametru
 *      suma = suma ce se dorește a fi transferată returnată ca parametru
 *
 * Return:
 *      0 dacă șirul conține după „transfer” un număr natural și un număr real
 *      pozitiv cu cel mult 2 zecimale, -1 altfel
 */
int decode_transfer(char *str, int *nr_card, double *suma) {

    char copy[strlen(str) + 1];
    char *ptr = copy;
    memset(copy, 0, strlen(copy));
    strcpy(copy, str);

    if ((ptr = strtok(ptr, " ")) == NULL) {
        return -1;
    }

    if ((ptr = strtok(NULL, " ")) == NULL) {
        return -1;
    }
    *nr_card = atoi(ptr);

    if ((ptr = strtok(NULL, " \n")) == NULL) {
        return -1;
    }
    *suma = atof(ptr);

    return 0;
}

/*
 *      Preia dintr-un string care conține comanda „transfer” numărul cardului
 *      destinatarului, suma ce se dorește a fi trimisă și numărul cardului 
 *      expeditorului.
 *
 * Paramteri:
 *      str = șir de caractere care conține cuvântul „transfer”, un număr
 *          natural și un număr real pozitiv cu cel mult 2 zecimale separate
 *          prin spațiu
 *      nr_card_dest = numărul cardului destinatarului returnat ca parametru
 *      suma = suma ce se dorește a fi transferată returnată ca parametru
 *      nr_card_sursa = numărul cardului expeditorului returnat ca parametru
 *
 * Return:
 *      0 dacă șirul conține după „transfer” un număr natural, un număr real
 *      pozitiv cu cel mult 2 zecimale și un alt număr natural, -1 altfel
 */
int decode_transfer_server(char *str, int *nr_card_dest, double *suma, 
        int *nr_card_sursa) {

    if (decode_transfer(str, nr_card_dest, suma) < 0) {
        return -1;
    }

    char *ptr = strrchr(str, ' ');
    if (ptr == NULL) {
        return -1;
    }
    *nr_card_sursa = atoi(ptr + 1);

    return 0;
}

/*
 *      Preia dintr-un string care conține comanda „logout” sau „listsold”
 *      numărul cardului.
 *
 * Paramteri:
 *      str = șir de caractere care conține cuvântul „logout” sau „listsold” și
 *          un număr natural separate prin spațiu
 *      nr_card = numărul cardului returnat ca parametru
 *
 * Return:
 *      0 dacă șirul conține după „logout” sau „listsold” un număr natural,
 *      -1 altfel
 */
int decode_nr_card(char *str, int *nr_card) {

    char copy[strlen(str) + 1];
    char *ptr = copy;
    memset(copy, 0, strlen(copy));
    strcpy(copy, str);

    if ((ptr = strtok(ptr, " ")) == NULL) {
        return -1;
    }

    if ((ptr = strtok(NULL, " ")) == NULL) {
        return -1;
    }
    *nr_card = atoi(ptr);

    return 0;
}

/*
 *      Preia dintr-un string care conține comanda de deblocare a contului,
 *      numărul cardului care dorește a fi deblocat și parola secretă a
 *      clientului care deține cardul respectiv.
 *
 * Paramteri:
 *      str = șir de caractere care conține comanda de deblocare a contului, un
 *          număr de card și un șir de caractere
 *      nr_card = numărul cardului returnat ca parametru
 *      parola = șirul de caractere reprezentând parola returnată ca parametru
 *
 * Return:
 *      0 dacă șirul conține comanda de deblocare a contului, un număr natural
 *      și un șir de caractere separate prin spațiu, -1 altfel
 */
int decode_unlock_pass(char *str, int *nr_card, char **parola) {

    char copy[strlen(str) + 1];
    char *ptr = copy;
    memset(copy, 0, strlen(copy));
    strcpy(copy, str);

    if ((ptr = strtok(ptr, " ")) == NULL) {
        return -1;
    }

    if ((ptr = strtok(NULL, " ")) == NULL) {
        return -1;
    }
    *nr_card = atoi(ptr);

    if ((ptr = strtok(NULL, " \n")) == NULL) {
        return -1;
    }
    *parola = strdup(ptr);

    return 0;
}

#endif
