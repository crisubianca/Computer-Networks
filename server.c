#include "functii.c"

#define port 2908

extern int errno;

typedef struct
{
    char username[16];
    int fd;
    int uid;

} Client;

Client *clients[10];
pthread_mutex_t clients_mutex=PTHREAD_MUTEX_INITIALIZER;

void *handle_client(void *arg);
void add_client(Client *cl);
void remove_client(Client *cl);
void send_message(int uid, char message[240]);
void online_users(char response[1000]);

int main()
{
    pthread_t tid;
    int optval=1;

    int sd=socket(AF_INET, SOCK_STREAM, 0);
    if(sd==-1)
    {
        perror("Eroare la crearea socketului.\n");
        return errno;
    }

    setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    // intro dupa ./server

    printf("~Asteptam sa se conecteze clientii~\n\n");

    // "umplem" structura sockaddr pentru a putea realiza conexiunea intre client si server
    struct sockaddr_in server;
    server.sin_family=AF_INET;
    server.sin_addr.s_addr=inet_addr("127.0.0.1");
    server.sin_port=htons(port);

    // bind asigneaza adresa server la socket descriptorul sd
    if(bind(sd, (struct sockaddr *)&server, sizeof(server))==-1)
    {
        perror("[server]Eroare la bind().\n");
        return errno;
    }
    // cu listen preparam socket-ul sa accepte conexiuni de la client
    // 3 reprezinta nr maxim de clienti care se pot afla in coada
    if(listen(sd, 3)==-1)
    {
        perror("[server]Eroare la listen().\n");
        return errno;
    }

    while(1)
    {
        // cu accept luam un client din coada si stabilim o conexiune cu el
        int client=accept(sd, NULL, NULL);

        Client *cli=(Client *)malloc(sizeof(Client));
        cli->fd=client;

        // adaugam clientul in Client *clients[10];
        add_client(cli);

        // cream thread ul cu pthread_create
        pthread_create(&tid, NULL, &handle_client, (void*)cli);

        // reducem utilizarea CPU
        sleep(1); 
    }
}

void *handle_client(void *arg)
{
    char buffer[270], response[1000], name[16], char_uid[4]="-1";
    int offline=0;
    Client *cli=(Client *)arg;

    // trimiterea si primirea de mesaje se face folosind functiile send si respectiv, recv

    recv(cli->fd, name, 16, 0); strcpy(cli->username, name); //preluam username-ul
    recv(cli->fd, char_uid, 4, 0); cli->uid=atoi(char_uid); //preluam user id-ul

    sprintf(buffer, "Utilizatorul: %s este online.\n", cli->username);
    printf("[UID %d] %s", cli->uid, buffer); fflush(stdout);
    send_message(cli->uid, buffer);

    while(offline==0)
    {
        bzero(buffer, 270); bzero(response, 1000);
        int length=recv(cli->fd, buffer, 270, 0);

        if(length==-1)
        {
            perror("[server]Eroarea la citirea de la client.\n");
            offline=1;
        }
        else if(length==0)
        {
            sprintf(buffer, "Utilizatorul: %s este offline.\n", cli->username);
            printf("[UID %d] %s", cli->uid, buffer); fflush(stdout);
            send_message(cli->uid, buffer);
            offline=1;
        }
        else
        {
            char aux[270]; strcpy(aux, buffer);
            char *command=strtok(aux, " ");

            // comanda meniu pe care user-ul o poate folosi pentru a vizualiza functionalitatile aplicatiei
            if(strcmp(command, "#meniu")==0)
            {
                char *token=strtok(NULL, "");

                if(token!=NULL)
                    sprintf(response, "Sintaxa este incorecta! Incearca: #meniu.\n");
                else
                {
                    // functionalitatile aplicatiei
                    sprintf(response, "#meniu [vezi comenzile pe care le poti folosi]\n");
                    strcat(response, "#register <username>/<password> [creaza un cont nou]\n");
                    strcat(response, "#login <username>/<password> [logheaza-te in contul tau]\n");
                    strcat(response, "<text> [trimite un mesaj catre toti utilizatorii online]\n(*mesajul nu poate avea mai mult decat 230 de caractere)\n");
                    strcat(response, "#to <username>: <text> [trimite un mesaj privat catre utilizatorul <username> offline]\n");
                    strcat(response, "#view [vezi mesajele pe care le-ai primit cat timp ai fost offline]\n");
                    strcat(response, "#history [vezi istoricul mesajelor tale]\n");
                    strcat(response, "#history <username> [vezi istoricul mesajelor tale cu <username>]\n");
                    strcat(response, "#chat-history [vezi istoricul chat-ului]\n");
                    strcat(response, "#online-users [vezi toti utilizatorii online]\n");
                    strcat(response, "#exit [paraseste chat-ul]\n");
                }
            }
            else if(strcmp(command, "#to")==0) // implementarea functionalitatii #to: trimite mesaj privat de la un user catre altul
            {
                char *username=strtok(NULL, ":");
                char *text=strtok(NULL, "");

                if(username==NULL || text==NULL)
                    sprintf(response, "Sintaxa este incorecta! Incearca: #to <username>: <text>.\n");
                else
                {
                    int i, is_online=0;
                    for(i=0;i<10;i++)
                        if(clients[i]!=NULL && clients[i]->uid==get_uid(username))
                            is_online=1;

                    if(is_online==0)
                    {
                        sprintf(response, "%s %s:%s\n", get_current_time(), cli->username, text);
                        int result=send_pm(username, response);

                        if(result==-1)
                        {
                            bzero(response, 1000);
                            sprintf(response, "Utilizatorul:%s nu exista.\n", username);
                        }
                        else
                        {
                            char path_sender[60], path_both[60];

                            sprintf(path_sender, "users/%s_history.txt", cli->username);
                            if(strcmp(cli->username, username)>0)
                                sprintf(path_both, "users/%s_%s_history.txt", username, cli->username);
                            else sprintf(path_both, "users/%s_%s_history.txt", cli->username, username);

                            add_history(path_sender, response);
                            add_history(path_both, response);

                            bzero(response, 1000);
                            sprintf(response, "Mesajul a fost trimis catre %s.\n", username);
                            printf("%s a trimis un mesaj privat catre %s.\n", cli->username, username); fflush(stdout);
                        }
                    }
                    else sprintf(response, "Nu poti trmite mesaje private catre utilizatori online.\n");
                }
            }
            else if(strcmp(command, "#view")==0) // implementarea functionalitatii #view: vezi notif cat timp ai fost offline
            {
                char *token=strtok(NULL, "");

                if(token!=NULL)
                    sprintf(response, "Sintaxa este incorecta! Incearca: #view.\n");
                else view_unread(cli->username, response);
            }
            else if(strcmp(command, "#history")==0) // implementarea functionalitatii #history
            {
                char *username=strtok(NULL, "");

                if(username==NULL)
                    view_history(cli->username, NULL, response);
                else view_history(cli->username, username, response);
            }
            else if(strcmp(command, "#chat-history")==0) // implementarea functionalitatii #chat-history
            {
                char *token=strtok(NULL, "");

                if(token!=NULL)
                    sprintf(response, "Sintaxa este incorecta! Incearca: #chat-history.\n");
                else view_history("\\", NULL, response);
            }
            else if(strcmp(command, "#online-users")==0) // implementarea functionalitatii #online-users
            {
                char *token=strtok(NULL, "");

                if(token!=NULL)
                    sprintf(response, "Sintaxa este incorecta! Incearca: #online-users.\n");
                else online_users(response);
            }
            else // mesajele din chat
            {
                char path[60];
                sprintf(path, "users/%s_history.txt", cli->username);

                send_message(cli->uid, buffer);
                add_history("users/history_chat.txt", buffer);
                add_history(path, buffer);
                printf("Utilizatorul: %s a scris in chat.\n", cli->username); fflush(stdout);
            }

            if(send(cli->fd, response, strlen(response), 0)==-1)
            {
                perror("[server]Eroare la trimiterea mesajului.\n");
                offline=1;
            }
        }
    }

    close(cli->fd); 
    remove_client(cli);
    free(cli);
    pthread_detach(pthread_self());

    return NULL;
}

void add_client(Client *cl)
{
    pthread_mutex_lock(&clients_mutex);
    for(int i=0;i<10;i++)
        if(clients[i]==NULL)
        {
            clients[i]=cl;
            break;
        }
    pthread_mutex_unlock(&clients_mutex);
}

void remove_client(Client *cl)
{
    pthread_mutex_lock(&clients_mutex);
    for(int i=0;i<10;i++)
        if(clients[i]==cl)
        {
            clients[i]=NULL;
            break;
        }
    pthread_mutex_unlock(&clients_mutex);
}

void send_message(int uid, char message[240])
{
    pthread_mutex_lock(&clients_mutex);
    for(int i=0;i<10;i++)
        if(clients[i]!=NULL && clients[i]->uid!=uid)
            if(send(clients[i]->fd, message, strlen(message), 0)==-1)
            {
                perror("[server]Eroare la scrierea catre client.\n");
                break;
            }
    pthread_mutex_unlock(&clients_mutex);
}

void online_users(char response[1000])
{
    pthread_mutex_lock(&clients_mutex);
    for(int i=0;i<10;i++)
        if(clients[i]!=NULL)
        {
            strcat(response, clients[i]->username);
            strcat(response, "\n");
        }
    pthread_mutex_unlock(&clients_mutex);
}