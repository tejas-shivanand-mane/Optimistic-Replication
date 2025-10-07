#include <unordered_map>
#include <cstdlib>
#include <vector>
#include <mutex>
#include <atomic>
#include <iostream>
#include <chrono>
#include <iomanip>

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
    
    // Fault tolerance additions
    std::unordered_map<int, std::atomic<bool>> node_status;
    std::mutex connections_mutex;
    std::atomic<bool> running{true};

    ServerConnection *getConnection(int remoteId);
    void establishConnection(Socket *socket, int remoteId);
    void markNodeDown(int nodeId);
    void markNodeUp(int nodeId);
    bool isNodeAlive(int nodeId);
    std::string getTimestamp();
    void logFailure(int nodeId, const std::string& context);
    void logRecovery(int nodeId);

public:
    ServersCommunicationLayer(int id, std::vector<string> hosts, int *portNumbers, Handler *hdl, std::atomic<int> *initcounter);

    ~ServersCommunicationLayer();

    void broadcast(string &message);
    void broadcast(Buffer *message);
    void connectAll();
    void run();
    void handleAllReceives();
    void closeAllSockets();
    void shutdown();
};

// Get current timestamp for logging
std::string ServersCommunicationLayer::getTimestamp()
{
    auto now = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);
    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&now_time_t), "%Y-%m-%d %H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << now_ms.count();
    return ss.str();
}

void ServersCommunicationLayer::logFailure(int nodeId, const std::string& context)
{
    std::cout << "[" << getTimestamp() << "] [Node " << id << "] FAILURE DETECTED: "
              << "Node " << nodeId << " failed during " << context << std::endl;
}

void ServersCommunicationLayer::logRecovery(int nodeId)
{
    std::cout << "[" << getTimestamp() << "] [Node " << id << "] RECOVERY: "
              << "Node " << nodeId << " has reconnected and is now UP" << std::endl;
}

ServersCommunicationLayer::~ServersCommunicationLayer()
{
    shutdown();
    std::lock_guard<std::mutex> lock(connections_mutex);
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

    // Initialize all nodes as alive
    for (int i = 1; i <= hosts.size(); i++)
    {
        node_status[i].store(true);
    }
    
    std::cout << "[" << getTimestamp() << "] [Node " << id << "] Initializing ServersCommunicationLayer" << std::endl;
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
    std::lock_guard<std::mutex> lock(connections_mutex);
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

void ServersCommunicationLayer::markNodeDown(int nodeId)
{
    bool wasAlive = node_status[nodeId].load();
    node_status[nodeId].store(false);
    
    if (wasAlive) {
        std::cout << "[" << getTimestamp() << "] [Node " << id << "] STATUS CHANGE: "
                  << "Node " << nodeId << " marked as DOWN" << std::endl;
    }
}

void ServersCommunicationLayer::markNodeUp(int nodeId)
{
    bool wasDown = !node_status[nodeId].load();
    node_status[nodeId].store(true);
    
    if (wasDown) {
        logRecovery(nodeId);
    }
}

bool ServersCommunicationLayer::isNodeAlive(int nodeId)
{
    return node_status[nodeId].load();
}

// broadcasts the message to others (does not send it to self)
void ServersCommunicationLayer::broadcast(string &message)
{
    std::lock_guard<std::mutex> lock(connections_mutex);
    
    for (auto it : connections)
    {
        if (it.first == id) continue;  // Skip self
        
        // Skip known dead nodes
        if (!isNodeAlive(it.first)) continue;
        
        if (getConnection(it.first) != NULL)
        {
            try
            {
                getConnection(it.first)->send(message);
            }
            catch (Exception *e)
            {
                logFailure(it.first, "broadcast (string)");
                markNodeDown(it.first);
                delete e;
            }
        }
    }
}

void ServersCommunicationLayer::broadcast(Buffer *message)
{
    std::lock_guard<std::mutex> lock(connections_mutex);
    
    int alive_count = 0;
    int dead_count = 0;
    
    for (auto it : connections)
    {
        if (it.first == id) continue;  // Skip self
        
        // Skip known dead nodes
        if (!isNodeAlive(it.first)) {
            dead_count++;
            continue;
        }
        
        if (getConnection(it.first) != NULL)
        {
            try
            {
                getConnection(it.first)->send(message);
                alive_count++;
            }
            catch (Exception *e)
            {
                logFailure(it.first, "broadcast (buffer)");
                markNodeDown(it.first);
                dead_count++;
                delete e;
            }
        }
    }
    
    // Log broadcast summary if there are failures
    if (dead_count > 0) {
        std::cout << "[" << getTimestamp() << "] [Node " << id << "] BROADCAST: "
                  << "Sent to " << alive_count << " nodes, "
                  << dead_count << " nodes unreachable" << std::endl;
    }
}

void ServersCommunicationLayer::establishConnection(Socket *socket, int remoteId)
{
    std::lock_guard<std::mutex> lock(connections_mutex);
    connections.at(remoteId)->reconnect(socket);
    markNodeUp(remoteId);  // Mark node as alive when connection is established
}

void ServersCommunicationLayer::run()
{
    std::cout << "[" << getTimestamp() << "] [Node " << id << "] Starting communication threads" << std::endl;
    
    std::thread accept_thread([this]()
    {
        std::cout << "[" << getTimestamp() << "] [Node " << id << "] Accept thread started" << std::endl;
        
        while (running.load()) 
        {
            try 
            {
                TCPSocket* newSocket = static_cast<TCPSocket*>(acceptingSocket->accept());
                int remoteId = std::stoi(newSocket->getString());
                
                std::cout << "[" << getTimestamp() << "] [Node " << id << "] CONNECTION: "
                          << "Accepted connection from node " << remoteId << std::endl;
                
                establishConnection(newSocket, remoteId);
            }
            catch (Exception* e)
            {
                if (running.load())  // Only log if not shutting down
                {
                    std::cout << "[" << getTimestamp() << "] [Node " << id << "] Accept thread exception: " 
                              << e->getMessage() << std::endl;
                }
                delete e;
            }
        }
        
        std::cout << "[" << getTimestamp() << "] [Node " << id << "] Accept thread terminated" << std::endl;
    });

    handleAllReceives();

    accept_thread.join();
}

void ServersCommunicationLayer::handleAllReceives()
{
    std::cout << "[" << getTimestamp() << "] [Node " << id << "] Receive loop started" << std::endl;
    
    while (running.load())
    {
        std::lock_guard<std::mutex> lock(connections_mutex);
        
        for (auto it = connections.begin(); it != connections.end(); ++it)
        {
            int nodeId = it->first;
            ServerConnection* conn = it->second;
            
            // Skip dead nodes or self
            if (nodeId == id || !isNodeAlive(nodeId)) continue;
            
            try
            {
                if (conn != nullptr)
                {
                    conn->receive();
                }
            }
            catch (Exception* e)
            {
                logFailure(nodeId, "receive");
                markNodeDown(nodeId);
                
                if (conn != nullptr)
                {
                    conn->closeSocket();
                }
                delete e;
            }
        }
        
        // Small sleep to prevent busy waiting
        std::this_thread::sleep_for(std::chrono::microseconds(10));
    }
    
    std::cout << "[" << getTimestamp() << "] [Node " << id << "] Receive loop terminated" << std::endl;
}

void ServersCommunicationLayer::closeAllSockets()
{
    std::lock_guard<std::mutex> lock(connections_mutex);
    std::cout << "[" << getTimestamp() << "] [Node " << id << "] Closing all sockets" << std::endl;
    
    for (auto &pair : connections)
    {
        ServerConnection *conn = pair.second;
        if (conn)
        {
            conn->closeSocket();
        }
    }
}

void ServersCommunicationLayer::shutdown()
{
    std::cout << "[" << getTimestamp() << "] [Node " << id << "] Shutting down ServersCommunicationLayer" << std::endl;
    running.store(false);
    closeAllSockets();
}