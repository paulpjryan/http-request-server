
const char * usage =
"                                                               \n"
"myhttpd:                                                \n"
"                                                               \n"
"Simple server program that shows how to use socket calls       \n"
"in the server side.                                            \n"
"                                                               \n"
"To use it in one window type:                                  \n"
"                                                               \n"
"   daytime-server <port>                                       \n"
"                                                               \n"
"Where 1024 < port < 65536.             \n"
"                                                               \n"
"In another window type:                                       \n"
"                                                               \n"
"   telnet <host> <port>                                        \n"
"                                                               \n"
"where <host> is the name of the machine where daytime-server  \n"
"is running. <port> is the port number you used when you run   \n"
"daytime-server.                                               \n"
"                                                               \n"
"Then type your name and return. You will get a greeting and   \n"
"the time of the day.                                          \n"
"                                                               \n";


#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

int QueueLength = 5;

// Processes time request
void processRequest( int socket );

int main( int argc, char ** argv )
{
	// Print usage if not enough arguments
	if ( argc < 2 ) {
    		fprintf( stderr, "%s", usage );
    		exit( -1 );
  	}
  
  	// Get the port from the arguments
  	int port = atoi( argv[1] );
  
  	// Set the IP address and port for this server
  	struct sockaddr_in serverIPAddress; 
  	memset( &serverIPAddress, 0, sizeof(serverIPAddress) );
  	serverIPAddress.sin_family = AF_INET;
  	serverIPAddress.sin_addr.s_addr = INADDR_ANY;
  	serverIPAddress.sin_port = htons((u_short) port);
  
  	// Allocate a socket
  	int masterSocket =  socket(PF_INET, SOCK_STREAM, 0);
  	if ( masterSocket < 0) {
    		perror("socket");
    		exit( -1 );
  	}

  	// Set socket options to reuse port. Otherwise we will
  	// have to wait about 2 minutes before reusing the sae port number
  	int optval = 1; 
  	int err = setsockopt(masterSocket, SOL_SOCKET, SO_REUSEADDR, (char *) &optval, sizeof( int ) );
   
  	// Bind the socket to the IP address and port
  	int error = bind( masterSocket, (struct sockaddr *)&serverIPAddress, sizeof(serverIPAddress) );
  	if ( error ) {
    		perror("bind");
    		exit( -1 );
  	}
  
  	// Put socket in listening mode and set the 
  	// size of the queue of unprocessed connections
  	error = listen( masterSocket, QueueLength);
  	if ( error ) {
    		perror("listen");
    		exit( -1 );
  	}

	//MAIN LOOP
  	while (1) {
		// Accept incoming connections
    		struct sockaddr_in clientIPAddress;
    		int alen = sizeof( clientIPAddress );
    		int slaveSocket = accept( masterSocket, (struct sockaddr *)&clientIPAddress, (socklen_t*)&alen);

    		if ( slaveSocket < 0 ) {
      			perror( "accept" );
      			exit( -1 );
    		}

    		// Process request.
    		processRequest( slaveSocket );

    		// Close socket
    		close( slaveSocket );
  	}
  
}

void processRequest( int fd )
{
  	// Buffer used to store the name received from the client
  	const int maxLength = 1024;
  	char docpath[ maxLength + 1 ];
  	int length = 0;
  	int n;

  	// Send prompt
  	const char * prompt = "\nType your name:";
  	write( fd, prompt, strlen( prompt ) );
  
	// Currently character read
  	unsigned char newChar;

  	// Last character read
  	unsigned char lastChar = 0;

  	//
  	// The client should send <name><cr><lf>
  	// Read the name of the client character by character until a
  	// <CR><LF> is found.
  	//
    
  	
  	char request[ maxLength + 1 ];
	while ( length < maxLength && ( n = read( fd, &newChar, sizeof(newChar) ) ) > 0 ) {

    		if ( lastChar == '\015' && newChar == '\012' ) {
      			// Discard previous <CR> from name
      			length--;
      			break;
    		}

    		request[ length ] = newChar;
    		length++;
    		lastChar = newChar;
  	}
	sscanf(request, "%*s %s %*s", docpath);

  	// Add null character at the end of the string
  	docpath[ length ] = 0;

	//TEST that docpath is being cut out
	printf("Request:\n%s\n", request);
	printf("Document path: %s\n", docpath);

	//Map docpath to real file
	char cwd[256] = {0};
	getcwd(cwd, 256);
	
	if(!strcmp(docpath, "/"))
	{
		strcat(cwd, "/http-root-dir/htdocs/index.html");
		printf("cwd = %s\n", cwd);
	}
	//TODO other mappings
	//TODO expand filepath
	//TODO detemine content type
	//TODO open file
	//TODO send HTTP reply header
	//TODO add concurrency
	//TODO part 2
}
