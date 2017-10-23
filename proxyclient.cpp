#include "proxyclient.h"

proxyclient::proxyclient()
{
    error("Must provide port, url and id parameters\n");
}

proxyclient::proxyclient(int port, char * url, int sock_id)
{
    // set origin sock id
    socket_origin_id = sock_id;
    // logging off by default
    log_flag = 0;
    char * dest_url = url;
    dest_port = port;

    char target_ip[32];
    convert_hostname_ip(target_ip, sizeof(target_ip), dest_url);
    print_connection_info();

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

int proxyclient::print_connection_info()
{
    cout << "New connection: ";
    time_t rawtime;
    struct tm * timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    char * result = asctime(timeinfo);
    result[strlen(result)-1] = 0;
    cout << result;
    cout << ", from [source]" << endl;
    return 0;
}

int proxyclient::send_message(char * message, int length)
{
    outgoing_log(message);
    write_to_client(message, length);
    return check_response_ready();
}

int proxyclient::receive_message(char * message, int length)
{
    //char http_response[2048];
    bzero(message, length);
    int response_size = read_from_client(message, length);
    incoming_log(message);
    return response_size;
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
    bzero(message, length);
    error_flag = read(proxy_socket, message, length); 
    // error check
    if (error_flag < 0)
        error("ERROR reading from socket");
    return error_flag;
}

int proxyclient::incoming_log(char * message)
{
    if(log_flag == 1)
        cout << "<---- " << message << endl;
    return 0;
}

int proxyclient::outgoing_log(char * message)
{
    if(log_flag == 1)
        cout << "----> " << message << endl;
    return 0;
}

int proxyclient::check_response_ready()
{
    struct timeval timeout;
    // set timeout to be 1 second
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    fd_set active_fd_set;
    fd_set read_fd_set;
    fd_set write_fd_set;

    FD_ZERO (&active_fd_set);
    FD_SET (proxy_socket, &active_fd_set);

    read_fd_set = active_fd_set;
    write_fd_set = active_fd_set;
    // timeout happens when receiving an incremental
    // when the destination server is not ready to
    // return, and as we are only checking one socket
    // the select() function would block with the
    // timeout
    if(select(FD_SETSIZE, &read_fd_set, &write_fd_set, NULL, &timeout) < 0)
        error("Check select error\n");

    if(FD_ISSET(proxy_socket, &read_fd_set)) {
        // host is ready to respond, so
        // return 1
        return 1;
    } 
    return 0;
}

/*
 * Used for searching for the right
 * proxy for a certain client
 */
int proxyclient::get_socket_origin_id()
{
    return socket_origin_id;
}

int proxyclient::destroy()
{
    close(proxy_socket);
    return 0;
}
    

/*
 * Error handler
 */
void proxyclient::error(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}
