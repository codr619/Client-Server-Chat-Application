# Client-Server Chat Application

## Description

This is a simple client-server chat application implemented in C using sockets. The application supports basic chat functionalities such as sending messages to all connected clients (broadcast) and sending private messages to specific clients. It also allows users to list connected clients and gracefully quit the chat application.

## Features

### Client Side:
- Connect to the server using the server's IP address and port number.
- Register a nickname upon connecting.
- Send messages to the server, including private messages to specific clients and broadcast messages to all connected clients.
- Display messages received from the server.
- Handle commands for listing connected clients, sending private messages, broadcasting messages, and quitting the application.

### Server Side:
- Listen for incoming client connections on a specified port.
- Accept multiple client connections concurrently.
- Receive messages from clients, including private messages and broadcast messages.
- Send messages to specific clients or broadcast messages to all connected clients.
- Implement commands to manage client connections and shut down the server gracefully.

## Compilation Instructions

To compile the server and client applications, follow these steps:

1. Open a terminal and navigate to the directory containing the source code files.

2. Compile the server application:
    ```bash
    gcc -o server server.c -lpthread
    ```

3. Compile the client application:
    ```bash
    gcc -o client client.c -lpthread
    ```

## Running the Applications

### Running the Server

1. Open a terminal and navigate to the directory containing the `server` executable.

2. Start the server:
    ```bash
    ./server
    ```

3. The server will start listening on port 5000. You should see the message:
    ```
    Server started. Listening on port 5000...
    ```

### Running the Client

1. Open a new terminal window or tab for each client instance.

2. Navigate to the directory containing the `client` executable.

3. Start the client:
    ```bash
    ./client
    ```

4. Follow the prompts to enter the server's IP address and port number (e.g., 127.0.0.1 for localhost and 5000 for the port).

5. Register a nickname when prompted.

### Example Client Interaction

```shell
Enter server IP address: 127.0.0.1
Enter server port number: 5000
Enter your nickname: ClientA
=== WELCOME TO THE CHATROOM ===
> /broadcast Hello, everyone!
ClientA: Hello, everyone!
> /private ClientB Hi there!
ClientB: Hi there!
> /list
Connected clients:
- ClientA (127.0.0.1:5000)
- ClientB (127.0.0.1:5000)
- ClientC (127.0.0.1:5000)
> /quit
