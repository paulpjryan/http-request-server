
const char * usage =
"                                                               \n"
"myhttpd:                                                	\n"
"                                                               \n"
"Simple server program that shows how to use socket calls       \n"
"in the server side.                                            \n"
"                                                               \n"
"To use it in one window type:                                  \n"
"                                                               \n"
"   daytime-server <port>                                       \n"
"                                                               \n"
"Where 1024 < port < 65536.             			\n"
"                                                               \n"
"In another window type:                                       	\n"
"                                                               \n"
"   telnet <host> <port>                                        \n"
"                                                               \n"
"where <host> is the name of the machine where daytime-server  	\n"
"is running. <port> is the port number you used when you run   	\n"
"daytime-server.                                          	\n"
"                                                               \n";


#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>

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
	sscanf(request, "GET %s %*s", docpath);

  	// Add null character at the end of the string
  	docpath[ length ] = 0;

	//TEST that docpath is being cut out
	printf("=======================================\n");
	printf("Request\n");
	printf("=======================================\n");
	printf("Document path: %s\n", docpath);

	//Map docpath to real file
	char cwd[256] = {0};
	getcwd(cwd, 256);
	
	if(!strcmp(docpath, "/"))
	{
		strcat(cwd, "/http-root-dir/htdocs/index.html");
	} 
	else
	{
		char begins[48];
		int i = 0;
		//sscanf(docpath, "%s/%*s", begins);
		char * tmp = docpath;
		tmp++;
		while(*tmp)
		{
			if(*tmp == '/')
			{
				break;
			}
			else
			{
				begins[i++] = *tmp;
			}
			tmp++;
			
		}
		begins[i] = 0;
		printf("Docpath begins with: %s\n", begins);
		if(!strcmp(begins, "icons") || !strcmp(begins, "htdocs"))
		{
			printf("In if\n");
			strcat(cwd, "/http-root-dir");
			strcat(cwd, docpath);
		}

		else
		{
			strcat(cwd, "/http-root-dir/htdocs");
			strcat(cwd, docpath);
		}
	}
	printf("cwd = %s\n", cwd);
	//TODO expand filepath

	//Detemine content type
	char * ends;
	char contentType[48];
	ends = strchr(cwd, '.');
	printf("Docpath ends with: %s\n", ends);
	if(!strcmp(ends, ".html") || !strcmp(ends, ".html/"))
	{
		strcpy(contentType, "text/html");
	}
	else if(!strcmp(ends, ".gif") || !strcmp(ends, ".gif/"))
	{
		strcpy(contentType, "image/gif");
	}
	else
	{
		strcpy(contentType, "text/plain");
	}
	printf("Content Type: %s\n", contentType);

	//Open file
	const char * crlf = "\r\n";
	const char * space = " ";
	
	const char * protocol = "HTTP/1.1";
	const char * serverType = "CS 252 lab5";
	
	int file;
	file = open(cwd, O_RDONLY, 0600);
	printf("File descriptor: %d\n", file);
	//Send 404 if file isn't found
	if(file < 0)
	{ 
		printf("File not found\n");
		const char * notFound = "File not Found";
		write(fd, protocol, strlen(protocol));
		write(fd, space, 1);
		write(fd, "404", 3);
		write(fd, "File", 4);
		write(fd, "Not", 3);
		write(fd, "Found", 5);
		write(fd, crlf, 2);
		write(fd, "Server:", 7);
		write(fd, space, 1);
		write(fd, serverType, strlen(serverType));
		write(fd, crlf, 2);
		write(fd, "Content-type:", 13);
		write(fd, space, 1);
		write(fd, contentType, strlen(contentType));
		write(fd, crlf, 2);
		write(fd, crlf, 2);
		write(fd, notFound, strlen(notFound));
	}
	
	//TODO send HTTP reply header
	//TODO add concurrency
	//TODO part 2
	
 	const char * newline = "\n";
  	write(fd, newline, strlen(newline));
}
