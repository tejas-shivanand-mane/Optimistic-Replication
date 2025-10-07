#include <unordered_map>
#include <cstdlib>
#include <vector>
#include <mutex>
#include <atomic>
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>

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
    
    // Methods for failure handling
    int getAliveNodeCount();
    std::vector<int> getAliveNodes();
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
    std::cout << "[DESTRUCTOR] Node " << id << " - Entering destructor" << std::endl;
    shutdown();
    std::lock_guard<std::mutex> lock(connections_mutex);
    std::cout << "[DESTRUCTOR] Node " << id << " - Deleting connections" << std::endl;
    for (auto it : connections)
        delete it.second;
    std::cout << "[DESTRUCTOR] Node " << id << " - Deleting acceptingSocket" << std::endl;
    delete acceptingSocket;
    std::cout << "[DESTRUCTOR] Node " << id << " - Destructor complete" << std::endl;
}

ServersCommunicationLayer::ServersCommunicationLayer(int id, std::vector<string> hosts, int *ports, Handler *hdl, std::atomic<int> *initcounter)
{
    std::cout << "[CONSTRUCTOR] Node " << id << " - Starting constructor" << std::endl;
    this->id = id;
    this->hosts = hosts;
    this->ports = ports;
    this->handler = hdl;
    this->initcounter = initcounter;
    
    std::cout << "[CONSTRUCTOR] Node " << id << " - Creating TCPServerSocket on port " << ports[id - 1] << std::endl;
    acceptingSocket = new TCPServerSocket(ports[id - 1]);
    std::cout << "[CONSTRUCTOR] Node " << id << " - TCPServerSocket created successfully" << std::endl;
    
    // Initialize all nodes as alive
    std::cout << "[CONSTRUCTOR] Node " << id << " - Initializing node_status for " << hosts.size() << " nodes" << std::endl;
    for (int i = 1; i <= hosts.size(); i++)
    {
        node_status[i].store(true);
        std::cout << "[CONSTRUCTOR] Node " << id << " - Node " << i << " marked as ALIVE" << std::endl;
    }
    
    std::cout << "[" << getTimestamp() << "] [Node " << id << "] Initializing ServersCommunicationLayer" << std::endl;
    
    std::cout << "[CONSTRUCTOR] Node " << id << " - Calling connectAll()" << std::endl;
    connectAll();
    std::cout << "[CONSTRUCTOR] Node " << id << " - Constructor complete" << std::endl;
}

void ServersCommunicationLayer::connectAll()
{
    std::cout << "[CONNECT_ALL] Node " << id << " - Starting to create connections to all nodes" << std::endl;
    for (int i = 1; i <= hosts.size(); i++)
    {
        std::cout << "[CONNECT_ALL] Node " << id << " - Getting connection to node " << i << std::endl;
        getConnection(i);
        std::cout << "[CONNECT_ALL] Node " << id << " - Connection to node " << i << " obtained" << std::endl;
    }
    std::cout << "[CONNECT_ALL] Node " << id << " - All connections created" << std::endl;
}

ServerConnection *ServersCommunicationLayer::getConnection(int remoteId)
{
    std::cout << "[GET_CONNECTION] Node " << id << " - Getting connection to remote node " << remoteId << std::endl;
    std::lock_guard<std::mutex> lock(connections_mutex);
    std::cout << "[GET_CONNECTION] Node " << id << " - Acquired connections_mutex" << std::endl;
    
    ServerConnection *ret;
    if (connections.find(remoteId) == connections.end())
    {
        std::cout << "[GET_CONNECTION] Node " << id << " - Creating NEW ServerConnection to node " << remoteId << std::endl;
        ret = new ServerConnection(id, remoteId, NULL, hosts, ports, handler, initcounter);
        connections.insert(std::make_pair(remoteId, ret));
        std::cout << "[GET_CONNECTION] Node " << id << " - ServerConnection to node " << remoteId << " created and inserted" << std::endl;
    }
    else
    {
        std::cout << "[GET_CONNECTION] Node " << id << " - Using EXISTING ServerConnection to node " << remoteId << std::endl;
        ret = connections.at(remoteId);
    }
    std::cout << "[GET_CONNECTION] Node " << id << " - Returning connection to node " << remoteId << std::endl;
    return ret;
}

// MODIFIED: Now notifies Handler about failures
void ServersCommunicationLayer::markNodeDown(int nodeId)
{
    std::cout << "[MARK_NODE_DOWN] Node " << id << " - Attempting to mark node " << nodeId << " as DOWN" << std::endl;
    bool wasAlive = node_status[nodeId].load();
    std::cout << "[MARK_NODE_DOWN] Node " << id << " - Node " << nodeId << " wasAlive: " << wasAlive << std::endl;
    
    node_status[nodeId].store(false);
    std::cout << "[MARK_NODE_DOWN] Node " << id << " - Node " << nodeId << " status set to false" << std::endl;
    
    if (wasAlive) {
        std::cout << "[" << getTimestamp() << "] [Node " << id << "] STATUS CHANGE: "
                  << "Node " << nodeId << " marked as DOWN" << std::endl;
        
        // CRITICAL: Notify Handler about the failure
        if (handler != nullptr) {
            std::cout << "[MARK_NODE_DOWN] Node " << id << " - Calling handler->setfailurenode(" << nodeId << ")" << std::endl;
            handler->setfailurenode(nodeId);
            std::cout << "[MARK_NODE_DOWN] Node " << id << " - handler->setfailurenode() completed" << std::endl;
            std::cout << "[" << getTimestamp() << "] [Node " << id 
                      << "] Notified Handler about Node " << nodeId 
                      << " failure. Alive nodes: " << getAliveNodeCount() << std::endl;
        } else {
            std::cout << "[MARK_NODE_DOWN] Node " << id << " - WARNING: handler is NULL!" << std::endl;
        }
    }
    std::cout << "[MARK_NODE_DOWN] Node " << id << " - markNodeDown complete for node " << nodeId << std::endl;
}

void ServersCommunicationLayer::markNodeUp(int nodeId)
{
    std::cout << "[MARK_NODE_UP] Node " << id << " - Attempting to mark node " << nodeId << " as UP" << std::endl;
    bool wasDown = !node_status[nodeId].load();
    std::cout << "[MARK_NODE_UP] Node " << id << " - Node " << nodeId << " wasDown: " << wasDown << std::endl;
    
    node_status[nodeId].store(true);
    std::cout << "[MARK_NODE_UP] Node " << id << " - Node " << nodeId << " status set to true" << std::endl;
    
    if (wasDown) {
        logRecovery(nodeId);
    }
    std::cout << "[MARK_NODE_UP] Node " << id << " - markNodeUp complete for node " << nodeId << std::endl;
}

bool ServersCommunicationLayer::isNodeAlive(int nodeId)
{
    bool alive = node_status[nodeId].load();
    // Too verbose for every call, comment out if needed
    // std::cout << "[IS_NODE_ALIVE] Node " << id << " - Node " << nodeId << " is " << (alive ? "ALIVE" : "DEAD") << std::endl;
    return alive;
}

// Get count of alive nodes
int ServersCommunicationLayer::getAliveNodeCount()
{
    // std::cout << "[GET_ALIVE_COUNT] Node " << id << " - Counting alive nodes" << std::endl;
    std::lock_guard<std::mutex> lock(connections_mutex);
    int count = 0;
    for (auto& pair : node_status) {
        if (pair.second.load()) {
            count++;
        }
    }
    // std::cout << "[GET_ALIVE_COUNT] Node " << id << " - Found " << count << " alive nodes" << std::endl;
    return count;
}

// Get list of alive node IDs
std::vector<int> ServersCommunicationLayer::getAliveNodes()
{
    std::cout << "[GET_ALIVE_NODES] Node " << id << " - Getting list of alive nodes" << std::endl;
    std::lock_guard<std::mutex> lock(connections_mutex);
    std::vector<int> alive;
    for (auto& pair : node_status) {
        if (pair.second.load()) {
            alive.push_back(pair.first);
        }
    }
    std::cout << "[GET_ALIVE_NODES] Node " << id << " - Found " << alive.size() << " alive nodes" << std::endl;
    return alive;
}

// broadcasts the message to others (does not send it to self)
void ServersCommunicationLayer::broadcast(string &message)
{
    std::cout << "[BROADCAST_STRING] Node " << id << " - Starting broadcast (string)" << std::endl;
    std::lock_guard<std::mutex> lock(connections_mutex);
    std::cout << "[BROADCAST_STRING] Node " << id << " - Acquired mutex, broadcasting to " << connections.size() << " connections" << std::endl;
    
    int sent_count = 0;
    int skipped_count = 0;
    int failed_count = 0;
    
    for (auto it : connections)
    {
        if (it.first == id) {
            std::cout << "[BROADCAST_STRING] Node " << id << " - Skipping self (node " << it.first << ")" << std::endl;
            skipped_count++;
            continue;  // Skip self
        }
        
        // Skip known dead nodes
        if (!isNodeAlive(it.first)) {
            std::cout << "[BROADCAST_STRING] Node " << id << " - Skipping dead node " << it.first << std::endl;
            skipped_count++;
            continue;
        }
        
        // FIX: Use it.second directly instead of calling getConnection()
        ServerConnection* conn = it.second;
        if (conn != NULL)
        {
            try
            {
                std::cout << "[BROADCAST_STRING] Node " << id << " - Sending to node " << it.first << std::endl;
                conn->send(message);  // FIXED: Use conn directly
                std::cout << "[BROADCAST_STRING] Node " << id << " - Successfully sent to node " << it.first << std::endl;
                sent_count++;
            }
            catch (Exception *e)
            {
                std::cout << "[BROADCAST_STRING] Node " << id << " - EXCEPTION sending to node " << it.first << ": " << e->getMessage() << std::endl;
                logFailure(it.first, "broadcast (string)");
                markNodeDown(it.first);
                failed_count++;
                delete e;
            }
        }
    }
    std::cout << "[BROADCAST_STRING] Node " << id << " - Broadcast complete: sent=" << sent_count 
              << ", skipped=" << skipped_count << ", failed=" << failed_count << std::endl;
}

void ServersCommunicationLayer::broadcast(Buffer *message)
{
    // std::cout << "[BROADCAST_BUFFER] Node " << id << " - Starting broadcast (buffer)" << std::endl;
    std::lock_guard<std::mutex> lock(connections_mutex);
    // std::cout << "[BROADCAST_BUFFER] Node " << id << " - Acquired mutex, broadcasting to " << connections.size() << " connections" << std::endl;
    
    int alive_count = 0;
    int dead_count = 0;
    
    for (auto it : connections)
    {
        if (it.first == id) {
            // std::cout << "[BROADCAST_BUFFER] Node " << id << " - Skipping self (node " << it.first << ")" << std::endl;
            continue;  // Skip self
        }
        
        // Skip known dead nodes
        if (!isNodeAlive(it.first)) {
            // std::cout << "[BROADCAST_BUFFER] Node " << id << " - Skipping dead node " << it.first << std::endl;
            dead_count++;
            continue;
        }
        
        // FIX: Use it.second directly instead of calling getConnection()
        ServerConnection* conn = it.second;
        if (conn != NULL)
        {
            try
            {
                // std::cout << "[BROADCAST_BUFFER] Node " << id << " - Sending to node " << it.first << std::endl;
                conn->send(message);  // FIXED: Use conn directly
                // std::cout << "[BROADCAST_BUFFER] Node " << id << " - Successfully sent to node " << it.first << std::endl;
                alive_count++;
            }
            catch (Exception *e)
            {
                // std::cout << "[BROADCAST_BUFFER] Node " << id << " - EXCEPTION sending to node " << it.first << ": " << e->getMessage() << std::endl;
                logFailure(it.first, "broadcast (buffer)");
                markNodeDown(it.first);
                dead_count++;
                delete e;
            }
        }
    }
    
    // std::cout << "[BROADCAST_BUFFER] Node " << id << " - Broadcast complete: alive=" << alive_count 
            //   << ", dead=" << dead_count << std::endl;
    
    // Log broadcast summary if there are failures
    if (dead_count > 0) {
        std::cout << "[" << getTimestamp() << "] [Node " << id << "] BROADCAST: "
                  << "Sent to " << alive_count << " nodes, "
                  << dead_count << " nodes unreachable" << std::endl;
    }
}

void ServersCommunicationLayer::establishConnection(Socket *socket, int remoteId)
{
    std::cout << "[ESTABLISH_CONNECTION] Node " << id << " - Establishing connection with remote node " << remoteId << std::endl;
    std::lock_guard<std::mutex> lock(connections_mutex);
    std::cout << "[ESTABLISH_CONNECTION] Node " << id << " - Acquired mutex" << std::endl;
    
    std::cout << "[ESTABLISH_CONNECTION] Node " << id << " - Calling reconnect for node " << remoteId << std::endl;
    connections.at(remoteId)->reconnect(socket);
    std::cout << "[ESTABLISH_CONNECTION] Node " << id << " - Reconnect complete" << std::endl;
    
    std::cout << "[ESTABLISH_CONNECTION] Node " << id << " - Marking node " << remoteId << " as UP" << std::endl;
    markNodeUp(remoteId);
    std::cout << "[ESTABLISH_CONNECTION] Node " << id << " - establishConnection complete for node " << remoteId << std::endl;
}

void ServersCommunicationLayer::run()
{
    std::cout << "[RUN] Node " << id << " - Starting run() method" << std::endl;
    std::cout << "[" << getTimestamp() << "] [Node " << id << "] Starting communication threads" << std::endl;
    
    std::cout << "[RUN] Node " << id << " - Creating accept_thread" << std::endl;
    std::thread accept_thread([this]()
    {
        std::cout << "[ACCEPT_THREAD] Node " << this->id << " - Accept thread started" << std::endl;
        std::cout << "[" << getTimestamp() << "] [Node " << this->id << "] Accept thread started" << std::endl;
        
        int accept_count = 0;
        while (running.load()) 
        {
            try 
            {
                std::cout << "[ACCEPT_THREAD] Node " << this->id << " - Waiting for incoming connection (iteration " << accept_count << ")..." << std::endl;
                TCPSocket* newSocket = static_cast<TCPSocket*>(acceptingSocket->accept());
                std::cout << "[ACCEPT_THREAD] Node " << this->id << " - Accepted a connection" << std::endl;
                
                std::cout << "[ACCEPT_THREAD] Node " << this->id << " - Getting remote ID from socket" << std::endl;
                int remoteId = std::stoi(newSocket->getString());
                std::cout << "[ACCEPT_THREAD] Node " << this->id << " - Remote ID is: " << remoteId << std::endl;
                
                std::cout << "[" << getTimestamp() << "] [Node " << this->id << "] CONNECTION: "
                          << "Accepted connection from node " << remoteId << std::endl;
                
                std::cout << "[ACCEPT_THREAD] Node " << this->id << " - Calling establishConnection for node " << remoteId << std::endl;
                establishConnection(newSocket, remoteId);
                std::cout << "[ACCEPT_THREAD] Node " << this->id << " - establishConnection complete for node " << remoteId << std::endl;
                accept_count++;
            }
            catch (Exception* e)
            {
                if (running.load())  // Only log if not shutting down
                {
                    std::cout << "[ACCEPT_THREAD] Node " << this->id << " - EXCEPTION in accept thread: " 
                              << e->getMessage() << std::endl;
                    std::cout << "[" << getTimestamp() << "] [Node " << this->id << "] Accept thread exception: " 
                              << e->getMessage() << std::endl;
                }
                delete e;
            }
        }
        
        std::cout << "[ACCEPT_THREAD] Node " << this->id << " - Accept thread terminating" << std::endl;
        std::cout << "[" << getTimestamp() << "] [Node " << this->id << "] Accept thread terminated" << std::endl;
    });

    std::cout << "[RUN] Node " << id << " - Accept thread created, calling handleAllReceives()" << std::endl;
    handleAllReceives();
    std::cout << "[RUN] Node " << id << " - handleAllReceives() returned, joining accept_thread" << std::endl;

    accept_thread.join();
    std::cout << "[RUN] Node " << id << " - accept_thread joined, run() complete" << std::endl;
}

void ServersCommunicationLayer::handleAllReceives()
{
    // std::cout << "[HANDLE_RECEIVES] Node " << id << " - Starting handleAllReceives()" << std::endl;
    // std::cout << "[" << getTimestamp() << "] [Node " << id << "] Receive loop started" << std::endl;
    
    int iteration = 0;
    while (running.load())
    {
        // if (iteration % 1000 == 0) {
        //     // std::cout << "[HANDLE_RECEIVES] Node " << id << " - Receive loop iteration " << iteration << std::endl;
        // }
        iteration++;
        
        std::lock_guard<std::mutex> lock(connections_mutex);
        
        for (auto it = connections.begin(); it != connections.end(); ++it)
        {
            int nodeId = it->first;
            ServerConnection* conn = it->second;
            
            // Skip dead nodes or self
            if (nodeId == id) {
                continue;
            }
            
            if (!isNodeAlive(nodeId)) {
                // if (iteration % 1000 == 0) {
                //     // std::cout << "[HANDLE_RECEIVES] Node " << id << " - Skipping dead node " << nodeId << std::endl;
                // }
                continue;
            }
            
            try
            {
                if (conn != nullptr)
                {
                    conn->receive();
                }
            }
            catch (Exception* e)
            {
                std::cout << "[HANDLE_RECEIVES] Node " << id << " - EXCEPTION receiving from node " << nodeId << ": " << e->getMessage() << std::endl;
                logFailure(nodeId, "receive");
                markNodeDown(nodeId);
                
                if (conn != nullptr)
                {
                    std::cout << "[HANDLE_RECEIVES] Node " << id << " - Closing socket for node " << nodeId << std::endl;
                    conn->closeSocket();
                }
                delete e;
            }
        }
        
        // Small sleep to prevent busy waiting
        std::this_thread::sleep_for(std::chrono::microseconds(10));
    }
    
    // std::cout << "[HANDLE_RECEIVES] Node " << id << " - Receive loop terminated" << std::endl;
    // std::cout << "[" << getTimestamp() << "] [Node " << id << "] Receive loop terminated" << std::endl;
}

void ServersCommunicationLayer::closeAllSockets()
{
    std::cout << "[CLOSE_SOCKETS] Node " << id << " - Starting closeAllSockets()" << std::endl;
    std::lock_guard<std::mutex> lock(connections_mutex);
    std::cout << "[CLOSE_SOCKETS] Node " << id << " - Acquired mutex" << std::endl;
    std::cout << "[" << getTimestamp() << "] [Node " << id << "] Closing all sockets" << std::endl;
    
    int closed_count = 0;
    for (auto &pair : connections)
    {
        ServerConnection *conn = pair.second;
        if (conn)
        {
            std::cout << "[CLOSE_SOCKETS] Node " << id << " - Closing socket for node " << pair.first << std::endl;
            conn->closeSocket();
            closed_count++;
        }
    }
    std::cout << "[CLOSE_SOCKETS] Node " << id << " - Closed " << closed_count << " sockets" << std::endl;
}

void ServersCommunicationLayer::shutdown()
{
    std::cout << "[SHUTDOWN] Node " << id << " - Starting shutdown()" << std::endl;
    std::cout << "[" << getTimestamp() << "] [Node " << id << "] Shutting down ServersCommunicationLayer" << std::endl;
    running.store(false);
    std::cout << "[SHUTDOWN] Node " << id << " - Set running=false" << std::endl;
    closeAllSockets();
    std::cout << "[SHUTDOWN] Node " << id << " - Shutdown complete" << std::endl;
}