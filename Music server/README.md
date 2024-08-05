For the first question we've used the same process for socket creation,bind,listen accept from quote server that sir gave.
Number of connected clients was kept in track to ensure maximum number of connections and a lock was used for it to avoid race condition
The variable names were chosen to align semantically with their respective tasks and comments were mentioned to make the code more readable.
This program sets up a TCP server to stream songs to clients. It allows multiple clients to connect simultaneously and request songs from a specified folder. Key features include:
Dynamic Client Handling: Supports concurrent connections and manages multiple clients using threads.
Song Streaming: Streams requested songs to clients over TCP connections.
Error Handling: Handles broken pipe signals and client connection limits gracefully.
Simple Setup: Command-line arguments specify the port, songs folder, and maximum clients.

To use, compile the program and run it with the appropriate command-line arguments. Clients can connect to the server, request songs by index, and enjoy streaming music.Command-line arguments were just as mentioned in the question.