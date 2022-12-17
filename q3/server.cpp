#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>

/////////////////////////////
#include <iostream>
#include <assert.h>
#include <tuple>
#include <pthread.h>
#include <bits/stdc++.h>
using namespace std;
/////////////////////////////

// Regular bold text
#define BBLK "\e[1;30m"
#define BRED "\e[1;31m"
#define BGRN "\e[1;32m"
#define BYEL "\e[1;33m"
#define BBLU "\e[1;34m"
#define BMAG "\e[1;35m"
#define BCYN "\e[1;36m"
#define ANSI_RESET "\x1b[0m"

typedef long long LL;

#define pb push_back
#define debug(x) cout << #x << " : " << x << endl
#define part cout << "-----------------------------------" << endl;

///////////////////////////////
#define MAX_CLIENTS 4
#define PORT_ARG 8001

const int initial_msg_len = 256;

int num_nodes;

vector<vector<pair<int, int> > > adj;

struct routing_table_entry
{
    int dest;
    int next_hop;
    int delay;
};

struct routing_table
{
    routing_table_entry *entries;
};

struct message
{
    int source;
    int destination;
    int fwd_destination;
    string msg;
    struct routing_table rt;
    int delay;
};

vector<queue<message> > queues;
vector<pthread_mutex_t> locks;
vector<pthread_cond_t> conds;

routing_table *rt0;

// vector<string> adv_tokenizer(string s, char del)
// {
//     vector<string> tokens;
//     stringstream ss(s);
//     string word;
//     while (!ss.eof()) {
//         getline(ss, word, del);
//         tokens.push_back(word);
//         cout << word << endl;
//     }
//     return tokens;
// }

vector<string> simple_tokenizer(string s)
{
    stringstream ss(s);
    string word;
    vector<string> tokens;
    while (ss >> word) {
        tokens.push_back(word);
    }
    return tokens;
}

////////////////////////////////////

const LL buff_sz = 1048576;
///////////////////////////////////////////////////
pair<string, int> read_string_from_socket(const int &fd, int bytes)
{
    std::string output;
    output.resize(bytes);

    int bytes_received = read(fd, &output[0], bytes - 1);
    debug(bytes_received);
    if (bytes_received <= 0)
    {
        cerr << "Failed to read data from socket. \n";
    }

    output[bytes_received] = 0;
    output.resize(bytes_received);
    // debug(output);
    // return {output, bytes_received};
    return make_pair(output, bytes_received);
}

int send_string_on_socket(int fd, const string &s)
{
    // debug(s.length());
    int bytes_sent = write(fd, s.c_str(), s.length());
    if (bytes_sent < 0)
    {
        cerr << "Failed to SEND DATA via socket.\n";
    }

    return bytes_sent;
}

///////////////////////////////

void handle_connection(int client_socket_fd)
{
    // int client_socket_fd = *((int *)client_socket_fd_ptr);
    // ####################################################

    int received_num, sent_num;

    /* read message from client */
    int ret_val = 1;

    while (true)
    {
        string cmd;
        // tie(cmd, received_num) = read_string_from_socket(client_socket_fd, buff_sz);
        pair<string, int> p = read_string_from_socket(client_socket_fd, buff_sz);
        cmd = p.first;
        received_num = p.second;
        ret_val = received_num;
        string rtable = "";

        // stringstream ss;

        // ss << cmd;
        // string command;
        // int dest;
        // string msg;
        // ss >> command >> dest >> msg;

        vector<string> tokens=simple_tokenizer(cmd);
        // debug(ret_val);
        // printf("Read something\n");
        if (ret_val <= 0)
        {
            // perror("Error read()");
            printf("Server could not read msg sent from client\n");
            goto close_client_socket_ceremony;
        }
        cout << "Client sent : " << cmd << endl;
        if (cmd == "exit")
        {
            cout << "Exit pressed by client" << endl;
            goto close_client_socket_ceremony;
        }
        else if (cmd == "pt")
        {
            rtable += "\ndest\tforw\tdelay\n";

            // print rt0 table
            for (int i = 1; i < num_nodes; i++)
            {
                rtable += to_string(i) + "\t" + to_string(rt0->entries[i].next_hop) + "\t" + to_string(rt0->entries[i].delay) + "\n";
            }

            // put /0 at end
            rtable[rtable.length() - 1] = '\0';
        }

        else if(tokens[0]=="send")
        {
            int destin =stoi(tokens[1]);
            
            cout << "Sending message to " << destin << endl;
            string mssgs="";
            for(int i=2;i<tokens.size();i++)
            {
                mssgs+=tokens[i];
                mssgs+=" ";
            }
            cout << "Message is " << mssgs << endl;
            message m = {0,destin,-1,mssgs,NULL,0};
            pthread_mutex_lock(&locks[0]);
            queues[0].push(m);
            pthread_cond_signal(&conds[0]);
            pthread_mutex_unlock(&locks[0]);
        }
         
        string msg_to_send_back = "Ack: " + cmd + "\n" + rtable;

        ////////////////////////////////////////
        // "If the server write a message on the socket and then close it before the client's read. Will the client be able to read the message?"
        // Yes. The client will get the data that was sent before the FIN packet that closes the socket.

        int sent_to_client = send_string_on_socket(client_socket_fd, msg_to_send_back);
        // debug(sent_to_client);
        if (sent_to_client == -1)
        {
            perror("Error while writing to client. Seems socket has been closed");
            goto close_client_socket_ceremony;
        }
    }

close_client_socket_ceremony:
    close(client_socket_fd);
    printf(BRED "Disconnected from client" ANSI_RESET "\n");
    // return NULL;
}

void *thread_handler(void *arg)
{
    int i = *((int *)arg);

    // create routing table for each node
    struct routing_table rt = {new routing_table_entry[num_nodes]};
    rt.entries = new routing_table_entry[num_nodes];

    if (i == 0)
    {
        rt0 = &rt;
    }

    // initialize routing table
    for (int j = 0; j < num_nodes; j++)
    {
        rt.entries[j].dest = j;
        rt.entries[j].next_hop = j;
        rt.entries[j].delay = 1e9;
    }
    // initialize routing table for each node with the adj list
    for (int j = 0; j < adj[i].size(); j++)
    {
        rt.entries[adj[i][j].first].next_hop = adj[i][j].first;
        rt.entries[adj[i][j].first].delay = adj[i][j].second;
    }

    for (int j = 0; j < adj[i].size(); j++)
    {
        message m;
        m.source = i;
        m.destination = adj[i][j].first;
        m.fwd_destination = adj[i][j].first;
        m.delay = adj[i][j].second;
        m.rt = rt;
        m.msg = "";
        pthread_mutex_lock(&locks[adj[i][j].first]);
        queues[adj[i][j].first].push(m);
        pthread_cond_signal(&conds[adj[i][j].first]);
        pthread_mutex_unlock(&locks[adj[i][j].first]);
    }
    while (1)
    {
        pthread_mutex_lock(&locks[i]);
        while (queues[i].empty())
        {
            pthread_cond_wait(&conds[i], &locks[i]);
            // continue;
        }
        message m = queues[i].front();
        queues[i].pop();
        pthread_mutex_unlock(&locks[i]);
        if (m.msg == "")
        {
            // update routing table using bellman ford
            int flag = 0;
            for (int j = 0; j < num_nodes; j++)
            {
                if (rt.entries[j].delay > m.rt.entries[j].delay + m.delay)
                {
                    if (j == i)
                        continue;
                    rt.entries[j].delay = m.rt.entries[j].delay + m.delay;
                    rt.entries[j].next_hop = m.source;
                    flag = 1;
                }
            }
            // send routing table to all neighbours if changed

            if (flag)
            {
                for (int j = 0; j < adj[i].size(); j++)
                {
                    message m;
                    m.source = i;
                    m.destination = adj[i][j].first;
                    m.fwd_destination = adj[i][j].first;
                    m.delay = adj[i][j].second;

                    m.rt = rt;
                    m.msg = "";
                    pthread_mutex_lock(&locks[adj[i][j].first]);
                    queues[adj[i][j].first].push(m);
                    pthread_cond_signal(&conds[adj[i][j].first]);
                    pthread_mutex_unlock(&locks[adj[i][j].first]);
                }
            }
        }
        else
        {
            // send message to destination
            if (m.destination == i)
            {
                // cout << "Message from " << m.source << " to " << m.destination << " : " << m.msg << endl;
                cout << "Data received at node: "<< i << endl;
                cout << "Source: " << m.source << endl;
                cout << "Destination: " << m.destination << endl;
                cout << "Forward_Destination: " << m.fwd_destination << endl;
                cout << "Message: " << m.msg << endl;
            }
            else
            {
                m.fwd_destination = rt.entries[m.destination].next_hop;
                pthread_mutex_lock(&locks[m.fwd_destination]);
                queues[m.fwd_destination].push(m);
                pthread_cond_signal(&conds[m.fwd_destination]);
                pthread_mutex_unlock(&locks[m.fwd_destination]);
            }
        }
    }

    return NULL;
}

int main(int argc, char *argv[])
{
    int i, j, k, t, n;

    int N, M;
    cin >> N >> M;

    num_nodes = N;

    queues.resize(N);
    locks.resize(N);
    conds.resize(N);
    adj.resize(N);

    // initialize adj list

    for (i = 0; i < M; i++)
    {
        int u, v, w;
        cin >> u >> v >> w;
        adj[u].pb(make_pair(v, w));
        adj[v].pb(make_pair(u, w));
    }

    // pthread_t node_threads[N];
    vector<pthread_t> node_threads(N);

    int arr[N];
    for (int i = 0; i < N; i++)
    {
        arr[i] = i;
        pthread_create(&node_threads[i], NULL, thread_handler, (void *)&arr[i]);
    }

    int wel_socket_fd, client_socket_fd, port_number;
    socklen_t clilen;

    struct sockaddr_in serv_addr_obj, client_addr_obj;
    /////////////////////////////////////////////////////////////////////////
    /* create socket */
    /*
    The server program must have a special door—more precisely,
    a special socket—that welcomes some initial contact
    from a client process running on an arbitrary host
    */
    // get welcoming socket
    // get ip,port
    /////////////////////////
    wel_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (wel_socket_fd < 0)
    {
        perror("ERROR creating welcoming socket");
        exit(-1);
    }

    //////////////////////////////////////////////////////////////////////
    /* IP address can be anything (INADDR_ANY) */
    bzero((char *)&serv_addr_obj, sizeof(serv_addr_obj));
    port_number = PORT_ARG;
    serv_addr_obj.sin_family = AF_INET;
    // On the server side I understand that INADDR_ANY will bind the port to all available interfaces,
    serv_addr_obj.sin_addr.s_addr = INADDR_ANY;
    serv_addr_obj.sin_port = htons(port_number); // process specifies port

    /////////////////////////////////////////////////////////////////////////////////////////////////////////
    /* bind socket to this port number on this machine */
    /*When a socket is created with socket(2), it exists in a name space
       (address family) but has no address assigned to it.  bind() assigns
       the address specified by addr to the socket referred to by the file
       descriptor wel_sock_fd.  addrlen specifies the size, in bytes, of the
       address structure pointed to by addr.  */

    // CHECK WHY THE CASTING IS REQUIRED
    if (bind(wel_socket_fd, (struct sockaddr *)&serv_addr_obj, sizeof(serv_addr_obj)) < 0)
    {
        perror("Error on bind on welcome socket: ");
        exit(-1);
    }
    //////////////////////////////////////////////////////////////////////////////////////

    /* listen for incoming connection requests */

    listen(wel_socket_fd, MAX_CLIENTS);
    cout << "Server has started listening on the LISTEN PORT" << endl;

    clilen = sizeof(client_addr_obj);

    while (1)
    {
        /* accept a new request, create a client_socket_fd */
        /*
        During the three-way handshake, the client process knocks on the welcoming door
of the server process. When the server “hears” the knocking, it creates a new door—
more precisely, a new socket that is dedicated to that particular client.
        */
        // accept is a blocking call
        printf("Waiting for a new client to request for a connection\n");
        client_socket_fd = accept(wel_socket_fd, (struct sockaddr *)&client_addr_obj, &clilen);
        if (client_socket_fd < 0)
        {
            perror("ERROR while accept() system call occurred in SERVER");
            exit(-1);
        }

        printf(BGRN "New client connected from port number %d and IP %s \n" ANSI_RESET, ntohs(client_addr_obj.sin_port), inet_ntoa(client_addr_obj.sin_addr));

        handle_connection(client_socket_fd);
    }

    close(wel_socket_fd);
    return 0;
}