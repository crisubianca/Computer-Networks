#include "functii.h"

int get_uid(char user[16])
{
    char line[50];

    FILE *fptr=fopen("users/users.txt", "r");
    if(fptr==NULL)
    {
        perror("Eroare la deschiderea fisierului users.txt.\n");
        return errno;
    }
    while(fgets(line, 50, fptr))
    {
        char *uid=strtok(line, " ");
        char *username=strtok(NULL, " ");
        if(strcmp(user, username)==0)
            return atoi(uid);
    }
    fclose(fptr);

    return -1;
}

char* get_username(int id)
{
    char line[50];
    static char user[16];

    FILE *fptr=fopen("users/users.txt", "r");
    if(fptr==NULL)
    {
        perror("Eroare la deschiderea fisierului users.txt.\n");
        exit(errno);
    }
    while(fgets(line, 50, fptr))
    {
        char *uid=strtok(line, " ");
        char *username=strtok(NULL, " ");

        if(atoi(uid)==id)
        {
            bzero(user, 16);
            strcat(user, username);
            break;
        }
    }
    fclose(fptr);
    return user;
}

int get_file_size(char path[40])
{
    char line[320];
    int content=0;

    FILE *fptr=fopen(path, "a+");
    if(fptr==NULL)
    {
        perror("Eroare la deschiderea fisierului.\n");
        return 0;
    }
    while(fgets(line, 320, fptr))
        content++;
    fclose(fptr);

    return content;
}

char* get_current_time()
{
    time_t now;
    static char timestamp[21];
    struct tm *tm_info;

    now=time(NULL);
    tm_info=localtime(&now);

    strftime(timestamp, 21, "[%d/%m  %H:%M]", tm_info);
    return timestamp;
}

void encrypt(char pass[26], int key)
{
    for(int i=0;i<strlen(pass);i++)
        pass[i]=pass[i]+key+100;
}

void decrypt(char pass[26], int key)
{
    // printf("suntem la decrypt\n");
    for(int i=0;i<strlen(pass);i++)
        pass[i]=pass[i]-key-100;
}

int newuser(char user[16], char pass[26])
{
    char line[50], new[50], path[40];

    if(strlen(user)>15)
        return -3;
    if(strlen(pass)>25)
        return -2;
    if(strchr(user, ' ')!=NULL || strchr(user, '\\')!=NULL)
        return -1;

    FILE *fptr=fopen("users/users.txt", "a+");
    if(fptr==NULL)
    {
        perror("Eroare la deschiderea fisierului users.txt.\n");
        return errno;
    }
    while(fgets(line, 50, fptr))
    {
        char *uid=strtok(line, " ");
        char *username=strtok(line, " ");
        if(strcmp(user, username)==0)
            return 0;
    }
    bzero(new, 50);
    sprintf(new, "%d", get_file_size("users/users.txt")); strcat(new, " ");
    strcat(new, user); strcat(new, " ");
    encrypt(pass, get_file_size("users/users.txt")); strcat(new, pass); strcat(new, "\n");
    fputs(new, fptr);
    fclose(fptr);

    return 1;
}

int login(char user[16], char pass[26])
{
    char line[50], password[26];

    FILE *fptr=fopen("users/users.txt", "a+");
    // printf("hello\n");
    if(fptr==NULL)
    {
        perror("Eroare la deschiderea fisierului users.txt.\n");
        return errno;
    }
    // printf("hello\n");
    while(fgets(line, 50, fptr))
    {
        // printf("hello\n");
        char *uid=strtok(line, " ");
        // printf("hello\n");
        char *username=strtok(NULL, " ");
        // printf("hello\n");
        char *password=strtok(NULL, "\n"); 
        // printf("hello\n");
        decrypt(password, atoi(uid));
        printf("hello\n");
        if(strcmp(user, username)==0 && strcmp(pass, password)==0)
            return atoi(uid);
    }
    fclose(fptr);

    if(get_uid(user)==-1)
        return -2;
    return -1;
}

int send_pm(char user[16], char text[320])
{
    char line[50], path[40], char_mid[5];
    int mid, val=-1;

    FILE *fptr=fopen("users/users.txt", "r");
    if(fptr==NULL)
    {
        perror("Eroare la deschiderea fisierului users.txt.\n");
        return errno;
    }
    while(fgets(line, 50, fptr))
    {
        char *uid=strtok(line, " ");
        char *username=strtok(NULL, " ");

        if(strcmp(username, user)==0)
        {
            val=atoi(uid);
            break;
        }
    }
    fclose(fptr);

    if(val==-1)
        return -1;

    bzero(path, 40); strcat(path, "users/"); strcat(path, user); strcat(path, "_unread.txt");
    mid=get_file_size(path);

    FILE *unreadfile=fopen(path, "a");
    if(unreadfile==NULL)
    {
        perror("Eroare la trimiterea mesajului.\n");
        return errno;
    }
    sprintf(char_mid, "%d", mid);
    fputs("[", unreadfile); fputs(char_mid, unreadfile); fputs("] ", unreadfile); fputs(text, unreadfile);
    fclose(unreadfile);

    return val;
}

void view_unread(char user[16], char response[1000])
{
    char line[320], path[40];

    bzero(path, 40); strcat(path, "users/"); strcat(path, user); strcat(path, "_unread.txt");

    FILE *unreadfile=fopen(path, "a+"); //create file if it doesn't exist
    if(unreadfile==NULL)
    {
        perror("Eroare la vizualizarea mesajelor necitite.\n");
        exit(errno);
    }
    if(get_file_size(path)==0)
        sprintf(response, "Nu ai niciun mesaj necitit!\n");
    else
    {
        while(fgets(line, 320, unreadfile))
            strcat(response, line);
    }
    fclose(unreadfile);
}

void add_history(char path[60], char text[320])
{
    FILE *fptr=fopen(path, "a");
    if(fptr==NULL)
    {
        perror("Eroare la adaugarea mesajelor in istoric.\n");
        exit(errno);
    }
    fputs(text, fptr);
    fclose(fptr);
}

int view_history(char user1[16], char user2[16], char response[1000])
{
    char line[320], path[60];

    bzero(path, 60);
    if(strcmp(user1, "\\")==0)
        strcat(path, "users/history_chat.txt");
    else
    {
        if(user2!=NULL)
        {
            if(strcmp(user1, user2)==0)
                return view_history(user1, NULL, response);
            if(strcmp(user1, user2)>0)
            {
                char aux[16];
                strcpy(aux, user1); strcpy(user1, user2); strcpy(user2, aux);
            }
            if(get_uid(user2)==-1)
            {
                sprintf(response, "Utilizatorul '%s' nu exista.\n", user2);
                return -1;
            }
        }

        strcat(path, "users/"); strcat(path, user1);
        if(user2!=NULL)
        {
            strcat(path, "_");
            strcat(path, user2);
        }
        strcat(path, "_history.txt");
    }

    FILE *fptr=fopen(path, "a+"); //create file if it doesn't exist
    if(fptr==NULL)
    {
        perror("Eroare la vizualizarea istoricului.\n");
        return errno;
    }
    if(get_file_size(path)==0)
        sprintf(response, "Nu exista mesaje!\n");
    else
    {
        while(fgets(line, 320, fptr))
            strcat(response, line);
    }
    fclose(fptr);

    return 1;
}