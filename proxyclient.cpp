#include "proxyclient.h"

proxyclient::proxyclient()
{
    error("Must provide port, url and id parameters\n");
}

proxyclient::proxyclient(int port, char * url, int sock_id, struct sockaddr_in cli_addr)
{
    // set default options
    log_flag = 0;
    replace_flag = 0;
    replace_not_set = 1;
    // get remote connection info
    char * dest_url = url;
    dest_port = port;

    // set origin sock id
    socket_origin_id = sock_id;

    // resolve origin hostname
    resolve_origin_hostname(cli_addr);

    // get remote connection IP from hostname
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
}

int proxyclient::set_log_replace(int logging_option, int replace_option)
{
    // set logging and replace options
    log_flag = logging_option;
    replace_flag = replace_option;
    return 0;
}

int proxyclient::set_replace_strings(char * replace_old, char * replace_new)
{
    // copy replace strings
    memcpy(replace_str_old, replace_old, sizeof(replace_str_old));
    memcpy(replace_str_new, replace_new, sizeof(replace_str_new));
    replace_not_set = 0;
    return 0;
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
    cout << ", from ";
    cout << origin_hostname;
    cout << endl;
    return 0;
}

int proxyclient::send_message(char * orig_message, int length)
{
    char * message = (char *)malloc(length);
    memcpy(message, orig_message, length);
    int new_size = find_replace(&message, length);
    print_log(message, 1, new_size);
    write_to_client(message, new_size);
    return check_response_ready();
}

int proxyclient::receive_message(char ** message, int length)
{
    bzero(*message, length);
    int response_size = read_from_client(*message, length);
    char * new_message = (char *)malloc(response_size);
    memcpy(new_message, *message, response_size);
    int new_size = find_replace(&new_message, response_size);
    print_log(new_message, 0, new_size);
    *message = new_message;
    return new_size;
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

int proxyclient::resolve_origin_hostname(struct sockaddr_in cli_addr)
{
    // get origin IP address
    socklen_t clilen = sizeof(cli_addr);
    strncpy(origin_ip, inet_ntoa(cli_addr.sin_addr), sizeof(origin_ip));

    // attempt to origin hostname
    // if fails, then set to IP address
    struct sockaddr *sa = (struct sockaddr *)&cli_addr;
    socklen_t len;
    char hbuf[NI_MAXHOST];
    if (getnameinfo(sa, len, hbuf, sizeof(hbuf), NULL, 0, NI_NAMEREQD)) {
        strncpy(origin_hostname, origin_ip, sizeof(origin_hostname));
    } else {
        strncpy(origin_hostname, hbuf, sizeof(origin_hostname));
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

int proxyclient::print_log(char * orig_message, int direct, int size)
{
    char message[size];
    memcpy(message, orig_message, size);
    char direction[8];
    if(direct == 0)
        strncpy(direction, "<----", sizeof("<----"));
    else
        strncpy(direction, "---->", sizeof("---->"));

    if(log_flag == 1) {
        // raw option, just print normally
        // until getting a \n then add padding
        printf("%s ", direction);
        for(int i=0;i<size;i++) {
            if(message[i] == '\n') {
                printf("\n      ");
                continue;
            }
            printf("%c", message[i]);
        }
        printf("\n");
    } else if(log_flag == 2 || log_flag == 3) {
        // either strip or hex chosen
        log_strip_hex(direction, message, size);
    } else if(log_flag >= 4) {
        // autoN chosen
        log_auton(direction, message, size);
    } else {
        // logging not set
        // do nothing
    }
    return 0;
}

int proxyclient::log_strip_hex(char * direction, char * message, int size)
{
    // save original for hex
    char original[size];
    memcpy(original, message, size);
    // change all non-ascii letters to .
    for(int i=0;i<size;i++) {
        if((int)message[i] < 32 || (int)message[i] > 126)
            message[i] = '.';
    }
    if(log_flag == 2) {
        // strip option, so just print
        // stripped string
        for(int i=0;i<size;i++) {
            printf("%c", message[i]);
        }
        printf("\n");
    } else {
        // hex option
        // print chars in line, then hex chars x16
        // then ascii attempt
        int total_counter = 0;
        printf("%s", direction);
        printf(" %08x  ", total_counter);
        int counter = 0;
        for(int i=0;i<size;i++) {
            if(counter == 16) {
                printf(" |");
                while(counter != 0) {
                    printf("%c", message[i-counter]);
                    counter--;
                }
                printf("|\n");
                printf("      %08x  ", total_counter);
            }
            if(counter == 8)
                printf(" ");
            printf("%02X ", (unsigned char)original[i]);
            counter++;
            total_counter++;
        }
        int remainder = 16 - counter;
        if(counter <= 8)
            printf(" ");
        for(int i=0;i<remainder;i++) {
            printf("   ");
        }
        printf(" |");
        while(counter != 0) {
            printf("%c", message[size-counter]);
            counter--;
        }
        printf("|\n");
    }
    return 0;
}

int proxyclient::log_auton(char * direction, char * message, int size)
{
    cout << direction << " ";
    int n_val = log_flag;
    int counter = 0;
    for(int i=0;i<size;i++) {
        if(counter == n_val) {
            printf("\n      ");
            counter = 0;
        }
        char next_char = message[i];
        if(next_char == '\r') {
            printf("\\r");
        } else if(next_char == '\n') {
            printf("\\n");
        } else if(next_char == '\t') {
            printf("\\t");
        } else if(next_char == '\\') {
            printf("\\");
        } else if(next_char > 31 && next_char < 127) {
            printf("%c", next_char);
        } else {
            printf("/%02X", (unsigned char)next_char);
        }

        counter++;
    }
    printf("\n");
    return 0;
}

int proxyclient::find_replace(char ** message, int length)
{
    // check if replace activated
    if(replace_flag == 0)
        return length;
    // check that replacement params provided
    if(replace_not_set == 1)
        error("Replace specified but strings not provided\n");

    // if activate, proceed with replace
    char * new_message = (char *)malloc(length);
    memcpy(new_message, *message, length);
    int current_size = length;
    int max_size = length;
    int inserted_size = 0;

    int length_diff = strlen(replace_str_new) - strlen(replace_str_old);
    int master_found = 0;

    for(int i=0; i<length; i++) {
        int found = 1;
        for(int j=0; j<strlen(replace_str_old); j++) {
            if(new_message[i+j] == replace_str_old[j]) {
                continue;
            } else {
                found = 0;
                break;
            }
        }
        // if a match is found at i
        if(found == 1) {
            // replace with new string
            master_found = 1;
            // replace is larger than original
            if(current_size + strlen(replace_str_new) >= max_size) {
                // increase new_message size, ifsource is maxed out
                new_message = (char *)realloc(new_message, max_size*2);
                // increment size of allocation
                max_size = max_size*2;
            }
            // bump everything after original up by difference
            char * tmp = (char *)calloc(current_size, sizeof(char));
            memcpy(tmp, new_message, current_size);
            memcpy(new_message+i+strlen(replace_str_old)+length_diff,
                   tmp+i+strlen(replace_str_old), current_size-i-strlen(replace_str_old));
            free(tmp);
            // now copy replacement string into place
            memcpy(new_message+i, replace_str_new, strlen(replace_str_new));
            // increment current size
            current_size += length_diff;
        } else {
            // string not found at this
            // index, so do nothing
        }
    }

    if(master_found == 1) {
        // update message with new pointer after realloc
        *message = (char *)realloc(*message, current_size);
        memcpy(*message, new_message, current_size);
    }

    return current_size;
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
