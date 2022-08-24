#ifndef SPIDER_H
#define SPIDER_H

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
#define REQUEST_SIZE (1 << 10)      //記得括弧
#define RESPONSE_SIZE (1 << 14)     //16Kb
#define BUF_SIZE (1 << 18)
#define MAX_REBORN_TIMES 3

#define STATUS_FINISH '*'
#define STATUS_CRAWLING '~'
#define STATUS_FAIL '!'

#define LIST_NAME "url_list.txt"
#define DEBUG_


#endif

