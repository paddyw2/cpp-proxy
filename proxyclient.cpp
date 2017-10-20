#include "proxyclient.h"

proxyclient::proxyclient()
{
    // basic telnet GET info
    const char * request = "GET / HTTP/1.0\r\nConnection: close\r\n\r\n";
    char dest_url[] = "rainmaker.wunderground.com";
    dest_port = 23;

    char target_ip[32];
    convert_hostname_ip(target_ip, sizeof(target_ip), dest_url);

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

    char http_response[2048];
    write_to_client((char *)request, sizeof(request));
    read_from_client((char *)http_response, sizeof(http_response));
    cout << http_response << endl;

}

/*
 * Takes a string hostname, such as "www.google.com"
 * and converts it to a string IP, such as "192.168.0.1"
 * Helped by:
 * http://www.binarytides.com/hostname-to-ip-address-c-sockets-linux/
 */
int proxyclient::convert_hostname_ip(char * target_ip, int target_size, char * dest_url)
{
    // resulting ip information
    struct addrinfo * ip_info;
    // struct containing ip information
    struct addrinfo format_hints;
    struct sockaddr_in * temp_ip;
    bzero(target_ip, target_size);

    bzero(&format_hints, sizeof(format_hints));
    format_hints.ai_family = AF_INET; 
    format_hints.ai_socktype = SOCK_STREAM;

    error_flag = getaddrinfo(dest_url, "telnet", &format_hints, &ip_info);
    if(error_flag < 0 || ip_info == NULL)
        error("IP conversion failed\n");

    // loop through all the results and connect to the first we can
    for(struct addrinfo * p = ip_info; p != NULL; p = p->ai_next) 
    {
        temp_ip = (struct sockaddr_in *) p->ai_addr;
        strncpy(target_ip, inet_ntoa( temp_ip->sin_addr ), target_size);
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
    message[error_flag] = 0;
    return 0;
}

/*
 * Error handler
 */
void proxyclient::error(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}
