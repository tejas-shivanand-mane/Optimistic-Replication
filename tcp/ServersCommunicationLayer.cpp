#include <unordered_map>
#include <cstdlib>
#include <vector>

#include "Thread.cpp"
#include "ServerConnection.cpp"
#include "Socket.cpp"

using namespace std;
using namespace amirmohsen;

class ServersCommunicationLayer : public Thread
{

private:
    int id;
    Handler *handler;
    std::unordered_map<int, ServerConnection *> connections;
    ServerSocket *acceptingSocket;
    std::vector<string> hosts;
    int *ports;
    std::atomic<int> *initcounter;

    ServerConnection *getConnection(int remoteId);
    void establishConnection(Socket *socket, int remoteId);

public:
    ServersCommunicationLayer(int id, std::vector<string> hosts, int *portNumbers, Handler *hdl, std::atomic<int> *initcounter);

    ~ServersCommunicationLayer();

    void broadcast(string &message);
    void broadcast(Buffer *message);
    void connectAll();
    void run();
    void handleAllReceives();
    void closeAllSockets();
};

ServersCommunicationLayer::~ServersCommunicationLayer()
{
    for (auto it : connections)
        delete it.second;
    delete acceptingSocket;
}

ServersCommunicationLayer::ServersCommunicationLayer(int id, std::vector<string> hosts, int *ports, Handler *hdl, std::atomic<int> *initcounter)
{
    this->id = id;
    this->hosts = hosts;
    this->ports = ports;
    this->handler = hdl;
    this->initcounter = initcounter;
    acceptingSocket = new TCPServerSocket(ports[id - 1]);
    connectAll();
}

void ServersCommunicationLayer::connectAll()
{
    for (int i = 1; i <= hosts.size(); i++)
    {
        getConnection(i);
    }
}

ServerConnection *ServersCommunicationLayer::getConnection(int remoteId)
{
    ServerConnection *ret;
    if (connections.find(remoteId) == connections.end())
    {
        ret = new ServerConnection(id, remoteId, NULL, hosts, ports, handler, initcounter);
        connections.insert(std::make_pair(remoteId, ret));
    }
    else
        ret = connections.at(remoteId);
    return ret;
}

void ServersCommunicationLayer::broadcast(string &message)
{
    for (auto it : connections)
    {
        if (getConnection(it.first) != NULL)
            try
            {
                if (it.first != id)
                    getConnection(it.first)->send(message);
            }
            catch (Exception *e)
            {
                cout << e->getMessage() << endl;
                cout << "Unable to send messages to remote " << it.first << endl;
                delete e;
            }
    }
}

void ServersCommunicationLayer::broadcast(Buffer *message)
{
    std::vector<int> failed_sends;
    
    for (auto it : connections)
    {
        if (getConnection(it.first) != NULL)
        {
            try
            {
                if (it.first != id)
                {
                    // Check if node is already marked as failed in handler
                    if (handler != nullptr && handler->failed[it.first - 1]) {
                        continue;  // Skip broadcasting to failed nodes
                    }
                    
                    getConnection(it.first)->send(message);
                }
            }
            catch (Exception *e)
            {
                cout << "Send failed to node " << it.first << ": " << e->getMessage() << endl;
                failed_sends.push_back(it.first);
                delete e;
            }
        }
    }
    
    // Report failed sends
    if (!failed_sends.empty()) {
        cout << "Failed to send to nodes: ";
        for (int node_id : failed_sends) {
            cout << node_id << " ";
        }
        cout << endl;
    }
}

void ServersCommunicationLayer::establishConnection(Socket *socket, int remoteId)
{
    connections.at(remoteId)->reconnect(socket);
}

void ServersCommunicationLayer::run()
{
    std::thread accept_thread([this]()
                              {
        while (true) {
            TCPSocket* newSocket = static_cast<TCPSocket*>(acceptingSocket->accept());
            int remoteId = std::stoi(newSocket->getString());
            establishConnection(newSocket, remoteId);
        } });

    handleAllReceives();

    accept_thread.join();
}

void ServersCommunicationLayer::handleAllReceives()
{
    std::unordered_map<int, int> consecutive_failures;
    const int MAX_FAILURES = 5;  // Mark as failed after 5 consecutive failures
    
    while (true)
    {
        for (auto it = connections.begin(); it != connections.end(); )
        {
            int remote_id = it->first;
            ServerConnection* conn = it->second;
            
            try
            {
                if (conn != nullptr && remote_id != id)
                {
                    conn->receive();
                    
                    // Reset failure counter on successful receive
                    consecutive_failures[remote_id] = 0;
                    ++it;
                }
                else
                {
                    ++it;
                }
            }
            catch (Exception* e)
            {
                cout << "Exception in receive from node " << remote_id 
                     << ": " << e->getMessage() << endl;
                
                // Increment failure counter
                consecutive_failures[remote_id]++;
                
                if (consecutive_failures[remote_id] >= MAX_FAILURES)
                {
                    cout << "Node " << remote_id << " exceeded failure threshold. "
                         << "Marking as FAILED and removing connection." << endl;
                    
                    // Notify handler about the failure
                    if (handler != nullptr) {
                        handler->setfailurenode(remote_id);
                    }
                    
                    // Clean up the connection
                    if (conn != nullptr)
                    {
                        conn->closeSocket();
                        delete conn;
                    }
                    
                    it = connections.erase(it);
                    consecutive_failures.erase(remote_id);
                }
                else
                {
                    cout << "Node " << remote_id << " failure count: " 
                         << consecutive_failures[remote_id] << "/" << MAX_FAILURES << endl;
                    ++it;
                }
                
                delete e;
            }
        }
        
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }
}

void ServersCommunicationLayer::closeAllSockets()
{
    cout << "Closing all sockets..." << endl;
    
    for (auto &pair : connections)
    {
        ServerConnection *conn = pair.second;
        if (conn)
        {
            try {
                conn->closeSocket();
            } catch (...) {
                cout << "Exception while closing socket for node " << pair.first << endl;
            }
        }
    }
    
    cout << "All sockets closed." << endl;
}