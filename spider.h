#ifndef SPIDER_H
#define SPIDER_H

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <signal.h>
#include <unistd.h>
#include <resolv.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/file.h>
#include <errno.h>

#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/x509_vfy.h>
#define SUCCESS 0
#define BASE 0
#define ERR_ARGC (BASE - 1)
#define ERR_URL_NAME (BASE - 2)
#define ERR_FOLDER_NAME (BASE - 3)
#define ERR_PROCESS_NUMBER (BASE - 4)
#define ERR_OPEN_FAIL (BASE - 5)
#define ERR_LOAD_FAIL (BASE - 6)
#define ERR_SAVE_FAIL (BASE - 7)
#define ERR_MALLOC_FAIL (BASE - 8)
#define ERR_SIZE_LIMIT (BASE - 9)
#define ERR_SSL_INFO (BASE - 11)
#define ERR_SSL_CONNCET (BASE - 12)
#define MAX_URL_SIZE (1 << 10)
#define MAX_NAME_SIZE 128
#define FOLDER_NAME_SIZE 96
#define MAX_CONTENT_SIZE (1 << 20)
#define MAX_RECORD_SIZE (1 << 25)
#define REQUEST_SIZE (1 << 10)      //記得括弧
#define RESPONSE_SIZE (1 << 14)     //16Kb
#define BUF_SIZE (1 << 18)
#define PID_MAX_LIMIT (1 << 23)
#define MAX_REBORN_TIMES 3

#define STATUS_FINISH '*'
#define STATUS_CRAWLING '~'
#define STATUS_FAIL '!'

#define LIST_NAME "url_list.txt"
#define DEBUG_

typedef struct ssl_info
{
    SSL* ssl;
    SSL_CTX* ctx;
    int server;
}ssl_info;

int ssl_connect(char*, ssl_info*, char*);
void ssl_disconnect(ssl_info*, char*);
int create_socket(char*, char*);
void print_ip(struct hostent*);
void get_host_n_path(char*, char*, int, char*, int);
void crawler(SSL*, char*, char*, char*, char*, int, char*, int*);
void make_request_message(char*, char*, char*);
void make_file_name(char*, char*, char*);
int SSL_read_n_write(SSL*, char*, char*, int*);
void general_case(SSL*, char*, FILE*, int, char*, int*);
void url_convert(char*, char*);
void sub_quit_signal_handle(int);
void parent_waiting(int);
void child_crawling();
bool is_target(char*, int);
int check_EOL(char*, int size, char* link);
int count_digit(long long);
void make_padding(long long);

int process_complete = 0;
int process_reborn = 0;
char folder[FOLDER_NAME_SIZE];
char padding[] = {' ', '\t'};

#endif

