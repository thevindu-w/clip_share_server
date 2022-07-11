#ifndef _UTILS_
#define _UTILS_

extern void error(const char *);
extern int open_listener_socket();
extern void bind_port(int, int);
extern int getConnection(int);

extern int clip_server(const int);
extern int web_server(const int);

#endif