## Q1. You are required to write your implementational details in the report.

### Implementation

1. A struct for message, routing table have been created.
2. Input has been taken in form of adjacency matrix.
3. Then for each node a thread is created with attritube id.
4. In the ```thread_handler``` function a routing table is created for the node.
5. Message is sent to all the nodes if change is detected while updating the routing table using Bellman Ford algorithm.
6. Otherwsise message is sent to destination node.
7. Client an server are connected to each other using ```socketpair```.
8. If the input from the user is ```exit```, then the program is terminated.
9. If the input from the user is ```pt```, then the routing table for node 0 is printed.
10. If the input from the user is ```send <node_id> <message>```, then the message is sent to the node with id ```node_id``` with its path indicated by the routing table.

## Q2. How would you handle server failures?

Server failure occurs if a node is not working and to handle this we recalculate the path to the node using Bellman Ford algorithm.
