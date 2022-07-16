#include <stdio.h>
#include <string.h>
#ifdef __linux__
#include <arpa/inet.h>
#elif _WIN32
#include <winsock2.h>
#endif

#include "utils/utils.h"
#include "servers.h"

#ifdef _WIN32
typedef int socklen_t;
#endif

void udp_info_serve(const int port)
{
    int sockfd;
    struct sockaddr_in servaddr, cliaddr;
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        error("UDP socket creation failed");
        return;
    }

#ifdef DEBUG_MODE
    puts("UDP socket created");
#endif

    memset(&servaddr, 0, sizeof(servaddr));
    memset(&cliaddr, 0, sizeof(cliaddr));

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htons(INADDR_ANY);
    servaddr.sin_port = htons(port);

    if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
    	char errmsg[32];
    	sprintf(errmsg, "Can\'t bind to port %i UDP", port);
        error(errmsg);
    }

#ifdef DEBUG_MODE
    puts("UDP bind completed");
#endif

    const size_t info_len = strlen(INFO_NAME);
    
    int n;
    socklen_t len;
    const int buf_sz = 8;
    char buffer[buf_sz];
#ifdef DEBUG_MODE
    puts("UDP server started");
#endif
    while (1)
    {
        len = sizeof(cliaddr);
#ifdef __linux__
        n = recvfrom(sockfd, (char *)buffer, 2, MSG_WAITALL, (struct sockaddr *)&cliaddr, &len);
#elif _WIN32
        n = recvfrom(sockfd, (char *)buffer, 2, 0, (struct sockaddr *)&cliaddr, &len);
#endif
        if (n<0){
            continue;
        }

        buffer[n] = '\0';

#ifdef DEBUG_MODE
        if (n>0) printf("Client UDP message : %s\n", buffer);
#endif

        if (strcmp(buffer, "in"))
        {
            continue;
        }

        sendto(sockfd, INFO_NAME, info_len, 0, (const struct sockaddr *)&cliaddr, len);
    }
}