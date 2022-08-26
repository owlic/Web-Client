#include "spider.h"
#include "spider.c"
#include "ssl.c"


int main(int argc, char* argv[])
{
    if(argc != 4)
        return ERR_ARGC;
    
    if(!argv[1])
        return ERR_URL_NAME;
    
    if(!argv[2])
        return ERR_FOLDER_NAME;
    
    int process_total = atoi(argv[3]);
    if (process_total < 1)
        return ERR_PROCESS_NUMBER;

    strncpy(folder, argv[2], FOLDER_NAME_SIZE);
    if (folder[strlen(folder) - 1] != '/')
        folder[strlen(folder)] = '/';
    
    make_padding_EOL(PID_MAX_LIMIT);
    printf("padding_EOL size: %d\n", padding_EOL[0]);

    if (mkdir(argv[2], S_IRWXU | S_IRWXG | S_IRWXO))
        printf("%s\n", strerror(errno));

    char list_file[MAX_NAME_SIZE];

    sprintf(list_file, "%s%s", folder, LIST_NAME);
    FILE* fptr = fopen(list_file, "w+");
    if (!fptr)
        return ERR_OPEN_FAIL;
    printf("URL: %s\n", argv[1]);

    fwrite(padding_SOL, sizeof(padding_SOL), 1, fptr);
    fwrite(argv[1], strlen(argv[1]), 1, fptr);
    fwrite(padding_EOL + 1, (int)padding_EOL[0], 1, fptr);

    fclose(fptr);

    pid_t pid;
    int i;

    for(i = 0; i < process_total; i++)
	{
		pid = fork();
        if (pid == -1)
		    printf("process No.%d: fork error\n", i);
		if (pid == 0)
			child_crawling();
	}
 
	if (pid)
		parent_waiting(process_total);

    return 0;
}

void sub_quit_signal_handle(int sig)
{
	int status;
	int quit_pid = wait(&status);

    if (status)
    {
        printf("Child process %d quit, exit status %d\n", quit_pid, status);
        pid_t pid = fork();
        if (pid == -1)
		    printf("new fork error\n");
        else if (pid == 0)
            child_crawling();     //有辦法傳參數進去? sub_quit_signal_handle 只接受一 int 參數
        else
            process_reborn++;
    }
    else
       process_complete++;
}

void parent_waiting(int process_total)
{
	printf("Parent process: %d\n", getpid());
	signal(SIGCHLD, sub_quit_signal_handle);
    while (1)
    {
        if (process_complete == process_total)
            break;
        
        if (process_reborn > MAX_REBORN_TIMES)
        {
            printf("process reborn limit !\n");
            break;
        }

        sleep(1);
    }
}

void connect_fail_handle(char* list_file, int seek_loc)
{
    FILE* fptr = fopen(list_file, "r+");
    if (!fptr)
        printf("---------------\nfopen error !\n---------------\n");

    fseek(fptr, seek_loc, SEEK_SET);
    fprintf(fptr, "%c", STATUS_FAIL);
    fclose(fptr);
}

void child_crawling()
{
    sleep(1);      //防止 child 先跑
	printf("create sub process id: %d, parent id: %d\n", getpid(), getppid());

    char list_file[MAX_NAME_SIZE];
    sprintf(list_file, "%s%s", folder, LIST_NAME);

    char record_file[MAX_NAME_SIZE];
    sprintf(record_file, "%s%s%d%s", folder, FNAME_TAG, getpid(), ".txt");

    int attempt = 0;

    while(attempt <= 10)
    {
        FILE* fptr = fopen(list_file, "r+");
        if (!fptr)
        {
            printf("---------------\nfopen error !\n---------------\n");
            exit(1);
        }
    
        if (flock(fileno(fptr), LOCK_EX))
        {
            printf("---------------\nflock error !\n---------------\n");
            exit(1);
        }

        struct stat fstat;
        if (stat(list_file, &fstat))
            printf("%s\n", strerror(errno));

        int fsize = fstat.st_size;
        if (fsize <= 0)
        {
            printf("---------------\nlist file empty !\n---------------\n");
            exit(1);
        }
    
        char* list_buf = malloc(fsize);
        if (!list_buf)
            printf("---------------\nmalloc fail !\n---------------\n");
    
        if (fread(list_buf, fsize, 1, fptr) != 1)
            printf("---------------\nfread error !\n---------------\n");

        char* url_loc = strstr(list_buf, padding_SOL);
        char* file_loc = strchr(url_loc + sizeof(padding_SOL), '\t');

        if (url_loc && file_loc)
        {
            int seek_loc = url_loc - list_buf;
#ifdef DEBUG
            printf("---------------\nseek_loc: %d\n", seek_loc);
#endif
            fseek(fptr, seek_loc, SEEK_SET);
            fprintf(fptr, "%c", STATUS_CRAWLING);
            fseek(fptr, file_loc - url_loc, SEEK_CUR);

            fprintf(fptr, "%s%d", FNAME_TAG, getpid());

            flock(fileno(fptr), LOCK_UN);
            printf("---------------\nLOCK_UN OK\n");
            fclose(fptr);

            //將 url_list 中的網址取出

            url_loc += sizeof(padding_SOL);
            char* url_end = strchr(url_loc, '\t');
            if (url_end == NULL)
            {
                printf("---------------\nURL capture error !\n");
                exit(1);
            }

            printf("---------------\nurl_end: %p\n", url_end);
            int url_size = url_end - url_loc;
            printf("---------------\nurl_size: %d\n", url_size);
            char url[url_size + 1];

            strncpy(url, url_loc, url_size);
            url[url_size] = '\0';
            free(list_buf);

            //將 url_list 中的網址抓下
            printf("抓取 URL: %s\n", url);

            char host[MAX_URL_SIZE];    //統一 size，不然 define 好雜
            char path[MAX_URL_SIZE];
            get_host_n_path(url, host, MAX_URL_SIZE, path, MAX_URL_SIZE);
            printf("----------\nhost: %s\n", host);
            printf("----------\npath: %s\n", path);

            ssl_info SI;

            if (ssl_connect(url, &SI, host) == ERR_SSL_CONNCET)
            {
                connect_fail_handle(list_file, seek_loc);
                continue;
            }

            char url_move[MAX_URL_SIZE] = {0};    //遇到網址轉移時

            char* content = malloc(MAX_CONTENT_SIZE);
            int content_size = 0;
            
            crawler(SI.ssl, host, path, record_file, url_move, 
                    SI.server, content, &content_size);
            if (url_move[0])
            {
                ssl_disconnect(&SI, url);
                get_host_n_path(url_move, host, MAX_URL_SIZE, path, MAX_URL_SIZE);
#ifdef DEBUG
                printf("----------\nNEW host: %s\n", host);
                printf("----------\nNEW path: %s\n", path);
#endif
                if (ssl_connect(url, &SI, host) == ERR_SSL_CONNCET)
                {
                    connect_fail_handle(list_file, seek_loc);
                    continue;
                }
                crawler(SI.ssl, host, path, record_file, url_move, SI.server, content, &content_size);
            }

            char* link_buf = calloc(BUF_SIZE, sizeof(char));
            printf("link_buf: %p\n---------------\n", link_buf);

            if (!link_buf)
                printf("---------------\nmalloc fail !\n---------------\n");
       
            int link_buf_size = 0;
            search_link(content, &content_size, link_buf, &link_buf_size);

            printf("link_buf:\n%s\n", link_buf);
            printf("---------------\nlink_buf_size: %d\n---------------\n", link_buf_size);
#ifdef DEBUG
            if (link_buf_size > 10000)
                sleep(3);
#endif

            fptr = fopen(list_file, "r+");
            if (!fptr)
            {
                printf("---------------\nfopen error !\n---------------\n");
                exit(1);
            }

            if (flock(fileno(fptr), LOCK_EX))
            {
                printf("---------------\nflock error !\n---------------\n");
                exit(1);
            }

            struct stat fstat;
            stat(list_file, &fstat);
            if (stat(list_file, &fstat))
                printf("%s\n", strerror(errno));
            
            int fsize = fstat.st_size;
            if (fsize <= 0)
            {
                printf("---------------\nlist file empty !\n---------------\n");
                return;
            }

            list_buf = malloc(fsize + 1);
            if (!list_buf)
            {
                printf("---------------\nmalloc fail !\n---------------\n");
                return;
            }

            if (fread(list_buf, fsize, 1, fptr) != 1)
            {
                printf("---------------\nfread error !\n---------------\n");
                return;
            }
            list_buf[fsize] = '\0';

            //將狀態改成完成
            fseek(fptr, seek_loc, SEEK_SET);
            fprintf(fptr, "%c", STATUS_FINISH);
            fseek(fptr, 0, SEEK_END);

            if (link_buf[0])     //將網址寫回 list_file
            {
                char* curr_ptr = link_buf;
                printf("curr_ptr: %p\n---------------\n", curr_ptr);

                while (curr_ptr < link_buf + link_buf_size)
                {
                    char* link_end = strchr(curr_ptr, '\n');
                    if (link_end == NULL)
                        break;
                    int link_size = link_end - curr_ptr;
#ifdef DEBUG
                    printf("link_end: %p\n---------------\n", link_end);
                    printf("link_size: %d\n---------------\n", link_size);
#endif
                    if (link_size <= 0)
                    {
                        printf("link_size error!\n---------------\n");
                        exit(1);
                    }

                    char link[link_size + 1];
                    strncpy(link, curr_ptr, link_size);
                    link[link_size] = '\0';

                    char* repeat_link = strstr(list_buf, link);
                    if (repeat_link == NULL)
                    {
                        fwrite(padding_SOL, sizeof(padding_SOL), 1, fptr);
                        fwrite(link, link_size, 1, fptr);
                        fwrite(padding_EOL + 1, (int)*padding_EOL, 1, fptr);
                    }
                    curr_ptr = link_end + 1;
                }
            }

            flock(fileno(fptr), LOCK_UN);
            fclose(fptr);

            if (url_move[0])
                ssl_disconnect(&SI, url_move);
            else
                ssl_disconnect(&SI, url);
        }
        else
        {
            flock(fileno(fptr), LOCK_UN);
            fclose(fptr);

            url_loc = strstr(list_buf, "~\t");
            free(list_buf);

            if (url_loc)
            {
                sleep(1);
                attempt++;
            }
            else
                exit(0);        //子行程結束
        }
    }
    exit(1);
}

void make_padding_EOL(long long num)
{
    int length = 0;
    while (num)
    {
        length++;
        num /= 10;
    }
    length += strlen(FNAME_TAG);
    padding_EOL = malloc((length + 5) * sizeof(char));
    memset(padding_EOL, ' ', length + 5);
    padding_EOL[0] = (char)(length + 3);
    padding_EOL[1] = '\t';
    padding_EOL[length + 3] = '\n';
    padding_EOL[length + 4] = '\0';
}

