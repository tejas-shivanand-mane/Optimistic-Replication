// #pragma once

#include <vector>
#include <chrono>

#include "Socket.cpp"
#include <atomic>

// #include "../replicated-object.hpp"
// #include "../protocol2-partialsyn.cpp"
// #define FAILURE_MODE
// #define CRDT_MESSAGE_PASSING
bool activate_timeout = true; // this is used to activate the timeout in the main-replication-protocols.cpp
#define OPTIMISTIC_REPLICATION
// #define ECROS

#ifdef CRDT_MESSAGE_PASSING
#include "../protocol0-crdt-message-passing.cpp"
#endif
#ifdef OPTIMISTIC_REPLICATION
#include "../protocol-hamsou.cpp"
#endif

#ifdef ECROS
#include "../protocol-ecros.cpp"
#endif

using namespace std;
using namespace amirmohsen;

class ServerConnection
{
private:
	class Sender : public Thread
	{
		string *message;
		Socket *socket;

	public:
		Sender(string *message, Socket *socket);
		void run();
	};

	int id;
	int remoteId;
	// bool activate_timeout;

	Socket *socket;
	// Receiver *receiver;
	Handler *handler;
// #ifdef FAILURE_MODE
// 	std::chrono::steady_clock::time_point last_recv_time;
// #endif

	// int* syn_counter; // new added
	//  Sender* sender;

	std::vector<string> hosts;
	int *ports;
	std::atomic<int> *initcounter;

	bool isToConnect();

public:
	ServerConnection(int id, int remoteId, Socket *socket, std::vector<string> hosts, int *ports, Handler *hdl, std::atomic<int> *initcounter);

	~ServerConnection();

	void createConnection();
	void send(string &message);
	void send(Buffer *buff);
	void reconnect(Socket *socket);
	void receive();
	void closeSocket();
};

ServerConnection::ServerConnection(int id, int remoteId, Socket *socket, std::vector<string> hosts, int *ports, Handler *hdl, std::atomic<int> *initcounter)
{
	this->id = id;
	this->remoteId = remoteId;
	this->hosts = hosts;
	this->ports = ports;
	this->socket = socket;
	this->handler = hdl;
	// this->activate_timeout = false;
	//  this->syn_counter = syn_counter;
	this->initcounter = initcounter;
// #ifdef FAILURE_MODE
// 	this->last_recv_time = std::chrono::steady_clock::now();
// #endif
	if (isToConnect())
		createConnection();

	// sender = new Sender();
}

ServerConnection::~ServerConnection()
{
	closeSocket();
}

void ServerConnection::send(string &message)
{
	socket->send(message);
}

void ServerConnection::send(Buffer *buff)
{
	if (socket == nullptr)
	{
		std::cout << "socket is null" << std::endl;
		// Handle the error here, for example by returning from the function
		return;
	}
	else
	{
		socket->send(to_string(buff->getSize()));
		socket->send(buff);
	}
}

void ServerConnection::createConnection()
{
	std::cout << "connecting to " << remoteId << std::endl;
	socket = new TCPSocket(const_cast<char *>(hosts.at(remoteId - 1).c_str()), ports[remoteId - 1]);
	socket->send(to_string(id));

	// receiver = new Receiver(socket, remoteId, handler, initcounter);
	// receiver->start();
}

bool ServerConnection::isToConnect()
{
	return id > remoteId;
}

void ServerConnection::closeSocket()
{
	socket->close();
}

// establishing connection only to the nodes with higher id
// connections to the nodes with lower ids have already been created
void ServerConnection::reconnect(Socket *newSocket)
{
	std::cout << "reconnecting " << "id: " << id << " remoteId: " << remoteId << std::endl;
	if (socket == NULL)
	{
		if (isToConnect())
		{
			createConnection();
		}
		else
		{
			socket = newSocket;
			// receiver = new Receiver(socket, remoteId, handler, initcounter);
			// receiver->start();
		}
	}
}

void ServerConnection::receive()
{
// #ifdef FAILURE_MODE
// 	static std::chrono::steady_clock::time_point last_print_time = std::chrono::steady_clock::now();
// 	auto now = std::chrono::steady_clock::now();
// 	if (std::chrono::duration_cast<std::chrono::seconds>(now - last_print_time).count() >= 1)
// 	{
// 		std::cout << "[HB] Connection active to node " << this->remoteId << std::endl;
// 		last_print_time = now;
// 	}
// #endif

	if (socket == nullptr)
		return;

	try
	{
		int drain_limit = 1;
		for (int i = 0; i < drain_limit; ++i)
		{
			if (!socket->isReadable(1))
				return;

			string lengthStr = socket->getString();
			if (lengthStr.empty())
				return;

			int length = std::stoi(lengthStr);
			Buffer *buff = socket->receive(length);
			if (buff == nullptr)
				break;
			cout << "checkkk222" << "this remote " << this->remoteId << endl;
			if (strcmp(buff->getContent(), "init") == 0)
			{
				std::cout << "in receive: " << buff->getContent() << std::endl;
				initcounter->fetch_add(1);
				if (initcounter->load() == handler->number_of_nodes - 1)
				{
					cout << "All nodes initialized, starting the protocol..." << endl;
					// activate_timeout = true;
				}
			}
			else
			{
				Call call;
				handler->deserializeCalls(reinterpret_cast<uint8_t *>(buff->getContent()), call);

				if (handler->number_of_nodes <= 8)
					std::this_thread::sleep_for(std::chrono::microseconds(400 / handler->number_of_nodes));

#if defined(CRDT_MESSAGE_PASSING)
				if (handler->obj.checkValidcall(call))
				{
					handler->internalDownstreamExecute(call);
				}
#elif defined(OPTIMISTIC_REPLICATION)
				// cout<<"check"<<call.type<<endl;
				if (call.type == "Ack")
				{
					handler->updateAcksTable(call);
				}
				else if (handler->obj.checkValidcall(call))
				{
					// std::lock_guard<std::mutex> lock(handler->mtx);
					handler->internalDownstreamExecute(call);
				}
#elif defined(ECROS)
				handler->internalDownstreamExecute(call);
#endif
			}
		}
	}
	catch (Exception *e)
	{

		{
			std::cout << "Node " << this->remoteId << " marked as failed due to socket closure\n";
			handler->setfailurenode(this->remoteId);
			activate_timeout = false; // Reset the flag after marking as failed
		}

		throw;


// #ifdef FAILURE_MODE
// 		// std::string msg = e->getMessage();
// 		// // if (activate_timeout)
// 		// // if (msg.find("closed the socket") != std::string::npos && activate_timeout)
// 		// {
// 		// 	std::cout << "Node " << this->remoteId << " marked as failed due to socket closure\n";
// 		// 	handler->setfailurenode(this->remoteId);
// 		// 	activate_timeout = false; // Reset the flag after marking as failed
// 		// }
// #else
// 		std::cerr << "Receive failed:" << e->getMessage() << std::endl;
// #endif
		delete e;
	}
}