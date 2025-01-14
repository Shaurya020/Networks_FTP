#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <dirent.h>

#define DSK "./files/"

int connect_to_server(char *ip, char *port);
int send_file(int sockfd, char *filename);
int fetch_file(int sockfd, char *filename);
int send_files_with_ext(int sockfd, char *ext);
int fetch_files_with_ext(int sockfd, char *ext);
int break_line(char *str, char *words[]);
int ls(int sockfd);
void error(const char *msg);
void print(const char *msg);

int main(int argc, char const *argv[])
{
    if (argc != 3)
    {
        error("Usage: ./client <server IP> <port>");
    }

    int sockfd;
    sockfd = connect_to_server(argv[1], argv[2]);

    char command[1000], buffer[1000];
    char *args[10];

    while (1)
    {
        bzero(command, 1000);
        bzero(buffer, 1000);

        printf("$ ");

        // Read command
        fgets(command, 1000, stdin);
        print("");

        int cargs = break_line(command, args);
        if (cargs == 0)
        {
            printf("$ ");
            continue;
        }

        // Check for command function
        if (strcmp("GET", args[0]) == 0)
        {
            if (cargs != 2)
            {
                print("Invalid Syntax.");
            }
            else
            {
                // Tell server command to be executed
                write(sockfd, args[0], strlen(args[0]));

                bzero(buffer, 1000);
                read(sockfd, buffer, 1000);
                if (strcmp("OK", buffer))
                {
                    print("ERROR");
                    continue;
                }

                // Send file name to server
                write(sockfd, args[1], strlen(args[1]));

                fetch_file(sockfd, args[1]);
            }
        }
        else if (strcmp("PUT", args[0]) == 0)
        {
            if (cargs != 2)
            {
                print("Invalid Syntax.");
            }
            else
            {
                // Tell server command to be executed
                write(sockfd, args[0], strlen(args[0]));

                bzero(buffer, 1000);
                read(sockfd, buffer, 1000);
                if (strcmp("OK", buffer))
                {
                    print("ERROR");
                    continue;
                }
                // Send file name to server
                write(sockfd, args[1], strlen(args[1]));

                send_file(sockfd, args[1]);
            }
        }
        else if (strcmp("MPUT", args[0]) == 0)
        {
            if (cargs != 2)
            {
                print("Invalid Syntax.");
            }
            else
            {
                // Tell server command to be executed
                write(sockfd, args[0], strlen(args[0]));

                bzero(buffer, 1000);
                read(sockfd, buffer, 1000);
                if (strcmp("OK", buffer))
                {
                    print("ERROR");
                    continue;
                }

                // Send extension to server
                write(sockfd, args[1], strlen(args[1]));

                send_files_with_ext(sockfd, args[1]);
            }
        }
        else if (strcmp("MGET", args[0]) == 0)
        {
            if (cargs != 2)
            {
                print("Invalid Syntax.");
            }
            else
            {
                // Tell server command to be executed
                write(sockfd, args[0], strlen(args[0]));

                bzero(buffer, 1000);
                read(sockfd, buffer, 1000);
                if (strcmp("OK", buffer))
                {
                    print("ERROR");
                    continue;
                }
                // Send extension to server
                write(sockfd, args[1], strlen(args[1]));

                fetch_files_with_ext(sockfd, args[1]);
            }
        }
        else if (strcmp("ls", args[0]) == 0)
        {
            if (cargs != 1)
            {
                print("Invalid Syntax.");
            }
            else
            {
                // Tell server command to be executed
                write(sockfd, args[0], strlen(args[0]));

                ls(sockfd);
            }
        }
        else if (strcmp("exit", args[0]) == 0)
        {
            if (cargs != 1)
            {
                print("Invalid Syntax.");
            }
            print("Adios amigo.");
            print("");
            break;
        }
        else
        { // default
            print("Invalid Command.");
            print("");
        }
    }
    close(sockfd);
    return 0;
}

//  to print error message and exit the program
void error(const char *msg)
{
    printf("%s\n", msg);
    exit(1);
}

void print(const char *msg)
{
    printf("%s\n", msg);
}

// break command into words
int break_line(char *str, char *words[])
{
    str = strtok(str, "\n");

    int k = 0; // Store number of words
    char *ptr = strtok(str, " ");

    while (ptr != NULL)
    {
        words[k++] = ptr;
        ptr = strtok(NULL, " ");
    }

    return k;
}

int connect_to_server(char *ip, char *port)
{
    print("");
    int portno = atoi(port);

    // Open socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0); // Addr domain = IPv4,
    // Comm type = TCP, Protocol = Internet Protocol

    if (sockfd < 0)
    {
        error("Error opening socket.");
    }

    struct sockaddr_in servAddr;

    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(portno);

    if (inet_pton(AF_INET, ip, &servAddr.sin_addr) < 0)
    {
        error("Invalid Address.");
    }

    // connect to server
    int sockconn = connect(sockfd, (struct sockaddr *)&servAddr,
                           sizeof(servAddr));

    if (sockconn < 0)
    {
        error("Error connecting to server.");
    }
    printf("CONNECTED TO %s:%s.\n", ip, port);
    print("");
    return sockfd;
}

int send_file(int sockfd, char *filename)
{
    printf("Sending %s. \n", filename);
    char filepath[256];
    bzero(filepath, 256);
    strcpy(filepath, DSK);
    strcat(filepath, filename);

    FILE *fd = fopen(filepath, "r");

    char buffer[1024];

    read(sockfd, buffer, 1024);

    if (fd == NULL)
    {
        // if file does not exist
        write(sockfd, "ABORT", 5);
        read(sockfd, buffer, 1024);
        print("File does not exist.");
        print("");
        return -1;
    }

    write(sockfd, "OK", 2);

    bzero(buffer, 1024);
    read(sockfd, buffer, 1024);

    if (strcmp(buffer, "EXIST") == 0)
    {
        // if file already exists on server then abort operation
        bzero(buffer, 1024);
        sprintf(buffer, "%s already exists on the server. Do you want to overwrite? (Y / N)", filename);
        print(buffer);

        bzero(buffer, 1024);
        fgets(buffer, 1024, stdin);
        if (buffer[0] != 'Y')
        {
            write(sockfd, "ABORT", 5);
            read(sockfd, buffer, 1024);
            print("Aborting.");
            print("");
            fclose(fd);
            return -1;
        }
        else
        {
            write(sockfd, "OK", 2);
            bzero(buffer, 1024);
            read(sockfd, buffer, 1024);
        }
    }

    if (strcmp(buffer, "OK") != 0)
    {
        print("Error creating file on server.");
        print("");
        fclose(fd);
        return -1;
    }
    int size;

    fseek(fd, 0L, SEEK_END);
    size = ftell(fd);

    fseek(fd, 0L, SEEK_SET);
    // check if file empty
    if (size == 0)
    {
        write(sockfd, "EMPTY", 5);
        goto end;
    }
    write(sockfd, "OK", 2);

    read(sockfd, buffer, 1024);
    // send file contents
    do
    {
        bzero(buffer, 1024);
        size = fread(buffer, sizeof(char), 1023, fd);
        send(sockfd, buffer, size, 0);
    } while (size == 1023);

end:
    read(sockfd, buffer, 1024);

    fclose(fd);

    bzero(buffer, 1024);
    sprintf(buffer, "Successfully sent file %s.", filename);
    print(buffer);
    print("");

    return 0;
}

int fetch_file(int sockfd, char *filename)
{
    char buffer[1024];
    bzero(buffer, 1024);
    read(sockfd, buffer, 1024);
    if (strcmp("OK", buffer) != 0)
    {
        print(buffer);
        print("");
        return -1;
    }

    char filepath[256];
    bzero(filepath, 256);
    strcpy(filepath, DSK);
    strcat(filepath, filename);

    // to check if file already exists in client directory
    FILE *fd = fopen(filepath, "r");

    if (fd != NULL)
    {
        sprintf(buffer, "%s already exists. Do you want to overwrite? (Y / N)", filename);
        print(buffer);
        fgets(buffer, 1024, stdin);
        if (buffer[0] != 'Y')
        {
            write(sockfd, "ABORT", 5);
            read(sockfd, buffer, 1024);
            print("Aborting.");
            print("");
            return -1;
        }
    }

    fd = fopen(filepath, "w");

    if (fd == NULL)
    {
        // couldn't create file
        write(sockfd, "ABORT", 5);
        read(sockfd, buffer, 1024);
        print("Error creating file.");
        print("");
        return -1;
    }

    write(sockfd, "OK", 1024);

    bzero(buffer, 1024);
    read(sockfd, buffer, 1024);
    if (strcmp(buffer, "EMPTY") == 0)
    {
        write(sockfd, "OK", 2);
        goto end;
    }
    write(sockfd, "OK", 2);
    // fetch file contents
    while (1)
    {
        bzero(buffer, 1023);
        recv(sockfd, buffer, 1023, 0);
        buffer[1023] = '\0';

        fwrite(buffer, sizeof(char), strlen(buffer), fd);
        if (strlen(buffer) < 1023)
            break;
    }
end:
    fclose(fd);
    bzero(buffer, 1024);

    sprintf(buffer, "Successfully recieved file %s.", filename);
    print(buffer);
    print("");

    return 0;
}

int ls(int sockfd)
{
    char buffer[256];
    while (1)
    {
        bzero(buffer, 256);
        read(sockfd, buffer, 256);

        if (strcmp(buffer, "DONE") == 0)
            // Indicates all file names recieved
            break;

        print(buffer);

        write(sockfd, "OK", 2);
    }
    print("");
    write(sockfd, "OK", 2);
    bzero(buffer, 256);
    read(sockfd, buffer, 256);
    print(buffer);
    print("");
    return 0;
}

int send_files_with_ext(int sockfd, char *extension)
{
    printf("Sending files with %s extension.\n", extension);
    print("");
    char buffer[256];
    read(sockfd, buffer, 256);

    // open client directory
    DIR *di;
    struct dirent *dir;
    di = opendir(DSK);
    char *filename;
    char *ext;
    int count = 0; // store number of files sent
    int res;

    while ((dir = readdir(di)) != NULL)
    {
        filename = dir->d_name;
        if (filename == NULL)
            continue;
        ext = strtok(filename, ".");
        if (ext == NULL)
            continue;
        // get ectension of file
        ext = strtok(NULL, ".");
        if (ext != NULL && strcmp(ext, extension) == 0)
        {
            strcat(filename, ".");
            strcat(filename, extension);
            // Tell server the file to be sent
            write(sockfd, filename, strlen(filename));
            res = send_file(sockfd, filename);
            if (res >= 0)
                count++;
        }
    }

    // Indicate all files sent
    write(sockfd, "DONE", 4);
    read(sockfd, buffer, 256);
    printf("--%d file(s) sent.\n", count);
    print("");
    return 1;
}

int fetch_files_with_ext(int sockfd, char *extension)
{
    printf("Fetching files with %s extension.\n", extension);
    print("");
    char buffer[256];
    int count = 0, res;
    while (1)
    {
        bzero(buffer, 256);
        read(sockfd, buffer, 256);
        if (strcmp(buffer, "DONE") == 0) // Indicates all files recieved
            break;
        write(sockfd, "OK", 2);
        res = fetch_file(sockfd, buffer);
        write(sockfd, "OK", 2);
        if (res >= 0)
            count++;
    }
    printf("--Recieved %d file(s).\n", count);
    print("");
    return 0;
}