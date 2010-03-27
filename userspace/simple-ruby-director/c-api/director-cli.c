#include <stdio.h>      /* for printf() and fprintf() */
#include <sys/socket.h> /* for socket(), connect(), send(), and recv() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_addr() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <string.h>     /* for memset() */
#include <unistd.h>     /* for close() */
#include <errno.h>
#include "director-cli.h"

static int sock = -1;

int fail_with_error(char* error) {
	printf("%s (Latest errno %s)\n", error, strerror(errno));
	return 1;
}

int initialize(int serverPort) {
    struct sockaddr_in address;
    unsigned short port;
    int res;
    port = serverPort;

    if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
        return fail_with_error("Failed to create socket");

    memset(&address, 0, sizeof(address)); 
    address.sin_family      = AF_INET;         
    address.sin_addr.s_addr = inet_addr("127.0.0.1");
    address.sin_port        = htons(port);

    if ( (res = connect(sock, (struct sockaddr *) &address, sizeof(address))) < 0) {
	//printf("RRR: %d\n", res);
	sock = -1;
        return fail_with_error("Failed to connect");
    }
    
    return 0;
}

#define BUFFER_SIZE 4000

/*
  @key: Public key of the entity requesting the operation
  @permission_type: type of the permission requested, like filesystem ("fs"), connect ("connect"), etc...
  @operation: operation specific for the permission ("read", "write",....)
  @object: object on which the operation shall be performed, for example file in filesystem ("/tmp", "/home/test.dat",...)

  @returns
  1 has permission
  0 has not permission
  negative => Error occured
 */
int check_permission(char* key, char* permission_type, char* operation, char* object) {
    char buffer[BUFFER_SIZE];
    int str_len;
    int bytes_recv, recv_pos = 0;

    if ( sock < 1 )
	return -fail_with_error("Not connected");

    snprintf(buffer, BUFFER_SIZE, "check entity key %s permission type %s operation %s object %s\n", key, permission_type, operation, object);

    str_len = strlen(buffer);

    if (send(sock, buffer, str_len, 0) != str_len)
        return -fail_with_error("send() failed");

    // Read until we reach "\n" which marks the end of response
    while (1)
    {
        if ((bytes_recv = recv(sock, buffer+recv_pos, BUFFER_SIZE - 1 -recv_pos, 0)) <= 0)
            return -fail_with_error("recv() failed");

        recv_pos += bytes_recv;	
	if ( buffer[recv_pos-1] == '\n' ) {
            buffer[recv_pos] = '\0';        
	    break;
	}
    }

    str_len = strlen(buffer);
    if ( str_len < 10 ) {	
	return -fail_with_error("Invalid data recieved");
    }
    // Just checks a first char of "granted" word.. some more robust detection? ;)
    return buffer[7] == 'g';
}

void finalize() {
    if ( sock < 1 )
	return;

    close(sock);
    sock = -1;
}

