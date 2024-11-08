#include	"unp.h"
#include	<stdlib.h>
#include	<stdio.h>

void
sig_chld(int signo){
    pid_t   pid;
    int     stat;
    while ((pid = waitpid(-1, &stat, WNOHANG)) > 0);
    return;
}

int main(int argc, char **argv){
	int			        TCPlistenfd, TCPconnfd, UDPsockfd, n, buff_num;
	pid_t			    childpid;
	socklen_t		    clilen;
	struct sockaddr_in	cliaddr, TCPservaddr, UDPservaddr;
    char                buff[MAXLINE], recvline[MAXLINE], sendline[MAXLINE], ipaddr[MAXLINE], student_id[MAXLINE];
    time_t			    ticks;

    // Concurrent TCP Server
	TCPlistenfd = Socket(AF_INET, SOCK_STREAM, 0);
	bzero(&TCPservaddr, sizeof(TCPservaddr));
	TCPservaddr.sin_family      = AF_INET;
	TCPservaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	TCPservaddr.sin_port        = htons(SERV_PORT + 3);
	Bind(TCPlistenfd, (SA *) &TCPservaddr, sizeof(TCPservaddr));
	Listen(TCPlistenfd, LISTENQ);
    Signal(SIGCHLD, sig_chld);      /* must call waitpid() */

    // for UDP client
    if (argc != 2){
        err_quit("Usage: hw4cli <Server IP Address>\n");
    }
    bzero(&UDPservaddr, sizeof(UDPservaddr));
    UDPservaddr.sin_family      = AF_INET;
    UDPservaddr.sin_port        = htons(SERV_PORT + 3);
    Inet_pton(AF_INET, argv[1], &UDPservaddr.sin_addr.s_addr);
    UDPsockfd = Socket(AF_INET, SOCK_DGRAM, 0);

    // 連接 UDP socket，確保 getsockname 可正確獲取 IP
    Connect(UDPsockfd, (SA *) &UDPservaddr, sizeof(UDPservaddr));

    // 使用 getsockname 獲取綁定後的 UDP Client IP Address
    struct sockaddr_in local_addr;
    socklen_t len = sizeof(local_addr);
    getsockname(UDPsockfd, (struct sockaddr *) &local_addr, &len);
    char local_ip[INET_ADDRSTRLEN];
    Inet_ntop(AF_INET, &local_addr.sin_addr, local_ip, sizeof(local_ip));

    // send 11 + studentID + TCP Server IP address to Teacher's UDP server
    snprintf(sendline, sizeof(sendline), "11 112550003 %s", local_ip);
    Send(UDPsockfd, sendline, strlen(sendline), 0);
    printf("Sent: %s\n", sendline);

    // receive info n + studentID + IP address from Teacher's UDP server
    Recvfrom(UDPsockfd, recvline, MAXLINE, 0, NULL, NULL);
    printf("Recv: %s\n", recvline);
    sscanf(recvline, "%d %s %s", &n, student_id, ipaddr);

    // send status code to Teacher's UDP server
    if(n >= 90){
        printf("n is >= 90\n");
        Close(UDPsockfd);    // Close UDP socket  n >= 90
        exit(0);  
    }

    snprintf(sendline, sizeof(sendline), "13 112550003 %d", SERV_PORT + 3);
    // Sendto(UDPsockfd, sendline, strlen(sendline), 0, (SA *) &UDPservaddr, sizeof(UDPservaddr));
    Send(UDPsockfd, sendline, strlen(sendline), 0); 
    printf("Sent: %s\n", sendline);

    // fork n TCP server child and do things
	for (int i=0; i<n; i++) {
		clilen = sizeof(cliaddr);
        if ((TCPconnfd = accept(TCPlistenfd, (SA *) &cliaddr, &clilen)) < 0) {
            if (errno == EINTR)
                continue;               /* back to for() */
            else
                err_sys("accept error");
        }
		if ( (childpid = Fork()) == 0) {	/* child process */
			Close(TCPlistenfd);	/* close listening socket */
            ticks = time(NULL);
            printf ("===================\n%.24s: connected from %s, port %d\n",
                ctime(&ticks),
                Inet_ntop(AF_INET, &cliaddr.sin_addr, buff, sizeof (buff)),
                ntohs(cliaddr.sin_port));
            srand((int) ticks);
            
            // recv x
            Readline(TCPconnfd, recvline, MAXLINE);
            printf("Recv: %s", recvline);
            
            // get peer IP address
            clilen = sizeof(cliaddr);
            getpeername(TCPconnfd, (SA *) &cliaddr, &clilen);

            // send peer port# + x
            snprintf(sendline, sizeof(sendline), "%d %s", ntohs(cliaddr.sin_port), recvline); 
            printf("Sent: %s", sendline);
            Writen(TCPconnfd, sendline, strlen(sendline));

            // read result
            Readline(TCPconnfd, recvline, MAXLINE);
            printf("Recv: %s", recvline);

			exit(0);
		}
		Close(TCPconnfd);			/* parent closes connected socket */
	}
}