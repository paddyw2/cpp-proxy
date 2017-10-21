#include "proxyclient.h"

proxyclient::proxyclient(int port, char * url)
{
    // log by default
    log_flag = 1;
    // basic telnet GET info
    //const char * request = (const char *) message;
        //"GET / HTTP/1.0\r\nConnection: close\r\n\r\n";
    char * dest_url = url;
        //"linux.cpsc.ucalgary.ca";
    //char dest_url[] = "rainmaker.wunderground.com";
    dest_port = port;

    char target_ip[32];
    convert_hostname_ip(target_ip, sizeof(target_ip), dest_url);
    cout << target_ip << endl;

    // set port and IP
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(dest_port);
    inet_aton(target_ip, &dest_addr.sin_addr);

    // create socket and connect to server
    proxy_socket = socket(AF_INET, SOCK_STREAM, 0);

    if(proxy_socket < 0)
        error("Proxy socket creation error\n");

    error_flag = connect(proxy_socket, (struct sockaddr *)&dest_addr, sizeof(dest_addr));

    if(error_flag < 0)
        error("Proxy remote connection error\n");
}

int proxyclient::send_message(char * message, int length)
{
    log(message);
    char newmessage[] = "GET / HTTP/1.1\r\nHost: www.neverssl.com\r\n"; //Connection: close\r\n\r\n";
    write_to_client(newmessage, strlen(newmessage));
    char newmessage2[] = "Connection: close\r\n\r\n";
    write_to_client(newmessage2, strlen(newmessage2));
    //write_to_client(message, strlen(newmessage));
    return check_response_ready();
}

int proxyclient::receive_message(char * message, int length)
{
    //char http_response[2048];
    bzero(message, length);
    read_from_client(message, length);
    log(message);
    cout << message << endl;
    return 0;
}


/*
 * Takes a string hostname, such as "www.google.com"
 * and converts it to a string IP, such as "192.168.0.1"
 * Helped by:
 * http://www.binarytides.com/hostname-to-ip-address-c-sockets-linux/
 */
int proxyclient::convert_hostname_ip(char * target_ip, int target_size, char * dest_url)
{
    // list of structs returned
    // by the getaddrinfo
    struct addrinfo * ip_info;

    // clear the target_ip string
    bzero(target_ip, target_size);

    // resolve hostname
    // NULL means no port initialized
    error_flag = getaddrinfo(dest_url, NULL, NULL, &ip_info);

    // check for errors
    if(error_flag < 0 || ip_info == NULL)
        error("IP conversion failed\n");

    // loop through all the ip_info structs
    // and get the IP of the first one
    for(struct addrinfo * p = ip_info; p != NULL; p = p->ai_next) 
    {
        // copy the socket IP address, converted to
        // readable format, to the target_ip string
        strncpy(target_ip, inet_ntoa(((struct sockaddr_in *)p->ai_addr)->sin_addr ), target_size);

        // check that a valid address was extracted
        // if so, break as we have an IP
        if(strlen(target_ip) > 0)
            break;
    }
    return 0;
}

/*
 * Writes to client socket and checks
 * for errors
 */
int proxyclient::write_to_client(char * message, int length)
{
    cout << "Writing..." << endl;
    error_flag = write(proxy_socket, message, length); 
    // error check
    if (error_flag < 0)
        error("ERROR writing to socket");
    return 0;
}

/*
 * Reads from client socket and checks
 * for errors
 */
int proxyclient::read_from_client(char * message, int length)
{
    cout << "Reading..." << endl;
    bzero(message, length);
    error_flag = read(proxy_socket, message, length); 
    // error check
    if (error_flag < 0)
        error("ERROR reading from socket");
    cout << error_flag << endl;
    //message[error_flag] = 0;
    return 0;
}

int proxyclient::log(char * message)
{
    return 0;
}

int proxyclient::check_response_ready()
{
    fd_set active_fd_set;
    fd_set read_fd_set;

    FD_ZERO (&active_fd_set);
    FD_SET (proxy_socket, &active_fd_set);

    read_fd_set = active_fd_set;
    if(select(FD_SETSIZE, &read_fd_set, NULL, NULL, NULL) < 0)
        exit(EXIT_FAILURE);


    for(int i=0;i<FD_SETSIZE;i++) {
        if(FD_ISSET(i, &read_fd_set)) {
            if(i == proxy_socket) {
                // host is ready to respond, so
                // return 1
                return 1;
            }
        }
    }
    return 0;
}


/*
 * Error handler
 */
void proxyclient::error(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}
