#include "server.h"

server::server(int argc, char * argv[])
{
    // check command line arguments
    if (argc < 4) {
        fprintf(stderr,"ERROR\nUsage: ./proxy [logOptions] [replaceOptions] srcPort server dstPort\n");
        exit(1);
    }

    // create client socket and check for errors
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
       error("ERROR opening socket");

    // determine if logging option, or replace
    // option are included
    int arg_offset = 0;
    int replace_set = 0;
    int logging_set = 0;
    if(argc == 5) {
        // logging option provided
        arg_offset = 1;
        logging_set = 1;
    } else if(argc > 5 && strncmp(argv[1], "-replace", sizeof("-replace")) == 0) {
        // if replace options are provided and no logging
        // options
        arg_offset = 3;
        replace_set = 1;
    } else if(argc == 8) {
        // both logging and replace options
        // provided
        arg_offset = 4;
        replace_set = 1;
        logging_set = 1;
    } else if(argc > 8) {
        error("Too many arguments provided\n"
              "Usage: ./proxy [logOptions] [replaceOptions] srcPort server dstPort\n");
    } else {
        // no replace or logging set, so
        // stay with defaults
    }

    process_replace_logging(replace_set, logging_set, argv);

    // convert argument to port number
    // and check for errors
    try{
        portno = stoi(argv[1+logging_set]);
        destport = stoi(argv[3+arg_offset]);
    } catch (const std::exception& ex) {
        error("Invalid port number\n");
    }
    cout << portno << endl;
    serverurl = argv[2+arg_offset];
    if(portno < 1024 || destport < 0)
       error("ERROR reserved port number");

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
    vector<proxyclient> client_proxies;
    struct timeval timeout;
    // set timeout to be 1 second
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    // set socket structs
    fd_set active_fd_set;
    fd_set read_fd_set;
    fd_set write_fd_set;

    FD_ZERO (&active_fd_set);
    FD_SET (sockfd, &active_fd_set);

    // define variables
    char buffer[BUFFERSIZE];
    int error_flag;
    socklen_t clilen = sizeof(cli_addr);

    // enter infinite server loop
    while(1) {
        // check current proxies for any response
        int remove_socket = check_proxy_responses(client_proxies, active_fd_set);
        // check for disconnected remote sockets
        if(remove_socket != -1) {
            // remove socket
            FD_CLR(remove_socket, &active_fd_set);
            remove_client(remove_socket, active_fd_set, client_proxies);
        }
        // set up file descriptor sets and run select
        // to look for readable sockets
        read_fd_set = active_fd_set;
        write_fd_set = active_fd_set;
        if(select(FD_SETSIZE, &read_fd_set, &write_fd_set, NULL, &timeout) < 0)
            error("Select error\n");

        // for each socket, check if it has information to
        // read
        for(int i=0;i<FD_SETSIZE;i++) {
            if(FD_ISSET(i, &read_fd_set)) {
                // if a socket has information to read, process
                // each socket
                if(i == sockfd) {
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
                    // create a proxy for the client and
                    // add to list
                    proxyclient new_proxy(destport, serverurl, clientsockfd, cli_addr, logging_option, replace_option);
                    new_proxy.print_connection_info();
                    client_proxies.push_back(new_proxy);
                } else {
                    // one of our existing clients are sending the
                    // server data
                    
                    // if the remote server has nothing to send
                    // back to us, we send our message to it
                    proxyclient proxy = get_proxy(i, client_proxies);
                    bzero(buffer,BUFFERSIZE);
                    int message_size = read_from_client(buffer,BUFFERSIZE-1, i);
                    //cout << "Received client input(no endl): " << buffer;
                    // if our local client message size is 0
                    // the client is disconnected
                    if(message_size == 0) {
                        // remove client
                        FD_CLR(i, &active_fd_set);
                        remove_client(i, active_fd_set, client_proxies);
                        cout << "Client connection closed" << endl;
                    } else {
                        // if message size is normal, send to proxy
                        proxy.send_message(buffer, message_size);
                    }
                }
            }
        }
    }
    return 0;
}

int server::check_proxy_responses(vector<proxyclient> proxies, fd_set active_fd_set)
{
    vector<proxyclient>::iterator itr;
    itr = proxies.begin();
    while(itr != proxies.end()) {
        proxyclient proxy = *itr;
        // check if a response is ready from server
        int i = proxy.get_socket_origin_id();
        // if the remote server is ready to send
        // back to us, continually get its response
        // until it doesn't have anything else to 
        // send
        char response[2048];
        int length = 1;
        while(proxy.check_response_ready() == 1) {
            bzero(response, 2048);
            length = proxy.receive_message(response, sizeof(response));
            if(length == 0) {
                // remote server disconnected, so disconnect
                // client
                cout << "Remote host closed" << endl;
                return i;
            } else {
                write_to_client(response, length, i);
            }
        }
        itr++;
    }
    return -1;
}


int server::remove_client(int client_socket, fd_set sockets, vector<proxyclient> & proxies)
{
    close(client_socket);
    proxyclient prxy = get_proxy(client_socket, proxies);
    int target_id = prxy.get_socket_origin_id();

    vector<proxyclient>::iterator itr;
    itr = proxies.begin();
    while(itr != proxies.end()) {
        prxy = *itr;
        if(target_id == prxy.get_socket_origin_id()) {
            prxy.destroy();
            proxies.erase(itr);
            return 0;
        } else {
            itr++;
        }
    }
    error("Proxy not found\n");
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
    //strip_newline((char *)message, length);
    // error check
    if (error_flag < 0)
        error("ERROR reading from socket");
    return error_flag;
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

int server::replace_tamper(char * message)
{
    return 0;
}

proxyclient server::get_proxy(int socket_id, vector<proxyclient> proxy_list)
{
    vector<proxyclient>::iterator itr;
    itr = proxy_list.begin();
    while(itr != proxy_list.end()) {
        if(socket_id == (*itr).get_socket_origin_id())
            return *itr;
        else
            itr++;
    }
    error("No proxy found for the specified client socket\n");
    proxyclient null_proxy;
    return null_proxy;
}

int server::process_replace_logging(int replace_set, int logging_set, char * arguments[])
{
    // if either are set, set appropriate
    // variables
    if(logging_set == 1 && replace_set == 1) {
        replace_option = 1;
        if(strncmp(arguments[1], "-raw", sizeof("-raw")) == 0) {
            cout << "Raw chosen!" << endl;
            logging_option = 1;
        }
    } else if(logging_set == 1 && replace_set == 0) {
        cout << "Logging only provided!" << endl;
        replace_option = 0;
        if(strncmp(arguments[1], "-raw", sizeof("-raw")) == 0) {
            cout << "Raw chosen!" << endl;
            logging_option = 1;
        } else if(strncmp(arguments[1], "-strip", sizeof("-strip")) == 0) {
            cout << "Strip chosen!" << endl;
            logging_option = 2;
        } else if(strncmp(arguments[1], "-hex", sizeof("-hex")) == 0) {
            cout << "Hex chosen!" << endl;
            logging_option = 3;
        } else if(strncmp(arguments[1], "-auto", 5) == 0) {
            cout << "AutoN chosen!" << endl;
            int end = strlen(arguments[1])-1;
            char val = arguments[1][end];
            unsigned int number = 0;
            unsigned int previous = number;
            unsigned int base = 1;
            while(val != 'o') {
                if(val > 47 && val < 58) {
                    number += ((val - 48)*base);
                } else {
                    error("Auto value must be an integer\n");
                }
                if(number < previous)
                    error("Exceeded max integer size\n");
                base *= 10;
                previous = number;
                end--;
                val = arguments[1][end];
            }
            logging_option = (int)number;
        } else {
            error("Invalid logging option: must be -raw, -strip, or -auto[N]\n");
        }
    } else if(replace_set == 1) {
        // just replace chosen
        cout << "Replace only provide!" << endl;
        replace_option = 1;
    } else {
        // neither logging or replace set
        cout << "No options set - logging off by default" << endl;
        logging_option = 0;
        replace_option = 0;
    }
    print_logging_status(logging_option);
    return 0;
}

int server::print_logging_status(int option)
{
    logging_option = option;
    if(option == 0) {
        cout << "Logging turned off" << endl;
    } else {
        cout << "Logging on" << endl;
    }
    return 0;
}

/*
 * Error handler
 */
void server::error(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
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
