#ifndef _UTILS_
#define _UTILS_

extern void error(const char *);

extern int clip_server(const int);
extern int web_server(const int);

extern int get_image(char **, size_t *);
extern int send_size(int, size_t);
extern long read_size(int socket);
extern int url_decode(char *);

#endif