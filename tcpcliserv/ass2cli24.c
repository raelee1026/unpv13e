#include    "unp.h"
#include    <string.h>

void exchange_data(FILE *fp, int sockfd){
    char    sendline[MAXLINE], recvline[MAXLINE];
    char    student_id[30] = "112550003";
    char    id_ip[30];
    char    local_ip[30];
    int     n, count = 0, maxfdp1;
    fd_set  rset; 
    struct sockaddr_in local_addr;
    socklen_t local_addrlen;

    // get local ip 
    local_addrlen = sizeof(local_addr);
    getsockname(sockfd, (SA *) &local_addr, &local_addrlen);
    char* ip_str = inet_ntoa(local_addr.sin_addr);
    strcpy(local_ip, ip_str);
    snprintf(id_ip, sizeof(id_ip), "%s %s\n", student_id, local_ip);
    // send student id + local ip
    printf("sent: %s", id_ip);
    Writen(sockfd, id_ip, strlen(id_ip));

    FD_ZERO(&rset);
    while(1) {
        FD_SET(fileno(fp), &rset);
        FD_SET(sockfd, &rset);
        maxfdp1 = max(fileno(fp), sockfd) + 1;
        Select(maxfdp1, &rset, NULL, NULL, NULL);

        if (FD_ISSET(sockfd, &rset)) { /* socket is readable */
            if ((n = Read(sockfd, recvline, MAXLINE)) == 0)
                err_quit("str_cli: server terminated prematurely");

            recvline[n] = '\n';
            recvline[n+1] = 0;
            printf("recv: %s\n", recvline);
            
            if(strcmp(recvline, "ok\n") == 0) {
                printf("successfully\n");
                return;
            } else if(strcmp(recvline, "bad\n") == 0) {
                count--;
                continue;
            } else if(strcmp(recvline, "stop\n") == 0) {
          sprintf(sendline, "%d ", count);
                // char *str = concat(sendline, inet_ntoa(clientAddr.sin_addr));
                snprintf(id_ip, sizeof(id_ip), "%s %s\n", sendline, local_ip);
                // str = concat(str, "\n");
                Writen(sockfd, id_ip, strlen(id_ip));
                printf("sent: %s\n", id_ip);
                continue;
            } else if(strcmp(recvline, "nak\n") == 0) {
                return;
            }

        }

        if (FD_ISSET(fileno(fp), &rset)) {  /* input is readable */
            if (Fgets(sendline, MAXLINE, fp) == NULL)
                return;     /* all done */
            Writen(sockfd, sendline, strlen(sendline));
            count++;
        }
    }
}

int main(int argc, char **argv){
    int                 sockfd;
    struct sockaddr_in  servaddr;

    if (argc != 2)
        err_quit("usage: tcpcli <IPaddress>");

    sockfd = Socket(AF_INET, SOCK_STREAM, 0);

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(SERV_PORT + 1);
    Inet_pton(AF_INET, argv[1], &servaddr.sin_addr);

    Connect(sockfd, (SA *) &servaddr, sizeof(servaddr));

    exchange_data(stdin, sockfd);       /* do it all */

    exit(0);
}