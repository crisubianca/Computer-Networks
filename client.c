#include "functii.c"

#define port 2908
extern int errno;

int sd;
char name[16];

void send_data();
void recv_data();

int main()
{
    int uid=-1;
    char initial[52], char_uid[4], *command, *username, *password;
    pthread_t send_thread, recv_thread;

    // intro dupa ./client

    printf("~OFFLINE MESSENGER~\n");
    printf("Pentru a te inregistra foloseste comanda: #register <username>/<parola>\n ");
    printf("Pentru a te autentifica foloseste comanda: #login <username>/<parola>\n");


    do {
        // citim din terminal si verificam ce comanda a introdus utilizatorul
        bzero(initial, 52); fgets(initial, 52, stdin); initial[strlen(initial)-1]='\0';
        command=strtok(initial, " ");
        username=strtok(NULL, "/");
        password=strtok(NULL, "");

        if(strcmp(command, "#register")!=0 && strcmp(command, "#login")!=0 && strcmp(command, "#exit")!=0)
            printf("Comanda nu exista.\n");
        else if(strcmp(command, "#exit")==0)
            exit(0);
        else if(username==NULL || password==NULL)
            printf("Sintaxa este incorecta! Incearca: %s <username>/<password>.\n", command);
        else if(strcmp(command, "#register")==0) 
        {
            // apelam functia newuser ce creeaza un cont nou si trece credentialele in fisierul users.txt
            int result=newuser(username, password);

            // vreificam cazurile in care credentialele nu respecta cerintele
            if(result==-3)
                printf("Username-ul nu poate depasi lungimea de 15 caractere.\n");
            else if(result==-2)
                printf("Parola nu poate depasi lungimea de 25 de caractere.\n");
            else if(result==-1)
                printf("Username-ul contine caractere care nu sunt permise.(Ex.: spatii sau '\\').\n");
            else if(result==0)
                printf("Deja exista acest utilizator.\n");
            else
                // daca totul este in regula, transmitem utilizatorului ca trebuie sa se autentifice pentru a folosi aplicatia(comenzile)
                printf("Contul a fost creat cu succes. Acum te poti autentifica pentru a folosi chat-ul.\n");
        }
        else if(strcmp(command, "#login")==0) 
        {
            // printf("am ajuns aici\n");
            // apelam functia login ce autentifica utilizatorul si ii permite sa acceseze comenzile
            uid=login(username, password);
            printf("sunt aici\n");
            // verificam cazurile in care utilizatorul nu se autentifica corect
            if(uid==-2)
                {printf("Utilizatorul nu exista.\n");
                printf("am ajuns aici\n");}
            else if(uid==-1)
                {printf("Username-ul sau parola este/sunt incorecta/e.\n");
                printf("ba nu, am ajuns aici\n");}

            // printf("am ajuns aici\n");
        }
    } while(uid<0);

    // cream socket-ul sd
    sd=socket(AF_INET, SOCK_STREAM, 0);
    if(sd==-1)
    {
        perror("[client]Eroare la crearea socket-ului.\n");
        return errno;
    }

    // "umplem" structura sockaddr pentru a putea realiza conexiunea intre client si server
    struct sockaddr_in server;
    server.sin_family=AF_INET;
    server.sin_addr.s_addr=inet_addr("127.0.0.1");
    server.sin_port=htons(port);

    // connect conecteaza socket descriptorul la adresa server-ului
    // o data ce conexiunea este stabilita si clientul s-a logat cu succes, incepe comunicarea client-server
    if(connect(sd, (struct sockaddr *)&server, sizeof(server))==-1)
    {
        perror("[client]Eroarea la conectarea cu server-ul.\n");
        return errno;
    }

    // trimiterea si primirea de mesaje se face folosind functiile send si respectiv, recv
    bzero(name, 16); bzero(char_uid, 4);
    strcat(name, username); sprintf(char_uid, "%d", uid);
    int sendname=send(sd, name, 16, 0);
    int senduid=send(sd, char_uid, 4, 0);
    if(sendname==-1 || senduid==-1)
    {
        perror("[client]Eroare la scrierea catre server.\n");
        return errno;
    }

    printf("Incearca '#meniu' pentru a vedea ce comenzi poti folosi.\n\n");

    // crearea thread-urilor
    
    if(pthread_create(&send_thread, NULL, (void *)send_data, NULL)!=0)
    {
        printf("[client]Eroare la crearea thread-ului(send_data).\n");
        return errno;
    }

    if(pthread_create(&recv_thread, NULL, (void *)recv_data, NULL)!=0)
    {
        printf("[client]Eroare la crearea thread-ului(recv_data).\n");
        return errno;
    }

    while(1)
    {
        // aplicatia ruleaza pana cand un utilizator tasteaza "#exit"
    }

    close(sd);
}

void send_data()
{
    char message[250], buffer[270];

    while(1)
    {
        fgets(message, 250, stdin); message[strlen(message)-1]='\0';
        if(strcmp(message, "#exit")==0)
        {
            char path[40];
            bzero(path, 40); strcat(path, "users/"); strcat(path, name); strcat(path, "_unread.txt");
            FILE *unreadfile=fopen(path, "w"); // fisierul e gol atunci cand utilizatorul inchide aplicatia
            fclose(unreadfile);
            exit(0);
        }
        else if(strncmp(message, "#register", 9)==0 || strncmp(message, "#login", 6)==0)
            printf("Esti deja autentificat! Daca vrei sa creezi un nou cont, mai intai trebuie sa parasesti chat-ul.\n");
        else
        {
            if(strncmp(message, "#help", 5)==0 || strncmp(message, "#view", 5)==0 || strncmp(message, "#to", 3)==0 ||
            strncmp(message, "#history", 8)==0 || strncmp(message, "#chat-history", 13)==0 || strncmp(message, "#online-users", 13)==0) 
                sprintf(buffer, "%s", message);
            else 
                sprintf(buffer, "%s %s: %s\n", get_current_time(), name, message);

            if(send(sd, buffer, strlen(buffer), 0)==-1)
            {
                perror("[client]Eroare la scrierea catre server.\n");
                exit(0);
            }
        }
        bzero(message, 250); bzero(buffer, 270);
    }
}

void recv_data()
{
    char buffer[1000], path[40];

    bzero(path, 40); strcat(path, "users/"); strcat(path, name); strcat(path, "_unread.txt");
    if(get_file_size(path)>0)
        printf("Ai mesaje noi! Incearca: '#view' pentru a le citi.\n");

    while(1)
    {
        int length=recv(sd, buffer, 1000, 0);
        if(length==-1)
        {
            perror("[client]Eroare la citirea de la server.\n");
            exit(0);
        }
        else if(length==0)
            break;
        else
        {
            printf("%s", buffer); fflush(stdout);
        }
        bzero(buffer, 1000);
    }
}