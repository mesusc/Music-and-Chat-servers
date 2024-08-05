For the third question we've used the same process for socket creation,bind,listen accept from quote server that sir gave.
The variable names were chosen to align semantically with their respective tasks and comments were mentioned to make the code more readable.
This program implements a multi-client chat server using TCP sockets and pthreads in C. Key features include:
Dynamic Client Handling: Supports multiple clients connecting concurrently.
Username Registration: Clients register unique usernames upon connection.
Real-time Chat: Clients can send messages to each other in real-time.
Special Commands: Supports special commands like \list to display active users and \bye to leave the chat.
Error Handling: Handles socket errors and disconnections gracefully.

To use, compile the program and run it with the appropriate command-line arguments specifying the port and maximum number of clients. There is also an optional timeout command.Command-line arguments were just as mentioned in the question.