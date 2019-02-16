/*  Biblioteca contine funcții care ajută la tratarea erorilor întâmpinate pe
    parcursul derulării programelor server și client.*/

#ifndef ERR_LIB
#define ERR_LIB

// Coduri eroare
#define ERR_NEAUTENTIFICAT -1
#define ERR_SESINE_DEJA_DESCHISA -2
#define ERR_PIN_GRESIT -3
#define ERR_NR_CARD_INEXISTENT -4
#define ERR_CARD_BLOCAT -5
#define ERR_OPERATIE_ESUATA -6
#define ERR_DEBLOCARE_ESUATA -7
#define ERR_FONDURI_INSUFICIENTE -8
#define ERR_OPERATIE_ANULATA -9
#define ERR_FUNCTIE -10

// Coduri pentru header-ele erorilor
#define IBANK 1
#define UNLOCK 2

// Dimensiune maximă buffer
#define BUFLEN 256

/*
 *      Creează un string care conține numele serviciului care a generat
 * eroarea, codul erorii și explicația erorii.
 *
 *  Paramteri:
 *      err_code: codul erorii
 *      head_msg = tipul serviciului care a generat eroarea
 *
 *  Return:
 *      Adresa către un string alocat dinamic
 */
char* get_error(int err_code, int head_msg) {

    char head[BUFLEN];
    memset(head, 0, BUFLEN);

    char tail[BUFLEN];
    memset(tail, 0, BUFLEN);

    switch(head_msg) {
        case IBANK:
            strcpy(head, "IBANK");
            break;

        case UNLOCK:
            strcpy(head, "UNLOCK");
            break;
    }

    switch(err_code) {
        case ERR_NEAUTENTIFICAT:
            strcpy(tail, "Clientul nu este autentificat");
            break;

        case ERR_SESINE_DEJA_DESCHISA:
            strcpy(tail, "Sesiune deja deschisa");
            break;

        case ERR_PIN_GRESIT:
            strcpy(tail, "Pin gresit");
            break;

        case ERR_NR_CARD_INEXISTENT:
            strcpy(tail, "Numar card inexistent");
            break;

        case ERR_CARD_BLOCAT:
            strcpy(tail, "Card blocat");
            break;

        case ERR_OPERATIE_ESUATA:
            strcpy(tail, "Operatie esuata");
            break;

        case ERR_DEBLOCARE_ESUATA:
            strcpy(tail, "Deblocare esuata");
            break;

        case ERR_FONDURI_INSUFICIENTE:
            strcpy(tail, "Fonduri insuficiente");
            break;

        case ERR_OPERATIE_ANULATA:
            strcpy(tail, "Operatie anulata");
            break;

        case ERR_FUNCTIE:
            strcpy(tail, "Parametri invalizi");
            break;
    }

    char final_str[BUFLEN];
    sprintf(final_str, "%s> %d : %s", head, err_code, tail);
    return strdup(final_str);
}

/*
 *      Iese din program închizând socket-ii, dacă au fost deschiși, și
 * afișează un mesaj de eroare la stderr.
 */
void exit_with_error(char *err_msg, int udp, int tcp) {
    fprintf(stderr, "Eroare! %s\n", err_msg);

    if (udp != -1) {
        close(udp);
    }

    if (tcp != -1) {
        close(tcp);
    }

    exit(ERR_FUNCTIE);
}

#endif
