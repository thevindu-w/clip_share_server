//#define _GNU_SOURCE
#ifndef NO_WEB
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include "utils.h"
#include "xclip/xclip.h"
#include "xscreenshot/screenshot.h"

#define FAIL -1

extern char blob_cert[];
extern char blob_key[];
extern char blob_page[];
extern int blob_size_page;

static void say(char *msg, SSL *ssl);
static SSL_CTX *InitServerCTX(void);
static void LoadCertificates(SSL_CTX *ctx);
static void ShowCerts(SSL *ssl);
static void receiver_web(SSL *ssl);
//int read_in(SSL *ssl, char *buf, int len);

static SSL_CTX *InitServerCTX(void)
{
    const SSL_METHOD *method;
    SSL_CTX *ctx;

    OpenSSL_add_all_algorithms(); /* load & register all cryptos, etc. */
    SSL_load_error_strings();     /* load all error messages */
    method = TLS_server_method(); /* create new server-method instance */
    ctx = SSL_CTX_new(method);    /* create new context from method */
    if (ctx == NULL)
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
    return ctx;
}

static void LoadCertificates(SSL_CTX *ctx)
{
    BIO *cbio = BIO_new_mem_buf((void *)blob_cert, -1);
    X509 *cert = PEM_read_bio_X509(cbio, NULL, 0, NULL);
    if (SSL_CTX_use_certificate(ctx, cert) <= 0)
    {
        ERR_print_errors_fp(stderr);
        abort();
    }

    BIO *kbio = BIO_new_mem_buf((void *)blob_key, -1);
    RSA *rsa = PEM_read_bio_RSAPrivateKey(kbio, NULL, 0, NULL);
    if (SSL_CTX_use_RSAPrivateKey(ctx, rsa) <= 0)
    {
        ERR_print_errors_fp(stderr);
        abort();
    }

    /* verify private key */
    if (!SSL_CTX_check_private_key(ctx))
    {
        fprintf(stderr, "Private key does not match the public certificate\n");
        abort();
    }
}

static void ShowCerts(SSL *ssl)
{
    X509 *cert;
    char *line;

    cert = SSL_get_peer_certificate(ssl); /* Get certificates (if available) */
    if (cert != NULL)
    {
        printf("Server certificates:\n");
        line = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0);
        printf("Subject: %s\n", line);
        free(line);
        line = X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0);
        printf("Issuer: %s\n", line);
        free(line);
        X509_free(cert);
    }
}

static void say(char *msg, SSL *ssl)
{
    if (SSL_write(ssl, msg, strlen(msg)) <= 0)
        error("send failed");
}

static void receiver_web(SSL *ssl)
{
    int sd;
    if (SSL_accept(ssl) < 0)
    { /* do SSL-protocol accept */
        // ERR_print_errors_fp(stderr);
        // goto END;
    }
    ShowCerts(ssl); /* get any certificates */

    char method[16];
    {
        int read = 0, r = 0, cnt = 0;
        char *ptr = method;
        while (read == 0 || r > 0)
        {
            r = SSL_read(ssl, ptr, 1);
            if (r > 0)
            {
                read = 1;
                if (*ptr == ' ')
                {
                    *ptr = '\0';
                    break;
                }
                ptr++;
            }
            else
            {
                if (cnt++ > 50)
                {
                    goto END;
                }
            }
        }
    }

    char path[2049];
    {
        int r, cnt = 0;
        char *ptr = path;
        do
        {
            if ((r = SSL_read(ssl, ptr, 1)) > 0)
            {
                if (*ptr == ' ')
                {
                    *ptr = '\0';
                    break;
                }
                ptr++;
            }
            else
            {
                if (cnt++ > 50)
                {
                    goto END;
                }
            }
        } while (r > 0);
    }

    if (!strcmp(method, "GET"))
    {
        char buf[128];
        while (SSL_read(ssl, buf, 128) > 0)
            ;
        if (!strcmp(path, "/"))
        {
            say("HTTP/1.0 200 OK\r\n", ssl);
            char tmp[96];                                                                                                              // = (char *)malloc(96);
            sprintf(tmp, "Content-Type: text/html; charset=utf-8\r\nContent-Length: %i\r\nConnection: close\r\n\r\n", blob_size_page); // Content-Disposition: attachment; filename="filename.ext" // put this, filename parameter is optional
            say(tmp, ssl);
            if (SSL_write(ssl, blob_page, blob_size_page) < blob_size_page)
            {
                error("send failed");
            }
        }
        else if (!strcmp(path, "/clip"))
        {
            unsigned long len;
            char *buf;
            if (xclip_util(1, NULL, &len, &buf))
            {
                say("HTTP/1.0 500 Internal Server Error\r\n\r\n", ssl);
                error("xclip read failed");
            }
            say("HTTP/1.0 200 OK\r\n", ssl);
            say("Content-Type: text/plain; charset=utf-8\r\n", ssl);
            char tmp[64];
            sprintf(tmp, "Content-Length: %lu\r\nConnection: close\r\n\r\n", len);
            say(tmp, ssl);
            if (SSL_write(ssl, buf, len) <= 0)
                error("send failed");
            if (len)
                free(buf);
        }
        else if (!strcmp(path, "/img"))
        {
            size_t len = 0;
            char *buf;
            if (xclip_util(1, "image/png", &len, &buf) || len == 0)
            {
                if (screenshot_util(&len, &buf) || len == 0)
                {
                    say("HTTP/1.0 404 Not Found\r\n\r\n", ssl);
                    goto END;
                }
            }
            say("HTTP/1.0 200 OK\r\n", ssl);
            say("Content-Type: image/png\r\nContent-Disposition: attachment; filename=\"clip.png\"\r\n", ssl);
            char tmp[64];
            sprintf(tmp, "Content-Length: %lu\r\nConnection: close\r\n\r\n", len);
            say(tmp, ssl);
            if (SSL_write(ssl, buf, len) <= 0)
            {
                fputs("send failed\n", stderr);
            }
            if (len)
                free(buf);
        }
        else
        {
            say("HTTP/1.0 404 Not Found\r\n\r\n", ssl);
            goto END;
        }
    }
    else if (!strcmp(method, "POST"))
    {
        unsigned int len = 2048;
        char *headers = (char *)malloc(len);
        *headers = 0;
        int r = 0, cnt = 0;
        char buf[256];
        {
            char *ptr = headers;
            char *check = ptr;
            while (1)
            {
                r = SSL_read(ssl, buf, 256);
                if (r > 0)
                {
                    //buf[r] = '\0';
                    memcpy(ptr, buf, r);
                    ptr += r;
                    *ptr = 0;
                    if (ptr - headers >= len - 256)
                        break;
                    cnt = 0;
                }
                else
                {
                    if (cnt == 0)
                    {
                        if (strstr(check, "\r\n\r\n"))
                            break;
                        check = ptr - 3;
                        check = check > headers ? check : headers;
                    }
                    else if (cnt++ > 50)
                    {
                        break;
                    }
                }
            }
        }
        unsigned long data_len;
        {
            char *cont_len_header = strstr(headers, "Content-Length: ");
            if (cont_len_header == NULL)
            {
                fputs("Content-Length header not found", stderr);
                goto END;
            }
            data_len = strtoul(cont_len_header + 16, NULL, 10);
        }
        char *data = strstr(headers, "\r\n\r\n");
        if (data == NULL)
        {
            fputs("HTTP Error", stderr);
            goto END;
        }
        data += 4;
        *(data - 1) = '\0';
        unsigned int header_len = (unsigned int)(data - headers);
        headers = (char *)realloc(headers, header_len + data_len + 1);
        data = headers + header_len;
        {
            cnt = 0;
            char *ptr = data + strlen(data);
            while (ptr - data < (long)data_len)
            {
                r = SSL_read(ssl, buf, 256);
                if (r > 0)
                {
                    //buf[r] = '\0';
                    memcpy(ptr, buf, r);
                    ptr += r;
                    *ptr = 0;
                    cnt = 0;
                }
                else
                {
                    if (cnt++ > 50)
                    {
                        fputs("Failed to receive data", stderr);
                        goto END;
                    }
                }
            }
        }

        // fprintf(stderr, "Expected : %lu\nGot : %lu\n", data_len, strlen(data));
        // puts(data);
        say("HTTP/1.0 204 No Content\r\n\r\n", ssl);
        sd = SSL_get_fd(ssl); /* get socket connection */
        SSL_free(ssl);        /* release SSL state */
        close(sd);
        if (xclip_util(0, NULL, &data_len, &data))
        {
            fputs("Failed to write to xclip", stderr);
        }
        free(headers);
        return;
    }
END:
    sd = SSL_get_fd(ssl); /* get socket connection */
    SSL_free(ssl);        /* release SSL state */
    close(sd);
}

int web_server(const int port)
{
    signal(SIGCHLD, SIG_IGN);
    int listener_d = open_listener_socket();
    bind_port(listener_d, port);
    if (listen(listener_d, 10) == -1)
        error("Can\'t listen");
    SSL_library_init();
    SSL_CTX *ctx = InitServerCTX();
    LoadCertificates(ctx); /* load certs */
    while (1)
    {
        int connect_d = getConnection(listener_d);
        pid_t p1 = fork();
        if (p1)
        {
            close(connect_d);
        }
        else
        {
            close(listener_d);
            SSL *ssl = SSL_new(ctx);
            SSL_set_fd(ssl, connect_d); /* set connection socket to SSL state */
            receiver_web(ssl);
            //close(connect_d);
            break;
        }
    }
    return 0;
}
#endif
