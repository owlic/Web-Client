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

void url_convert(char* new_url, char* url)
{
    char* ptr = new_url;

    for(int i = 0; i <= strlen(url); i++)
    {
        switch (url[i])
        {
            case '/':   strcpy(ptr, "∕");
                        ptr += strlen("∕");
                        break;
            case '?':   strcpy(ptr, "？");
                        ptr += strlen("？");
                        break;
            case ':':   strcpy(ptr, "：");
                        ptr += strlen("：");
                        break;
            case '*':   strcpy(ptr, "＊");
                        ptr += strlen("＊");
                        break;
            default:    *ptr++ = url[i];
        }
    }
}

int SSL_read_n_write(SSL* ssl, char* response, char* content, int* content_size)
{
    int ret = SSL_read(ssl, response, RESPONSE_SIZE);
    if(ret <= 0)
        SSL_get_error(ssl, ret);    //不能用 strerror(errno) 來查看，會出現 SUCCESS
    memmove(content + *content_size, response, ret);
    *content_size += ret;
    return ret;
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

void general_case(SSL* ssl, char* response, FILE* fptr, int server, char* content, int* content_size)
{
    char* transfer_mode = NULL;
    if (transfer_mode = strstr(response, "Transfer-Encoding: chunked"))
    {
        char* ending = NULL;
        while((ending = strstr(response, "0\r\n\r\n")) == NULL)     //不能完全判斷
            SSL_read_n_write(ssl, response, content, content_size);
    }
    else if ((transfer_mode = strstr(response, "Content-Length: ")) ||
             (transfer_mode = strstr(response, "content-length: ")))
    {
        char* size_loc = transfer_mode + 16;
        int content_length = atoi(size_loc);

        char* HTTP_body = strchr(size_loc, '\n');
        content_length -= *content_size - (HTTP_body - response);
        while (content_length > 0)
            content_length -= SSL_read_n_write(ssl, response, content, content_size);
    }
    else
    {
        while (1)
        {
            struct timeval timeout;
            timeout.tv_sec = 3;
            timeout.tv_usec = 0;
            setsockopt(server, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));     //設置超時時間
            SSL_read_n_write(ssl, response, content, content_size);
        }
    }
}

void get_host_n_path(char* url, char* host, int host_size, char* path, int path_size)
{
    memset(host, 0, host_size);
    memset(path, 0, path_size);

    char* ptr;
    ptr = strstr(url, "//");
    if (ptr)
        ptr += 2;
    else
        ptr = url;

    strncpy(host, ptr, host_size - 1);    //如果 src 字元數比 count 少，會把剩下未滿 count 的部分通通補上 '\0'
    host[host_size - 1] = '\0';

    ptr = strchr(host, '/');
    if (ptr)
    {
        strncpy(path, ptr + 1, path_size - 1);
        path[path_size - 1] = '\0';
        *ptr = '\0';
    }

    ptr = strchr(host, ':');
    if (ptr)
        *ptr = '\0';
}

bool is_target(char* url, int url_len)
{
    char original_char = *(url + url_len + 1);
    *(url + url_len + 1) = '\0';

    bool qualified = true;

    char* type[] = {".png", ".jpg", ".gif", ".css", "css?", 
                    ".js?", ".pdf", ".woff", ".ico",
                    "google.com.tw/maps", "#", " "};
    int type_count = sizeof(type) / sizeof(char*);

    for (int i = 0; i < type_count; i++)
    {
        if (strstr(url, type[i]))
        {
            qualified = false;
            break;
        }
    }
    
    *(url + url_len + 1) = original_char;
    return qualified;
}

int check_EOL(char* search_ptr, int size, char* link)
{
    int i = 0;
    int link_size = 0;

    for (; i < size; i++)
    {
        if (search_ptr[i] == '\r')
            break;
        link[link_size++] = search_ptr[i];
    }

    if (i == size)
        return size;

#ifdef DEBUG
    printf("search_str: ");
    for (int k = 0; k < size; k++)
        printf("%c", search_ptr[k]);
    printf("\n");
#endif

    for (; i < size; i++)
    {
        if (search_ptr[i] == '\n')
            break;
    }

    i++;
    printf("--------------------\nlink[i]: %d\n", link[i]);

    for (; i < size; i++)
    {
        if (search_ptr[i] == '\n')   //cause google map
            return 0;
        link[link_size++] = search_ptr[i];
    }
    
#ifdef DEBUG
    printf("--------------------\nlink_size: %d\n", link_size);
    printf("--------------------\nlink: \n");
    for (int j = 0; j < link_size; j++)
        printf("%c", link[j]);
#endif

    return link_size;
}

void search_link(char* content, int* content_size, char* link_buf, int* link_buf_size)
{
    char host[MAX_URL_SIZE];
    char path[MAX_URL_SIZE];
    char link_file[MAX_NAME_SIZE];
    char* store_ptr = link_buf;                             //用來儲存
    char* search_ptr = strstr(content, "href=\"http");      //用來查詢
    char* link_end;
    int size;
    int link_size;
    
#ifdef DEBUG
    printf("store_ptr (start): %p\n", store_ptr);
#endif

    while(search_ptr)
    {
        search_ptr += 6;
        link_end = strchr(search_ptr, '\"');
        if (link_end == NULL)
            break;
        
        size = link_end - search_ptr;

        char link[size + 1];
        link_size = check_EOL(search_ptr, size, link);
        link[link_size] = '\0';
        //為了在加入連結時就檢查是否有重覆，這裡就需使用 strstr()
        //因此特地讓 link 最後補上 \0，但不會複製到 link_buf

        if (link_size > 100)
            printf("-------------------- WARNING !!! --------------------\n"
            "%s\n-------------------- WARNING !!! --------------------\n", link);

        if (link_size && is_target(link, link_size))
        {
            if (strstr(link_buf, link) == NULL)
            {
                strncpy(store_ptr, link, link_size);
                store_ptr += link_size;
                *store_ptr++ = '\n';
            }
        }

        search_ptr = strstr(search_ptr, "href=\"http");
    }

    search_ptr = strstr(content, "'http");

    while(search_ptr)
    {
        search_ptr += 1;
        link_end = strchr(search_ptr, '\'');
        if (link_end == NULL)
            break;
        
        if (strncmp(link_end - 5, ".html", 5) == 0)
        {
            size = link_end - search_ptr;

            char link[size + 1];
            link_size = check_EOL(search_ptr, size, link);
            link[link_size] = '\0';

            if (link_size && is_target(search_ptr, link_size))
            {
                if (strstr(link_buf, link) == NULL)
                {
                    strncpy(store_ptr, link, link_size);
                    store_ptr += link_size;
                    *store_ptr++ = '\n';
                }
            }
        }

        search_ptr = strstr(search_ptr, "'http");
    }

    *link_buf_size = store_ptr - link_buf;

    *store_ptr++ = '\0';

#ifdef DEBUG
    printf("store_ptr (end): %p\n", store_ptr);
#endif

}

void search_link_extend(char* content, int* content_size, char* link_buf, int* link_buf_size, char* url)
{
    char* store_ptr = link_buf;
#ifdef DEBUG
    printf("store_ptr (start): %p\n", store_ptr);
#endif

    char* link_end;
    int size;
    int link_size;

    char* keyword[] = {"href=", "window.open(", "window.location = "};
    int keyword_count = sizeof(keyword) / sizeof(char*);

    for (int i = 0; i < keyword_count; i++)
    {
        char* search_ptr = strstr(content, keyword[i]);

        while(search_ptr)
        {

#ifdef DEBUG
            printf("--------------------\nsearch_ptr: %p\n--------------------\n", search_ptr);
#endif

            search_ptr += strlen(keyword[i]);
            bool path = false;

            //while(*search_ptr++ == ' ');

            if (*search_ptr == '\"')
            {
                search_ptr += 1;
                link_end = strchr(search_ptr, '\"');
            }
            else if (*search_ptr == '\'')
            {
                search_ptr += 1;
                link_end = strchr(search_ptr, '\'');
            }
            else
            {
                if (*search_ptr == '\\')
                {
                    if (*(search_ptr + 1) == '\"' || *(search_ptr + 1) == '\'')
                    {
                        search_ptr += 2;
                        link_end = strchr(search_ptr, '\\');
                    }
                }
                else if (*search_ptr == '/')
                {
                    search_ptr += 1;
                    path = true;
                }

                char* candidate1 = strchr(search_ptr, ' ');
                char* candidate2 = strchr(search_ptr, '>');

                link_end = (candidate1 < candidate2) ? candidate1 : candidate2;
            }

            if (strncmp(search_ptr, "javascript", strlen("javascript")) == 0 ||
                *search_ptr == '#')
            {
                search_ptr++;
                continue;
            }

            if (link_end == NULL)
                break;


            size = link_end - search_ptr;
            if (path)
                size += strlen(url);
            
            char link[size + 1];
            
            if (path)
            {
                strncpy(link, url, strlen(url));
                link_size = check_EOL(search_ptr, size, link + strlen(url));
                link_size += strlen(url);
            }
            else
                link_size = check_EOL(search_ptr, size, link);
        
            link[link_size] = '\0';

#ifdef DEBUG
            if (link_size > 100)
                printf("-------------------- WARNING !!! --------------------\n");
#endif

            printf("--------------------\nlink: %s\n--------------------\n", link);


            if (link_size && is_target(link, link_size))
            {
                if (strstr(link_buf, link) == NULL)
                {
                    strncpy(store_ptr, link, link_size);
                    store_ptr += link_size;
                    *store_ptr++ = '\n';
                }
            }

            search_ptr = strstr(search_ptr, keyword[i]);
        }
    }

    *link_buf_size = store_ptr - link_buf;

    *store_ptr++ = '\0';

#ifdef DEBUG
    printf("store_ptr (end): %p\n", store_ptr);
#endif

}


