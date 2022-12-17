# Multi-Threading

The following tasks have been implemented using multithreading in C language. 

## Task 1: Washing machine

• N students will come to get their clothes washed

• There are M functioning washing machines

• The time at which the i-th student comes is T_i seconds after the execution of the program

• The time taken to wash the i-th student's clothes is W_i seconds

• The patience of the i-th student is P_i seconds, after which he leaves without getting his clothes washed

More washing machines are needed if at least 25% of the students who came to the machines returned without getting their clothes washed.

## Task 2: Pizzeria Gino Sorbillo

Gino’s pizza is known to be the best pizza in the planet. It was featured in various movies, and people travel miles to eat the pizza. We want to simulate the functioning of the pizzeria. 
- The pizzeria offers M pizzas and N chefs are employed in the pizzeria.
- Unfortunately, these chefs have other part-time jobs and hence cannot reach the pizzeria at the same time.
- Given the rectangular layout of the restaurant, the drive-thru can only support K cars.

The input contains the following:

- the number of chefs (n), 
- the number of pizza varieties (m), 
- the number of limited ingredients (i), 
- the number of customers (c), 
- the number of ovens (o), 
- the time for a customer to reach the pickup spot (k).

Outputs are sequentially deterministic. The order of outputs can vary depending on synchronisation.
## Task 3: Internet Routing

A routing table is a set of rules, often viewed in table format, that is used to determine where data packets traveling over an Internet Protocol (IP) network will be directed. In this question you will be simulating the process of generating and using a routing table.

Two programs for this purpose are created:

1. Server 

- The server program will simulate all the nodes in the network with threads.
These threads should only communicate with each other through message
passing via sockets.
- Initially the node only knows the communication delay with the nodes it is
directly connected with. You will have to generate the routing table for every
node after communicating with each other node.
- When any data is received, the node must print the source, destination, data
received and the forwarded destination if the current node had forwarded it.

- Example : ```Data received at node: 4 : Source : 0; Destination : 5; Fowarded_Destination : 3; Message : “Hello!";```

2. Client

- The client program will provide a shell to interact with the network simulated by the server. This shell implements the following commands.
- Print table: pt prints the routing table of the node 0. The routing table consists of the destination node, delay and the node to which the packet needs to be forwarded to.
- The output of the pt command is as follows- 

                    dest forw delay

                    1     1     15

                    2     1     30 

                    3     3     40

- Send message: send send sends a message to node 0 which will be sent through the network to the destination node.
- The format of the send command is as follows- - > send 4 Ping!
- This message will be sent to node 0, which will be forwarded through the network till it reaches the destination node 4. All the nodes that receive this message need to print the information as specified in the server program.

Implementation details for each programs are given in the README of their respective folders.