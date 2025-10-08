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
    for (auto it : connections)
    {
        if (getConnection(it.first) != NULL)
        {
            try
            {
                if (it.first != id)
                {
                    // Skip crashed nodes
                    if (handler != nullptr && handler->failed[it.first - 1]) {
                        continue;
                    }
                    
                    getConnection(it.first)->send(message);
                }
            }
            catch (Exception *e)
            {
                cout << "Send failed to node " << it.first << ": " << e->getMessage() << endl;
                delete e;
            }
        }
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
    while (true)
    {
        for (auto it = connections.begin(); it != connections.end(); ++it)
        {
            int remote_id = it->first;
            ServerConnection* conn = it->second;
            
            try
            {
                if (conn != nullptr && remote_id != id)
                {
                    // Skip if already marked as failed
                    if (handler != nullptr && handler->failed[remote_id - 1]) {
                        continue;
                    }
                    
                    conn->receive();
                }
            }
            catch (Exception* e)
            {
                // Exception already handled in ServerConnection::receive()
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