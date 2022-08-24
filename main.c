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

    if (mkdir(argv[2], S_IRWXU | S_IRWXG | S_IRWXO))
        printf("%s\n", strerror(errno));

    char list_file[MAX_NAME_SIZE];
    sprintf(list_file, "%s%s", folder, LIST_NAME);
    FILE* fptr = fopen(list_file, "w+");
    if (!fptr)
        return ERR_OPEN_FAIL;
    printf("URL: %s\n", argv[1]);
    
    fprintf(fptr, "%s%s\n", padding, argv[1]);
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

