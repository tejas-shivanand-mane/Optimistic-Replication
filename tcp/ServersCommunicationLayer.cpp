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

class ServersCommunicationLayer : public Thread
{

public:
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
void ServersCommunicationLayer::broadcast(Buffer *message)
{
    for (auto it = connections.begin(); it != connections.end(); )
    {
        ServerConnection* conn = it->second;

        // prune obviously-dead entries first
        if (!conn || it->first == id || conn->socket == nullptr) {
            ++it;
            continue;
        }

        try {
            conn->send(message);
            ++it;
        }
        catch (Exception *e)
        {
            std::cout << "Broadcast(Buffer*) failed to " << conn->remoteId << " → removing\n";
            std::cout << e->getMessage() << std::endl;
            delete e;
            conn->closeSocket();
            delete conn;
            it = connections.erase(it);
        }
    }
}

void ServersCommunicationLayer::broadcast(std::string &message)
{
    for (auto it = connections.begin(); it != connections.end(); )
    {
        ServerConnection* conn = it->second;

        if (!conn || it->first == id || conn->socket == nullptr) {
            ++it;
            continue;
        }

        try {
            conn->send(message);
            ++it;
        }
        catch (Exception *e)
        {
            std::cout << "Broadcast(string) failed to " << conn->remoteId << " → removing\n";
            delete e;
            conn->closeSocket();
            delete conn;
            it = connections.erase(it);
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
    auto last_print = std::chrono::steady_clock::now();

    while (true)
    {
        for (auto it = connections.begin(); it != connections.end(); )
        {
            ServerConnection* conn = it->second;
            try
            {
                if (conn != nullptr)
                {
                    conn->receive();
                    ++it;  // advance iterator only if no exception
                }
                else
                {
                    it = connections.erase(it);
                    continue;
                }
            }
            catch (Exception* e)
            {
                std::cout << "Receive error on remote " << it->first << " → pruning\n";
                delete e;
                if (conn)
                {
                    conn->closeSocket();
                    delete conn;
                }
                it = connections.erase(it);   // erase returns next iterator
                continue;     
            }
        }

        // 🕒 print once every second to confirm the receive thread is alive
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::seconds>(now - last_print).count() >= 1)
        {
            std::cout << "[Pump] active connections: " << connections.size() << std::endl;
            last_print = now;
        }

        // Slight sleep to prevent busy spin
        std::this_thread::sleep_for(std::chrono::microseconds(20));
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