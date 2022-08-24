//gcc -o main main.c -lssl -lcrypto
#include "spider.h"


/* ---------------------------------------------------------- *
 * First we need to make a standard TCP socket connection.    *
 * create_socket() creates a socket & TCP-connects to server. *
 * ---------------------------------------------------------- */

int ssl_connect(char* url, ssl_info* SI, char* host)
{
    //同網域的話 ssl 連線只須建立一次
    //再抓網址時若是 http 需判斷 是否同網域
    //將 host_name 變成全域變數，讓程式可以更簡化

    if (!SI)
        return ERR_SSL_INFO;

    const SSL_METHOD* method;
    int ret;
    int i;

  /* ---------------------------------------------------------- *
   * These function calls initialize openssl for correct work.  *
   * ---------------------------------------------------------- */
    OpenSSL_add_all_algorithms();
    ERR_load_BIO_strings();
    ERR_load_crypto_strings();
    SSL_load_error_strings();

  /* ---------------------------------------------------------- *
   * initialize SSL library and register algorithms             *
   * ---------------------------------------------------------- */
    if(SSL_library_init() < 0)
    {
#ifdef DEBUG
        printf("Could not initialize the OpenSSL library !\n");
#endif

        return ERR_SSL_CONNCET;
    }

  /* ---------------------------------------------------------- *
   * Set SSLv2 client hello, also announce SSLv3 and TLSv1      *
   * ---------------------------------------------------------- */
    //method = SSLv23_client_method();
  
  /* ---------------------------------------------------------- *
   * SSLv23_client_method is deprecated function                *
   * Set TLS client hello by andric                             *
   * ---------------------------------------------------------- */
    method = TLS_client_method();

  /* ---------------------------------------------------------- *
   * Try to create a new SSL context                            *
   * ---------------------------------------------------------- */
    if ((SI->ctx = SSL_CTX_new(method)) == NULL)
    {
#ifdef DEBUG
        printf("Unable to create a new SSL context structure.\n");
#endif
        return ERR_SSL_CONNCET;
    }

  /* ---------------------------------------------------------- *
   * Disabling SSLv2 will leave v3 and TSLv1 for negotiation    *
   * ---------------------------------------------------------- */
    SSL_CTX_set_options(SI->ctx, SSL_OP_NO_SSLv2);

  /* ---------------------------------------------------------- *
   * Create new SSL connection state object                     *
   * ---------------------------------------------------------- */
    SI->ssl = SSL_new(SI->ctx);

  /* ---------------------------------------------------------- *
   * Make the underlying TCP socket connection                  *
   * ---------------------------------------------------------- */
    SI->server = create_socket(url, host);
    if (SI->server == -1)
        return ERR_SSL_CONNCET;
    else if (SI->server)
        printf("Successfully made the TCP connection to: %s\n", url);

  /* ---------------------------------------------------------- *
   * Attach the SSL session to the socket descriptor            *
   * ---------------------------------------------------------- */
    SSL_set_fd(SI->ssl, SI->server);

  /* ---------------------------------------------------------- *
   * Set to Support TLS SNI extension by Andric                 *
   * ---------------------------------------------------------- */
    SSL_ctrl(SI->ssl, SSL_CTRL_SET_TLSEXT_HOSTNAME, TLSEXT_NAMETYPE_host_name, (void*)host);

  /* ---------------------------------------------------------- *
   * Try to SSL-connect here, returns 1 for success             *
   * ---------------------------------------------------------- */
    if ((ret = SSL_connect(SI->ssl)) != 1)
    {
        int err;
#ifdef DEBUG
        printf("Error: Could not build a SSL session to: %s\n", url);
#endif
        /* ---------------------------------------------------------- *
         * Print SSL-connect error message by andric                  *
         * ---------------------------------------------------------- */
        err = SSL_get_error(SI->ssl, ret);
        if(err == SSL_ERROR_SSL)
        {              
            ERR_load_crypto_strings();
            SSL_load_error_strings();   //just once
            char msg[1024];
            ERR_error_string_n(ERR_get_error(), msg, sizeof(msg));
            printf("%s %s %s %s\n", msg, ERR_lib_error_string(0), ERR_func_error_string(0), ERR_reason_error_string(0));
        }
        return ERR_SSL_CONNCET;
    }
    else
#ifdef DEBUG
        printf("Successfully enabled SSL/TLS session to: %s\n", url);
#endif
    return SUCCESS;
}


/* ---------------------------------------------------------- *
 * create_socket() creates the socket & TCP-connect to server *
 * ---------------------------------------------------------- */
int create_socket(char* url, char* host_url)
{
    int sockfd;
    char portnum[6] = "443";
    char proto[6] = "";
    char *tmp_ptr = NULL;
    int port;
    struct hostent* host;
    struct sockaddr_in dest_addr;

  /* ---------------------------------------------------------- *
   * Remove the final / from url, if there is one           *
   * ---------------------------------------------------------- */
    if (url[strlen(url) - 1] == '/')
        url[strlen(url) - 1] = '\0';

  /* ---------------------------------------------------------- *
   * the first : ends the protocol string, i.e. http            *
   * ---------------------------------------------------------- */
    //strncpy(proto, url, (strchr(url, ':') - url));                  //proto 存 http/https (意義不明，待刪)

  /* ---------------------------------------------------------- *
   * the host_url starts after the "://" part                   *
   * ---------------------------------------------------------- */
    //strncpy(host_url, strstr(url, "://") + 3, sizeof(host_url));    //前端 :// 以前去掉，儲存在 host_url

  /* ---------------------------------------------------------- *
   * if the host_url contains a colon :, we got a port number   *
   * ---------------------------------------------------------- */
    if (strchr(host_url, ':'))
    {
        tmp_ptr = strchr(host_url, ':');
        /* the last : starts the port number, if avail, i.e. 8443 */
        strncpy(portnum, tmp_ptr + 1,  sizeof(portnum));
        *tmp_ptr = '\0';
    }

    char* host_end = strchr(host_url, '/');
    if(host_end)
        *host_end = '\0';

    port = atoi(portnum);

    if ((host = gethostbyname(host_url)) == NULL)
    {
        printf("Error: Cannot resolve host_url %s.\n",  host_url);
        abort();
    }

  /* ---------------------------------------------------------- *
   * Print host_url iP address by Andric                        *
   * ---------------------------------------------------------- */
    //print_ip(host);
  
  /* ---------------------------------------------------------- *
   * create the basic TCP socket                                *
   * ---------------------------------------------------------- */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(port);
    dest_addr.sin_addr.s_addr = *(long*)(host->h_addr);

  /* ---------------------------------------------------------- *
   * Zeroing the rest of the struct                             *
   * ---------------------------------------------------------- */
    memset(&(dest_addr.sin_zero), '\0', 8);

    tmp_ptr = inet_ntoa(dest_addr.sin_addr);

  /* ---------------------------------------------------------- *
   * Try to make the host connect here                          *
   * ---------------------------------------------------------- */
    if (connect(sockfd, (struct sockaddr *) &dest_addr, sizeof(struct sockaddr)) == -1)
        printf("Error: Cannot connect to host %s [%s] on port %d.\n", host_url, tmp_ptr, port);

    return sockfd;
}


