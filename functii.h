#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <string.h>
#include <time.h>

int get_uid(char user[16]);
char* get_username(int id);
int get_file_size(char path[30]);
char* get_current_time();
void encrypt(char pass[26], int key);
void decrypt(char pass[26], int key);
int newuser(char user[16], char pass[26]);
int login(char user[16], char pass[26]);
int send_pm(char user[16], char text[320]);
void view_unread(char user[16], char response[1000]);
void add_history(char path[60], char text[320]);
int view_history(char user1[16], char user2[16], char response[1000]);