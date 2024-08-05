
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>


// Structure to hold client information
typedef struct {
    int socket_fd;
    char* username;
    time_t last_active;
} ClientInfo;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

ClientInfo client_details[1024];
int client_present_status[1024];
int clients_entered=0;
int active_clients=0;
int time_out=10000;

void send_all(char* message,int client_index);
void* handle_client(void *arg);
void remove_client(int client_index);



int main(int argc,char *argv[]) {

    if(argc<3){
        printf("Insufficient arguments\n");
        return 0;
    }
    int PORT=atoi(argv[1]);
    int max_no_of_clients=atoi(argv[2]);
    
    // Optional timeout argument
    if(argc==4){
        
        time_out=atoi(argv[3]);
    }

    int server_fd, client_fd;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

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
        if(active_clients>=max_no_of_clients){
            printf("clients exceeded\n");
            close(client_fd);
            continue;
        }

	    char client_ip[INET_ADDRSTRLEN];
	    inet_ntop(AF_INET, &(address.sin_addr), client_ip, INET_ADDRSTRLEN);
	    printf("Client IP address: %s\n", client_ip);
        client_details[clients_entered].socket_fd=client_fd;
        client_present_status[clients_entered]=1;
	    clients_entered++;
        active_clients++;
        pthread_t client_1;
        pthread_create(&client_1,NULL,handle_client,&clients_entered);
       
    }
    
    close(server_fd);
    return 0;
}


// Function to handle individual client connections
void* handle_client(void *arg){
    
    int client_index=*((int *)arg);
    
    char buffer[1024];
    int client_fd=client_details[client_index-1].socket_fd;
    client_details[client_index-1].last_active=time(NULL);
   
    char message[100]="Enter username: ";
    // Send prompt for username
    send(client_fd,message,strlen(message),0);
    // Receive username from client
    int bytes_received=recv(client_fd,buffer,sizeof(buffer),0);
    if(bytes_received<=0){
        close(client_fd);
        client_present_status[client_index-1]=0;
        active_clients--;
        return NULL;
    }
    buffer[bytes_received]='\0';
    client_details[client_index-1].username=buffer;
    // Send welcome message
    char str1[]="Welocome ";
    strcat(str1,buffer);
    int l=send(client_fd,str1,strlen(str1),0);
    if(l==-1)
    {
        perror("Error sending data to client");
        client_present_status[client_index-1]=0;
        close(client_fd);
    }

    // Validate username uniqueness
    char *name=(char *)malloc(1024*sizeof(char)); 
    strcpy(name,buffer);
    client_details[client_index-1].last_active=time(NULL);
    for(int i=0;i<client_index-1;i++){
        if(client_present_status[i]==1){
            if(strcmp(name,client_details[i].username)==0)
            {
                char *repeat_message="Username already exists!!,So byee";
                send(client_fd,repeat_message,strlen(repeat_message),0);
                close(client_fd);
                active_clients--;
                return NULL;
            }
        }
    }

    // Inform all clients about the new user
    char *inform_all=(char *)malloc(1024*sizeof(char));
    strcpy(inform_all,buffer);
    char *joined=" joined the chat!!";
    strcat(inform_all,joined);
    send_all(inform_all,client_index);
    free(inform_all);

    // Set socket timeout
    struct timeval timeout;
    timeout.tv_sec = time_out;
    timeout.tv_usec = 0;

    if (setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout)) < 0) {
        perror("Error setting socket timeout");
        close(client_fd);
        remove_client(client_index);
        exit(EXIT_FAILURE);
    }

    // Continuously receive and broadcast messages
    while(1){
        if(client_present_status[client_index-1]==0){
            close(client_fd);
            return NULL;
        }
        char buffer2[1024];
        int bytes_received_2=recv(client_fd,buffer2,sizeof(buffer2),0);
        if(bytes_received_2<=0){
            remove_client(client_index);
            break;
            return NULL;
    }
    client_details[client_index-1].last_active=time(NULL);
    
    buffer2[bytes_received_2]='\0';

    // Check for special commands
    if(strcmp(buffer2,"\\list")==0){
        // Send list of active users to the client
        char *list=(char *)malloc(1024*sizeof(char));
        char *usage="Active users:";
        strcpy(list,usage);
        for(int i=0;i<clients_entered;i++){
            char* comma=", ";
            if(client_present_status[i]==1){
                strcat(list,comma);
                strcat(list,client_details[i].username);
            }
        }
         l=send(client_fd,list,strlen(list),0);
        if(l==-1)
        {
            perror("Error sending data to client");
            client_present_status[client_index-1]=0;
            close(client_fd);
            free(list);
            break;
        }
        free(list);
        continue;
    }
    if(strcmp(buffer2,"\\bye")==0){
        remove_client(client_index);
        break;
    }
    char *username=(char *)malloc(1024*sizeof(char));
    strcpy(username,buffer);
    char* isto=": ";
    strcat(username,isto);
    strcat(username,buffer2);
    
    send_all(username,client_index);
    free(username);
    }
    close(client_fd);
    return NULL;

}

// Function to broadcast message to all clients except the client at client_index-1
void send_all(char* message,int client_index)
{
    
    pthread_mutex_lock(&mutex);
    printf("%s\n",message);
    for(int i=0;i<clients_entered;i++)
    {
        int client_fd=client_details[i].socket_fd;
        if(i!=client_index-1){
            if(client_present_status[i]==1){
                int l=send(client_fd,message,strlen(message),0);
                if(l==-1)
                {
                    perror("Error sending data to client");
                    client_present_status[i]=0;
                    close(client_fd);
                }
            }
        }
    }
    pthread_mutex_unlock(&mutex);
}

// Function to remove client from the chat
void remove_client(int client_index)
{
    pthread_mutex_lock(&mutex);
    client_present_status[client_index-1]=0;
    int client_fd=client_details[client_index-1].socket_fd;
    close(client_fd);
    active_clients--;
    char* left_msg=(char *)malloc(1024*sizeof(char));
    strcpy(left_msg,client_details[client_index-1].username);
    pthread_mutex_unlock(&mutex);
    char* left=" left the chat :-)";
    strcat(left_msg,left);
    send_all(left_msg,client_index);
    
    free(left_msg);
}
