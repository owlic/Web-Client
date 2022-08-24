//gcc -o main main.c -lssl -lcrypto
#include "spider.h"


/* ---------------------------------------------------------- *
 * First we need to make a standard TCP socket connection.    *
 * create_socket() creates a socket & TCP-connects to server. *
 * ---------------------------------------------------------- */

int ssl_connect(char* url, ssl_info* SI, char* host)
{

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
    if (SI->server)
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
        printf("Successfully enabled SSL/TLS session to: %s\n", url);

    return SUCCESS;
}

