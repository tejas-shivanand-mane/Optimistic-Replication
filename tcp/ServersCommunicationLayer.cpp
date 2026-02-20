#include <unordered_map>
#include <cstdlib>
#include <vector>

#include "Thread.cpp"
#include "ServerConnection.cpp"
#include "Socket.cpp"
// #include "../wellcoordination/src/replicated_object.hpp"
// #include "../protocol2-partialsyn.cpp"

using namespace std;
using namespace amirmohsen;
using namespace std::chrono;

int print_id = 1;

class ServersCommunicationLayer : public Thread
{

private:
    int id;
    // ReplicatedObject* object;
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

    bool isConnectionDead(int remoteId);
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
    // this->syn_counter = syn_counter;
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

// broadcasts the message to others (does not send it to self)
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

    handleAllReceives(); // This is the single receiver loop

    accept_thread.join(); // Will never be reached unless you break the loop
}

void ServersCommunicationLayer::handleAllReceives()
{

    auto last_print_time = steady_clock::now();

    while (true)
    {
        
        int id = 1;
        for (auto &pair : connections)
        {
            ServerConnection *conn = pair.second;
            if (conn != nullptr)
            {
                conn->receive();
            }
            /*// Print every 1000 millisecond
            auto now = steady_clock::now();
            if (duration_cast<milliseconds>(now - last_print_time).count() >= 1000)
            {
                std::cout << "[handleAllReceives] active id: " <<"Print id: "<<print_id<<" node id: " << id<< std::endl;
                print_id++;
                last_print_time = now;
            }
            id++;*/
        }
    }
}

void ServersCommunicationLayer::closeAllSockets()
{
    for (auto &pair : connections)
    {
        ServerConnection *conn = pair.second;
        if (conn)
        {
            conn->closeSocket();
        }
    }
}


bool ServersCommunicationLayer::isConnectionDead(int remoteId) {
    auto it = connections.find(remoteId);
    if (it == connections.end()) return false;
    return it->second->isSocketNull();
}
