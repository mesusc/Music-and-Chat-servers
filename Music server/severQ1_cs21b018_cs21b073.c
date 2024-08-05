

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <dirent.h>
#include <signal.h>


#define MAX_FILES 100 // Maximum number of files in the directory


volatile sig_atomic_t broken_pipe_flag=0; // Flag to handle broken pipe signal

// Signal handler for SIGPIPE
void sigpipe_handler(int signum){
    broken_pipe_flag=1;
}

int connected_clients=0; // Counter to keep track of connected clients
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

struct client_information{
    int socket_fd;
    char* client_ip;
    char* song_names[MAX_FILES];
    char* folder;
};

// Function to send a song to the client
void *send_song_to_client(void *arg){
struct client_information* client_1=(struct client_information *)arg;
char* client_ip=strdup(client_1->client_ip);
int client_fd=client_1->socket_fd;
char** song_names=client_1->song_names;
char* folder=strdup(client_1->folder);

pthread_mutex_lock(&mutex);
connected_clients++;
pthread_mutex_unlock(&mutex);

ssize_t bytes_received;
char number_recieved[1024];

// Receive the number indicating the song to be sent
if((bytes_received=recv(client_fd,number_recieved,sizeof(number_recieved),0))<=0){
    perror("Error in recieving number from the client");
    close(client_fd);
    free(client_1);
    pthread_exit(NULL);
}

int number=atoi(number_recieved);
char* our_song=song_names[number-1];

char song_path[255];
// Construct path to the song file
snprintf(song_path, sizeof(song_path), "%s/%s", folder, our_song);

printf("Client IP:%s requested %s\n",client_ip,our_song);
FILE *song_file = fopen(song_path, "rb");
if (!song_file) {
    perror("The song file is not opening");
    close(client_fd);
    free(client_1);
    pthread_exit(NULL);
}

char part_of_song[1024];
// Read and send the song data to the client
    while ((bytes_received = fread(part_of_song, 1, sizeof(part_of_song), song_file)) > 0) {
        if (send(client_fd, part_of_song, bytes_received, 0) == -1) {
            perror("Error sending data to client");
            fclose(song_file);
            close(client_fd);
            pthread_mutex_lock(&mutex);
            connected_clients--;
             pthread_mutex_unlock(&mutex);
            free(client_1);
            pthread_exit(NULL);
        }
    }

//this loop is left delibirately,will help while doing viva
    // while(1){
    //     ;
    // }

    fclose(song_file);
    close(client_fd);
    free(client_1);
    pthread_mutex_lock(&mutex);
    connected_clients--;
    pthread_mutex_unlock(&mutex);
    pthread_exit(NULL);
}

int main(int argc,char* argv[]) {
    if(argc<4){
        printf("Insufficient agguments");
        exit(0);
        return 0;
    }
    int PORT=atoi(argv[1]);
    char* songs_folder=argv[2];
    int max_no_of_clients=atoi(argv[3]);
    signal(SIGPIPE,sigpipe_handler);// Register SIGPIPE signal handler
    int server_fd, client_fd;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    DIR *dir;
    struct dirent *entry;
    char *song_names[MAX_FILES];
    int file_count = 0;

    // Open "music" directory
    dir = opendir(songs_folder);
    if (dir == NULL) {
        perror("opendir");
        exit(EXIT_FAILURE);
    }

    // Read directory entries
    while ((entry = readdir(dir)) != NULL) {
        // Add only regular files to the array
        if (entry->d_type == DT_REG) {
            song_names[file_count] = strdup(entry->d_name);
            file_count++;
            if (file_count >= MAX_FILES) {
                fprintf(stderr, "Too many files\n");
                break;
            }
        }
    }

    // SOCKET - Create TCP socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Set server address details
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // BIND - Bind socket to address and port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Binding failed");
        exit(EXIT_FAILURE);
    }

    // LISTEN - Start listening for incoming connections
    if (listen(server_fd, max_no_of_clients) < 0) {
        perror("Listening failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", PORT);

    while(1) {
        
	    // ACCEPT - accept incoming connection
	    if ((client_fd = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
		perror("Acceptance failed");
		exit(EXIT_FAILURE);
	    }
        pthread_mutex_lock(&mutex);
        if (connected_clients >= max_no_of_clients) {
            printf("Maximum clients connected. Connection rejected.\n");
            char *message="clients exceeded";
            pthread_mutex_unlock(&mutex);
             close(client_fd);
            continue;
        }
        pthread_mutex_unlock(&mutex);
        

	    char client_ip[INET_ADDRSTRLEN];
	    inet_ntop(AF_INET, &(address.sin_addr), client_ip, INET_ADDRSTRLEN);
	    printf("Client IP address: %s\n", client_ip);
        struct client_information* client_=(struct client_information *)malloc(sizeof(struct client_information));
        client_->socket_fd=client_fd;
        client_->client_ip=strdup(client_ip);
        client_->folder=strdup(songs_folder);
        for (int i = 0; i < file_count; i++) {
           client_->song_names[i] = strdup(song_names[i]);
        }
	     pthread_t client_thread;
        if (pthread_create(&client_thread, NULL, send_song_to_client, (void *)client_) != 0) {
            perror("Error creating thread");
            close(client_fd);
            free(client_);
        }
    }
    
    close(server_fd);
    return 0;
}
