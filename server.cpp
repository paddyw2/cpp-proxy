#include "server.h"

server::server(int argc, char * argv[])
{
    // check command line arguments
    if (argc < 2) {
        fprintf(stderr,"ERROR, no port provided\n");
        exit(1);
    }

    // create client socket and check for errors
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
       error("ERROR opening socket");

    // convert argument to port number
    // and check for errors
    portno = atoi(argv[1]);
    if(portno < 0)
       error("ERROR invalid port number");

    // clear structures and set to chosen values
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    // bind socket to chosen address and port
    if (bind(sockfd, (struct sockaddr *) &serv_addr,
             sizeof(serv_addr)) < 0)
             error("ERROR on binding");

    // start listening for connections on the
    // created socket
    listen(sockfd,5);
}

/*
 * 
 * Runs infinitely, waiting for and handling client
 * connections one at a time
 */
int server::start_server()
{
    /* TRIAL AREA */
    fd_set active_fd_set;
    fd_set read_fd_set;

    FD_ZERO (&active_fd_set);
    FD_SET (sockfd, &active_fd_set);


    /* END */

    // define variables
    char buffer[BUFFERSIZE];
    int error_flag;
    socklen_t clilen = sizeof(cli_addr);

    // enter infinite server loop
    while(1) {

        read_fd_set = active_fd_set;
        if(select(FD_SETSIZE, &read_fd_set, NULL, NULL, NULL) < 0)
            exit(EXIT_FAILURE);

        for(int i=0;i<FD_SETSIZE;i++) {
            if(FD_ISSET(i, &read_fd_set)) {
                if(i == sockfd) {
                    // our server socket is ready to be read
                    // which means a new client wants to conenct
                    cout << "Getting a client connection" << endl;
                    // accept new client connection
                    clientsockfd = accept(sockfd, (struct sockaddr *) &cli_addr,&clilen);

                    // error check
                    if (clientsockfd < 0)
                       error("ERROR on accept");

                    char welcome[] = "Welcome to the server, type 'help' for "
                                     "a list of commands\n";
                    write_to_client(welcome, strlen(welcome), clientsockfd);
                    FD_SET(clientsockfd, &active_fd_set);
                } else {
                    // one of our existing clients are sending the
                    // server data
                    bzero(buffer,BUFFERSIZE);
                    read_from_client(buffer,BUFFERSIZE-1, i);            
                    // notify server of successful message transfer
                    // and process the client request
                    cout << "Received client input: " << buffer << endl;

                    FD_CLR(i, &active_fd_set);
                }
            } else {
                // if no sockets ready to read, do nothing
            }
        }
    }


    // close client socket and loop
    // back to accept new connection
    close(clientsockfd);
    return 0;
}

/*
 * Writes to client socket and checks
 * for errors
 */
int server::write_to_client(char * message, int length, int client)
{
    int error_flag;
    error_flag = write(client, message, length); 
    // error check
    if (error_flag < 0)
        error("ERROR writing to socket");
    return 0;
}

/*
 * Reads from client socket and checks
 * for errors
 */
int server::read_from_client(char * message, int length, int client)
{
    int error_flag;
    error_flag = read(client, message, length); 
    strip_newline(message, length);
    // error check
    if (error_flag < 0)
        error("ERROR reading from socket");
    return 0;
}

/*
 * Strip new line and carriage return
 * from string
 */
int server::strip_newline(char * input, int max)
{
    if(input[strlen(input)-1] == '\n')
        input[strlen(input)-1] = 0;

    if(input[strlen(input)-1] == '\r')
        input[strlen(input)-1] = 0;

    return 0;
}

/*
 * Error handler
 */
void server::error(const char *msg) {
    perror(msg); exit(1);
}

/*
 * Closes the server
 */
int server::stop_server()
{
    cout << "Closing server socket" << endl;
    close(sockfd);
    exit(EXIT_SUCCESS);
}
