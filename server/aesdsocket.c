#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <syslog.h>
#include <signal.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/wait.h>

#define PORT 9000
#define DATA_FILE "/var/tmp/aesdsocketdata"
#define BUFFER_SIZE 1024

static volatile sig_atomic_t shutdown_flag = 0;

void signal_handler(int sig) {
    if (sig == SIGINT || sig == SIGTERM) {
        syslog(LOG_INFO, "Caught signal, exiting");
        shutdown_flag = 1;
    }
}

void cleanup_and_exit(int server_fd) {
    close(server_fd);
    unlink(DATA_FILE);
    closelog();
    exit(EXIT_SUCCESS);
}

void daemonize() {
    pid_t pid;
    
    // Fork off the parent process
    pid = fork();
    if (pid < 0) {
        syslog(LOG_ERR, "Failed to fork: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }
    
    // If we got a good PID, then we can exit the parent process
    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }
    
    // Change the file mode mask
    umask(0);
    
    // Open any logs here
    // (already opened syslog in main)
    
    // Create a new SID for the child process
    if (setsid() < 0) {
        syslog(LOG_ERR, "Failed to create new session: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }
    
    // Change the current working directory
    if ((chdir("/")) < 0) {
        syslog(LOG_ERR, "Failed to change directory: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }
    
    // Close out the standard file descriptors
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
    
    // Redirect standard file descriptors to /dev/null
    open("/dev/null", O_RDONLY);
    open("/dev/null", O_WRONLY);
    open("/dev/null", O_WRONLY);
    
    syslog(LOG_INFO, "Daemon started successfully");
}

int main(int argc, char *argv[]) {
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;
    int data_fd;
    char client_ip[INET_ADDRSTRLEN];
    int daemon_mode = 0;
    
    // Parse command line arguments
    if (argc > 1) {
        if (strcmp(argv[1], "-d") == 0) {
            daemon_mode = 1;
        } else {
            printf("Usage: %s [-d]\n", argv[0]);
            printf("  -d    Run as daemon\n");
            exit(EXIT_FAILURE);
        }
    }
    
    // Open syslog
    openlog("aesdsocket", LOG_PID, LOG_USER);
    
    // Set up signal handlers
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // Create socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        syslog(LOG_ERR, "Failed to create socket: %s", strerror(errno));
        return -1;
    }
    
    // Allow socket reuse
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        syslog(LOG_ERR, "Failed to set socket options: %s", strerror(errno));
        close(server_fd);
        return -1;
    }
    
    // Make socket non-blocking
    int flags = fcntl(server_fd, F_GETFL, 0);
    if (flags < 0) {
        syslog(LOG_ERR, "Failed to get socket flags: %s", strerror(errno));
        close(server_fd);
        return -1;
    }
    if (fcntl(server_fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        syslog(LOG_ERR, "Failed to set socket non-blocking: %s", strerror(errno));
        close(server_fd);
        return -1;
    }
    
    // Set up server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);
    
    // Bind socket
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        syslog(LOG_ERR, "Failed to bind socket: %s", strerror(errno));
        close(server_fd);
        return -1;
    }
    
    // Listen for connections
    if (listen(server_fd, 5) < 0) {
        syslog(LOG_ERR, "Failed to listen on socket: %s", strerror(errno));
        close(server_fd);
        return -1;
    }
    
    syslog(LOG_INFO, "Server listening on port %d", PORT);
    
    // Fork to daemon if requested (after binding to port)
    if (daemon_mode) {
        daemonize();
    }
    
    // Main server loop
    while (!shutdown_flag) {
        // Use poll to wait for connections with timeout
        struct pollfd pfd;
        pfd.fd = server_fd;
        pfd.events = POLLIN;
        
        int poll_result = poll(&pfd, 1, 1000); // 1 second timeout
        if (poll_result < 0) {
            if (errno == EINTR) {
                // Poll was interrupted by signal, check shutdown_flag
                continue;
            }
            syslog(LOG_ERR, "Poll failed: %s", strerror(errno));
            break;
        }
        
        if (poll_result == 0) {
            // Timeout, check shutdown_flag and continue
            continue;
        }
        
        // Accept connection
        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // No connection available, continue
                continue;
            }
            if (shutdown_flag) {
                break; // Exit if we received a signal
            }
            syslog(LOG_ERR, "Failed to accept connection: %s", strerror(errno));
            continue;
        }
        
        // Get client IP address
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
        syslog(LOG_INFO, "Accepted connection from %s", client_ip);
        
        // Make client socket non-blocking
        int client_flags = fcntl(client_fd, F_GETFL, 0);
        if (client_flags >= 0) {
            fcntl(client_fd, F_SETFL, client_flags | O_NONBLOCK);
        }
        
        // Open data file for appending
        data_fd = open(DATA_FILE, O_CREAT | O_WRONLY | O_APPEND, 0644);
        if (data_fd < 0) {
            syslog(LOG_ERR, "Failed to open data file: %s", strerror(errno));
            close(client_fd);
            continue;
        }
        
        // Receive data and process
        char *packet_buffer = NULL;
        size_t packet_size = 0;
        
        while (!shutdown_flag) {
            // Use poll to wait for data on client socket
            struct pollfd client_pfd;
            client_pfd.fd = client_fd;
            client_pfd.events = POLLIN;
            
            int client_poll_result = poll(&client_pfd, 1, 1000); // 1 second timeout
            if (client_poll_result < 0) {
                if (errno == EINTR) {
                    // Poll was interrupted by signal, check shutdown_flag
                    break;
                }
                syslog(LOG_ERR, "Client poll failed: %s", strerror(errno));
                break;
            }
            
            if (client_poll_result == 0) {
                // Timeout, check shutdown_flag and continue
                continue;
            }
            
            bytes_read = recv(client_fd, buffer, BUFFER_SIZE, 0);
            if (bytes_read <= 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    // No data available, continue
                    continue;
                }
                // Connection closed or error
                break;
            }
            
            // Reallocate packet buffer to accommodate new data
            char *new_buffer = realloc(packet_buffer, packet_size + bytes_read + 1);
            if (!new_buffer) {
                syslog(LOG_ERR, "Failed to allocate memory for packet");
                free(packet_buffer);
                close(data_fd);
                close(client_fd);
                cleanup_and_exit(server_fd);
            }
            packet_buffer = new_buffer;
            
            // Copy received data to packet buffer
            memcpy(packet_buffer + packet_size, buffer, bytes_read);
            packet_size += bytes_read;
            packet_buffer[packet_size] = '\0';
            
            // Check for complete packet (newline character)
            char *newline_pos = strchr(packet_buffer, '\n');
            if (newline_pos) {
                // Found complete packet
                ssize_t packet_len = newline_pos - packet_buffer + 1;
                
                // Write packet to file
                if (write(data_fd, packet_buffer, packet_len) < 0) {
                    syslog(LOG_ERR, "Failed to write to data file: %s", strerror(errno));
                }
                
                // Read entire file and send back to client
                close(data_fd);
                data_fd = open(DATA_FILE, O_RDONLY);
                if (data_fd < 0) {
                    syslog(LOG_ERR, "Failed to reopen data file for reading: %s", strerror(errno));
                    free(packet_buffer);
                    close(client_fd);
                    continue;
                }
                
                // Get file size
                struct stat file_stat;
                if (fstat(data_fd, &file_stat) < 0) {
                    syslog(LOG_ERR, "Failed to get file size: %s", strerror(errno));
                    free(packet_buffer);
                    close(data_fd);
                    close(client_fd);
                    continue;
                }
                
                // Send file content to client
                off_t file_size = file_stat.st_size;
                ssize_t bytes_sent = 0;
                while (bytes_sent < file_size) {
                    ssize_t chunk_size = (file_size - bytes_sent > BUFFER_SIZE) ? BUFFER_SIZE : (file_size - bytes_sent);
                    ssize_t bytes_read_file = read(data_fd, buffer, chunk_size);
                    if (bytes_read_file <= 0) {
                        break;
                    }
                    
                    ssize_t bytes_sent_chunk = send(client_fd, buffer, bytes_read_file, 0);
                    if (bytes_sent_chunk < 0) {
                        syslog(LOG_ERR, "Failed to send data to client: %s", strerror(errno));
                        break;
                    }
                    bytes_sent += bytes_sent_chunk;
                }
                
                // Reset for next packet
                free(packet_buffer);
                packet_buffer = NULL;
                packet_size = 0;
                
                // Close file and reopen for next packet
                close(data_fd);
                data_fd = open(DATA_FILE, O_CREAT | O_WRONLY | O_APPEND, 0644);
                if (data_fd < 0) {
                    syslog(LOG_ERR, "Failed to reopen data file for appending: %s", strerror(errno));
                    close(client_fd);
                    break;
                }
            }
        }
        
        // Clean up
        if (packet_buffer) {
            free(packet_buffer);
        }
        close(data_fd);
        close(client_fd);
        
        syslog(LOG_INFO, "Closed connection from %s", client_ip);
    }
    
    // Cleanup and exit
    cleanup_and_exit(server_fd);
    
    return 0;
}
