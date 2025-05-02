#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/select.h>

#define TRUE 1
#define FALSE 0

int main(int argc, char *argv[])
{
	const char *port = "65001";		/* available for our use */
	char *host;
	struct addrinfo hints,*server;
	int r,sockfd,done;
	char buffer[BUFSIZ];
	fd_set read_fd;

	if( argc<2 )
	{
		fprintf(stderr,"Format: client hostname\n");
		exit(1);
	}
	host = argv[1];

	/* obtain and convert server name and port */
	printf("Looking for chat server on %s...",host);
	memset( &hints, 0, sizeof(hints) );		/* use memset_s() */
	hints.ai_family = AF_INET;				/* IPv4 */
	hints.ai_socktype = SOCK_STREAM;		/* TCP */
	r = getaddrinfo( host, port, &hints, &server );
	if( r!=0 )
	{
		perror("failed");
		exit(1);
	}
	puts("found");

	/* create a socket */
	sockfd = socket(server->ai_family,server->ai_socktype,server->ai_protocol);
	if( sockfd==-1 )
	{
		perror("failed");
		exit(1);
	}

	/* connect to the socket */
	r = connect(sockfd,server->ai_addr,server->ai_addrlen);
	freeaddrinfo(server);
	if( r==-1 )
	{
		perror("failed");
		exit(1);
	}

	/* loop to interact with the server */
	done = FALSE;
	while(!done)
	{
		/* initialize file descriptor set */
		FD_ZERO(&read_fd);
		FD_SET(sockfd, &read_fd);		/* add the socket */
		/* this step must be done to ensure that the fgets() call doesn't block */
		FD_SET(0, &read_fd);			/* add standard input */
		r = select( sockfd+1, &read_fd, NULL, NULL, 0);
		if( r==-1 )
		{
			perror("failed");
			exit(1);
		}

		/* remote input */
		if( FD_ISSET(sockfd, &read_fd) )
		{
			r = recv( sockfd, buffer, BUFSIZ, 0);
			/* check for zero input, disconnection */
			if( r<1 )
			{
				puts("Connection closed by peer");
				break;
			}
			/* otherwise, cap the buffer and output it */
			buffer[r] = '\0';
			printf("%s",buffer);
		}

		/* local input */
		if( FD_ISSET(0, &read_fd) )
		{
			/* don't send an empty line */
			if( fgets(buffer,BUFSIZ,stdin)==NULL )
			{
				putchar('\n');
			}
			/* if 'done' is input, close the loop and quit */
			else if( strcmp(buffer,"close\n")==0 )
			{
				done=TRUE;
			}
			/* otherwise, send the input string - including the newline */
			else
			{
				send(sockfd,buffer,strlen(buffer),0);
			}
		}
	}	/* end while loop */

	/* all done, clean-up */
	printf("Disconnected\nBye!\n");
	close(sockfd);

	return(0);
}
