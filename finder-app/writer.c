#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    // Check if exactly two arguments are provided
    if (argc != 3) {
        syslog(LOG_ERR, "Error: Two arguments required");
        printf("Error: Two arguments required\n");
        printf("Usage: %s <writefile> <writestr>\n", argv[0]);
        exit(1);
    }
    
    // Assign arguments to variables
    char *writefile = argv[1];
    char *writestr = argv[2];
    
    // Open file for writing
    int fd = open(writefile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        syslog(LOG_ERR, "Error: Could not create file %s: %s", writefile, strerror(errno));
        printf("Error: Could not create file %s\n", writefile);
        exit(1);
    }
    
    // Write the content to the file
    ssize_t bytes_written = write(fd, writestr, strlen(writestr));
    if (bytes_written == -1) {
        syslog(LOG_ERR, "Error: Could not write to file %s: %s", writefile, strerror(errno));
        printf("Error: Could not write to file %s\n", writefile);
        close(fd);
        exit(1);
    }
    
    // Close the file
    if (close(fd) == -1) {
        syslog(LOG_ERR, "Error: Could not close file %s: %s", writefile, strerror(errno));
        printf("Error: Could not close file %s\n", writefile);
        exit(1);
    }
    
    // Log successful write operation with LOG_DEBUG level
    syslog(LOG_DEBUG, "Writing %s to %s", writestr, writefile);
    
    return 0;
}
