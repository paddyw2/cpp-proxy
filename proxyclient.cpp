#include "proxyclient.h"

proxyclient::proxyclient()
{
    const char * request = "GET / HTTP/1.0\r\nHost: neverssl.com\r\nConnection: close\r\n\r\n";

    char dest_url[] = "linux.cpsc.ucalgary.ca";
    char dest_ip[100];
    dest_port = PORT;
    // resulting ip information
    struct addrinfo * ip_info;
    // struct containing ip information
    struct addrinfo format_hints;
    struct sockaddr_in * dest_addr;

    bzero(&format_hints, sizeof(format_hints));
    format_hints.ai_family = AF_INET; 
    format_hints.ai_socktype = SOCK_STREAM;

    error_flag = getaddrinfo(dest_url, "http", NULL, &ip_info);
    if(error_flag < 0)
        error("IP conversion failed\n");

    if(ip_info == NULL)
        cout << "Fail" << endl;

    // loop through all the results and connect to the first we can
    for(struct addrinfo * p = ip_info; p != NULL; p = p->ai_next) 
    {
        dest_addr = (struct sockaddr_in *) p->ai_addr;
        cout << inet_ntoa( dest_addr->sin_addr ) << endl;
    }


    // set port and IP
    dest_addr->sin_family = AF_INET;
    dest_addr->sin_port = htons(dest_port);

    // create socket and connect to server
    proxy_socket = socket(AF_INET, SOCK_STREAM, 0);

    if(proxy_socket < 0)
        error("Proxy socket creation error\n");

    error_flag = connect(proxy_socket, (struct sockaddr *)&dest_addr, sizeof(dest_addr));

    if(error_flag < 0)
        error("Proxy remote connection error\n");

}

/*
 * Writes to client socket and checks
 * for errors
 */
int proxyclient::write_to_client(char * message, int length)
{
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
    error_flag = read(proxy_socket, message, length); 
    // error check
    if (error_flag < 0)
        error("ERROR reading from socket");
    return 0;
}

/*
 * Error handler
 */
void proxyclient::error(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}
