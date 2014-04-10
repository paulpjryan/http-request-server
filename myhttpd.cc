
const char * usage =
"                                                               \n"
"myhttpd:                                                	\n"
"                                                               \n"
"Simple server program that shows how to use socket calls       \n"
"in the server side.                                            \n"
"                                                               \n"
"To use it in one window type:                                  \n"
"                                                               \n"
"   ./myhttpd <concurrency mode> <port>                         \n"
"                                                               \n"
"Where 1024 < port < 65536.             			\n"
"Concurrency mode can be -f for forking on each request, 	\n"
"	-t for a thread for each request, 			\n"
"	or -p for a pool of threads.				\n"
"                                                               \n"
"In another window type:                                       	\n"
"                                                               \n"
"   telnet <host> <port>                                        \n"
"                                                               \n"
"where <host> is the name of the machine where daytime-server  	\n"
"is running. <port> is the port number you used when you run   	\n"
"myhttpd.                                          		\n"
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
#include <pthread.h>

int QueueLength = 5;

// Processes time request
void processRequest( int socket );
void processRequestThread( int fd );
void poolSlave( int masterSocket );

pthread_mutex_t mutex;


const char * crlf = "\r\n";
const char * space = " ";

const char * protocol = "HTTP/1.1";
const char * serverType = "CS252lab5";

int main( int argc, char ** argv )
{
	int concurrency;
	// Print usage if not enough arguments
	if ( argc < 3 ) {
    		fprintf( stderr, "%s", usage );
    		exit( -1 );
  	}

	//Set concurrency mode
	if(!strcmp(argv[1], "-f"))
	{
		concurrency = 1;
	}
	else if(!strcmp(argv[1], "-t"))
	{
		concurrency = 2;
	}
	else if(!strcmp(argv[1], "-p"))
	{
		concurrency = 3;
	}
	else
	{
		concurrency = -1;
	}
	//printf("Concurrency mode: %d\n", concurrency);
 
  	// Get the port from the arguments
  	int port = atoi( argv[2] );
  
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

	//POOL OF THREADS
	if(concurrency == 3)
	{
		
		if(pthread_mutex_init(&mutex, NULL) != 0)
		{
			perror("Mutex");
		}
		pthread_t tid[5];
		pthread_attr_t attr;

		pthread_attr_init( &attr );
		pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);

		int i;
		for(i = 0; i < 5; i++)
		{
			pthread_create(tid + i*sizeof(tid[0]), &attr, (void*(*)(void*))poolSlave,(void*)masterSocket);  
		}
		pthread_join(tid[0], NULL); 
	}
	else
	{
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
		
			if(concurrency == -1)
			{		
				//sequential
		    		processRequest( slaveSocket );
	    			close( slaveSocket );
			}
			else if(concurrency == 1)
			{
				//process based
				pid_t slave = fork();
				if(slave == 0)
				{
		    			processRequest( slaveSocket );
	    				close( slaveSocket );
					exit(EXIT_SUCCESS);
				}
	    			
				close( slaveSocket );	
			}
			else if(concurrency == 2)
			{
				//thread based
				pthread_t t1;
				pthread_attr_t attr;

				pthread_attr_init( &attr );
				pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);

				pthread_create( &t1, &attr, (void * (*)(void *)) processRequestThread, (void *)slaveSocket );
			}
	    		// Close socket
	  	}
	}
  
}

void processRequestThread( int fd )
{
	//printf("Thread created %ld\n", pthread_self());
	processRequest(fd);
	close(fd);
}


void poolSlave( int masterSocket )
{
	//printf("Thread %ld entering main loop\n", pthread_self());
	while(1)
	{
		struct sockaddr_in clientIPAddress;
    		int alen = sizeof( clientIPAddress );
		pthread_mutex_lock(&mutex);
    		int slaveSocket = accept( masterSocket, (struct sockaddr *)&clientIPAddress, (socklen_t*)&alen);
		pthread_mutex_unlock(&mutex);
		//printf("Thread %ld accepted!\n", pthread_self());
    		if ( slaveSocket < 0 ) {
      			perror( "accept" );
      			exit( -1 );
    		}

	
    		processRequest( slaveSocket );
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
	while (( n = read( fd, &newChar, sizeof(newChar) ) ) > 0 ) {

    		if ( lastChar == '\r' && newChar == '\n' ) {
      			// Discard previous <CR> from name
      			length--;
      			break;
    		}

    		request[ length ] = newChar;
    		length++;
    		lastChar = newChar;
  	}
	//printf("request: %s\n", request);
	lastChar = 0;
	int consec = 2;
	while (( n = read( fd, &newChar, sizeof(newChar) )) > 0 )
	{	
		if (consec % 2 == 0 && newChar == '\r')
		{
			consec++;
		}
		else if (consec % 2 == 1 && newChar == '\n')
		{
			consec++;
		}
		else
		{
			consec = 0;
		}		
		if (consec == 4)
		{
			break;
		}    		
		lastChar = newChar;
	}
	//printf("AFTER WHILE LOOP\n");
	//sscanf(request, "GET %s %*s", docpath);
	char * tmp = request + 4;
	//printf("Temp = %s\n", tmp);
	int ln = 0;
	while(*tmp)
	{
		//printf("Checking character %c\n", *tmp);
		if(*tmp == ' ')
		{
			break;
		}
		else
		{
			docpath[ln] = *tmp;
			tmp++;
			ln++;
		}
	}

  	// Add null character at the end of the string
  	docpath[ ln++ ] = 0;

	//TEST that docpath is being cut out
	//printf("=======================================\n");
	//printf("Request\n");
	//printf("=======================================\n");
	//printf("Document path: %s\n", docpath);
	//printf("Request: %s\n", request);

	//Map docpath to real file
	char cwd[256] = {0};
	getcwd(cwd, 256);
	char root[256];
	strcpy(root, cwd);
	strcat(root, "/http-root-dir");
	//printf("cwd before expansion = %s\n", cwd);
	if(strcmp(docpath, "/") == 0)
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
		//printf("Docpath begins with: %s\n", begins);
		//printf("Docpath = %s\n", docpath);
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
	//printf("cwd = %s\n", cwd);

	//Detemine content type
	char * ends;
	char contentType[48];
	ends = strchr(cwd, '.');
	if(ends != NULL)
	{
		//printf("Docpath ends with: %s\n", ends);
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
		//printf("Content Type: %s\n", contentType);
	}
	else
	{
		strcpy(contentType, "text/plain");
	}

	//printf("CWD Length: %ld\nRoot Length: %ld\n", strlen(cwd) , strlen(root));
	//printf("CWD : %s\n", cwd);
	//printf("ROOT: %s\n", root);
	if(strlen(cwd) < strlen(root))
	{
		//printf("Sending 404, below root\n");
		const char * notFound = "File not Found";
		write(fd, protocol, strlen(protocol));
		write(fd, space, 1);
		write(fd, "404", 3);
		write(fd, space, 1);
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
		return;
	}

	//Open file
	
	int file;
	file = open(cwd, O_RDONLY, 0600);
	//printf("File descriptor: %d\n", file);
	//Send 404 if file isn't found
	//fd = 1;
	if(file < 0)
	{ 
		//printf("Sending 404, file open error\n");
		const char * notFound = "File not Found";
		write(fd, protocol, strlen(protocol));
		write(fd, space, 1);
		write(fd, "404", 3);
		write(fd, space, 1);
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
	
	//Send HTTP reply header
	else
	{
		write(fd, protocol, strlen(protocol));
		write(fd, space, 1);
		write(fd, "200", 3);
		write(fd, space, 1);
		write(fd, "Document", 8);
		write(fd, space, 1);
		write(fd, "follows", 7);
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
		
		//Write file to fd
		char buffer[1024];
		unsigned int cnt;
		while((cnt = read(file, buffer, 1024)))
		{
			write(fd, buffer, cnt);
		}
		
	}
	//TODO part 2
	
    	/*Fork child process
	Set the environment variable REQUEST_METHOD=GET
	Set the environment variable QUERY_STRING=(arguments after ?)
	Redirect output of child process to slave socket.
	Print the following header:

	HTTP/1.1 200 Document follows crlf 
	Server: Server-Type crlf

	Execute script */
}
