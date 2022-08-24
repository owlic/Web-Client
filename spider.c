#include "spider.h"


void crawler(SSL* ssl, char* host, char* path, char* record_file, char* url_move, int server, char* content, int* content_size)
{
    char request[REQUEST_SIZE];
    char response[RESPONSE_SIZE];

    make_request_message(request, host, path);

    //發送請求
    int ret = SSL_write(ssl, request, strlen(request));     //將緩衝區中的 num 個 byte 寫入指定的 ssl 連接
    if (ret)
        SSL_get_error(ssl, ret);                            //獲取 TLS/SSL I/O 操作的結果
    
    ret = SSL_read(ssl, response, RESPONSE_SIZE);
    if(ret <= 0)
        SSL_get_error(ssl, ret);

    //接收回應
    printf("----------\nResponse: (see PID-%d.txt)\n----------\n", getpid());
    
    char status_code_str[3];
    strncpy(status_code_str, response + 9, 3);
    int status_code = atoi(status_code_str);

    //fwrite(response, ret, 1, fptr);
    //general_case(ssl, response, fptr, server, ret);

    switch (status_code)
    {
        // case 200:
        // case 201:
        // {
        //     FILE* fptr = fopen(url_file, "w+");
        //     if (!fptr)
        //         return ERR_OPEN_FAIL;
        //     fwrite(response, ret, 1, fptr);
        //     general_case(ssl, response, fptr, server, ret);
        //     fclose(fptr);
        //     break;
        // }
        case 301:
        case 302:
        {
            char* new_url_loc = strstr(response, "Location: ");
            new_url_loc += 10;
            char* new_url_end = strchr(new_url_loc, '\n');
            int new_url_len = new_url_end - new_url_loc;
            // printf("------------\nnew_url_len: %d\n", new_url_len);
            strncpy(url_move, new_url_loc, new_url_len);
            url_move[new_url_len - 1] = '\0';      //注意: 在 \n 前面是 \r
            printf("------------\nurl_move: %s\n", url_move);
            // printf("------------\nurl_move[new_url_len - 1]: %d\n", url_move[new_url_len - 1]);
            break;
        }
        default:
        {
            memset(url_move, 0, MAX_URL_SIZE);
            FILE* fptr = fopen(record_file, "a+");
            if (!fptr)
                printf("---------------\nfile open fail !\n---------------\n");

            fprintf(fptr, "\n--------------------------------------------------\n%s/%s"
                          "\n--------------------------------------------------\n", host, path);

            //fwrite(response, ret, 1, fptr);

            memmove(content, response, ret);
            *content_size += ret;

            general_case(ssl, response, fptr, server, content, content_size);
            *(content + *content_size) = '\0';      //每個網址內容以'\0'結尾
            (*content_size)++;

            printf("----------\ncontent_size: %d\n----------\n", *content_size);

            fwrite(content, *content_size, 1, fptr);
            fclose(fptr);

            break;
        }
        // printf("------------\nNOT SUPPORT !\n------------\n");
    }
}

void make_request_message(char* request, char* host, char* path)
{
    char* original_loc = request;

    strncpy(request, "GET /", 6);
    if (path[0])
        strncat(request, path, strlen(path) + 1);

    strcat(request, " HTTP/1.1\r\nHost: ");
    strncat(request, host, strlen(host));
    strcat(request, "\r\n\r\n");

    printf("----------\nRequest:\n----------\n%s\n", original_loc);
}

void make_file_name(char* url_file, char* host, char* path)
{
    char host_convert[MAX_NAME_SIZE];
    url_convert(host_convert, host);

    if (path[0])
    {
        char path_convert[MAX_NAME_SIZE];
        url_convert(path_convert, path);
        sprintf(url_file, "%s%s∕%s%s", folder, host_convert, path_convert, ".txt");
    }
    else
        sprintf(url_file, "%s%s%s", folder, host_convert, ".txt");
}

