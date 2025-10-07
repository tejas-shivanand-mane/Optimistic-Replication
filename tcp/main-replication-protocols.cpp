#include <cstdlib>
#include <vector>
#include <chrono>
#include <thread>
#include <fstream>
#include <atomic>
#include <memory>

#include "ServersCommunicationLayer.cpp"

using namespace std;
using namespace amirmohsen;


std::atomic<int> *initcounter;

int main(int argc, char *argv[])
{
    std::vector<string> hosts;
    int id = std::stoi(argv[1]);
    initcounter = new std::atomic<int>;
    initcounter->store(0);
    
    std::cout << "id: " << id << std::endl;
    int numnodes = std::stoi(argv[2]);
    std::cout << "nodnum: " << numnodes << std::endl;
    int numop = std::stoi(argv[3]);
    std::cout << "op: " << numop << std::endl;
    int writep = std::stoi(argv[4]);
    std::cout << "writep " << writep << std::endl;
    std::string usecase = argv[5];
    std::cout << "usecase: " << usecase << std::endl;

    int oppersecond = std::stoi(argv[6]);
    std::cout << "operation rate " << oppersecond << std::endl;

    int failed_node = std::stoi(argv[7]);
    std::cout << "failed node " << failed_node << std::endl;

    int *ports = new int[numnodes];
    int baseport = 8150 + ((numnodes + 1) * numnodes);
    std::cout << "baseport: " << baseport << std::endl;
    for (int i = 0; i < numnodes; i++)
        ports[i] = baseport + i;

    bool calculate_throughput = true;
    for (int i = 8; i < (8 + numnodes); i++)
    {
        std::string hoststr = argv[i];
        std::cout << "hotsstr: " << hoststr << std::endl;
        hosts.push_back(hoststr);
    }

    std::string loc = "/home/tejas/Optimistic-Replication/wellcoordination/workload/";
    loc += std::to_string(numnodes) + "-" + std::to_string(numop) + "-" +
           std::to_string(static_cast<int>(writep));
    loc += "/" + usecase + "/";
    
    Handler *hdl = new Handler();
    std::cout << "Starting Handler"<< std::endl;

    if (usecase == "project")
    {
        // object = new Project();
    }
    
    int syn_counter = 0;
    ServersCommunicationLayer *sc = new ServersCommunicationLayer(id, hosts, ports, hdl, initcounter);
    std::cout << "Starting ServersCommunicationLayer"<< std::endl;
    sc->start();

    std::this_thread::sleep_for(std::chrono::seconds(numnodes * 3));

    Call call;
    std::vector<Call> calls;

    int call_id = 0;
    int sent = 0;
    std::string line;
    int expected_calls = 0;
    hdl->obj.readBenchFile((loc + std::to_string(id) + ".txt").c_str(), expected_calls, id, numnodes, call, calls);
    hdl->setVars(id, numnodes, expected_calls, writep);

    std::vector<uint8_t> payload_buffer;
    uint8_t *payload;
    Call send;
    bool send_flag;
    bool permiss;
    payload_buffer.resize(256);
    payload = &payload_buffer[0];

    // Begin sync phase
    std::string init = "init";
    std::vector<uint8_t> idVector(init.begin(), init.end());
    idVector.push_back('\0');
    uint8_t *id_bytes = &idVector[0];
    uint64_t id_len = idVector.size();
    Buffer *initbuff = new Buffer();
    std::string initMsg(id_bytes, id_bytes + id_len);
    initbuff->setContent(const_cast<char *>(initMsg.c_str()), id_len);
    sc->broadcast(initbuff);
    
    while (initcounter->load() != numnodes - 1);
    
    int delay = 10;
    int maxDelay = 10000;
    int ackdelay = 1000;
    int ackmaxDelay = 8000;
    std::cout << "started sending..." << std::endl;
    
    int ack_index = 0;
    int expected_call_counter = 0;
    bool wait = false;
    bool sent_acks = true;
    int clock = 0;
    int stableindex = 0;
    int test_counter = 0;
    uint64_t early_response_time_totall = 0;
    uint64_t early_start_time = 0;
    uint64_t early_end_time = 0;
    uint64_t real_end_time = 0;
    bool overwritetime = true;
    bool onetimeprint = true;
    
    uint64_t local_start = std::chrono::duration_cast<std::chrono::microseconds>(
                               std::chrono::high_resolution_clock::now().time_since_epoch())
                               .count();
    auto it = calls.begin();
    auto preit = calls.end();
    cout << "expected calls: " << expected_calls << endl;
    
    int ops_rate = oppersecond;
    int op_interval_ns = 1e9 / ops_rate;
    uint64_t next_op_time = std::chrono::duration_cast<std::chrono::nanoseconds>(
                                std::chrono::high_resolution_clock::now().time_since_epoch())
                                .count();

    // Start failure detection thread
    std::atomic<bool> running{true};
    std::thread failure_detector([&, &hdl, &adjusted_expected]() {
        // Wait for initial sync to complete before starting failure detection
        std::this_thread::sleep_for(std::chrono::seconds(15));
        
        while (running.load()) {
            std::this_thread::sleep_for(std::chrono::seconds(5));
            
            // Only check for failures if we're still actively processing operations
            // Don't check if we're near completion
            if (hdl->obj.waittobestable.load() < (adjusted_expected * 0.95)) {
                hdl->checkForFailures(10);
            }
        }
    });

#ifdef FAILURE_MODE
    int adjusted_expected = expected_calls;
    
    // If this is the failed node, adjust expectations
    if (id == failed_node) {
        adjusted_expected -= (expected_calls / numnodes) / 2;
        std::cout << "This node will fail mid-execution. Adjusted expected: " 
                  << adjusted_expected << std::endl;
    }
    
    bool failed_node_detected = false;
#else
    int adjusted_expected = expected_calls;
#endif

    while (hdl->obj.waittobestable.load() < adjusted_expected)
    {
        // Debug progress
        if (hdl->obj.waittobestable.load() % 100 == 0) {
            std::cout << "[PROGRESS] waittobestable: " << hdl->obj.waittobestable.load() 
                      << " / " << adjusted_expected << std::endl;
        }
        
#ifdef CRDT_MESSAGE_PASSING
        if (it != calls.end())
        {
            Call &req = *it;

            if (req.type != "Read")
            {
                auto length = hdl->obj.serializeCalls(req, payload);
                std::string message(payload, payload + length);
                payload_buffer.resize(length);
                payload = &payload_buffer[0];
                auto buff = std::make_unique<Buffer>();
                buff->setContent(const_cast<char *>(message.c_str()), length);
                sc->broadcast(buff.get());
                sent++;
                ++it;
                hdl->localHandler(req);
            }
            else
            {
                hdl->localHandler(req);
                ++it;
            }
        }
#endif

#if defined(OPTIMISTIC_REPLICATION) || defined(ECROS)
        sent_acks = false;
#ifndef CRDT
        if (ack_index < std::atomic_load(&hdl->obj.send_ack_call_list)->size())
        {
            auto ack_list_snapshot = std::atomic_load(&hdl->obj.send_ack_call_list);
            auto length = hdl->serializeCalls((*ack_list_snapshot)[ack_index], payload);
            std::string message(payload, payload + length);
            payload_buffer.resize(length);
            payload = &payload_buffer[0];

            auto buff = std::make_unique<Buffer>();
            buff->setContent(const_cast<char *>(message.c_str()), length);

            sc->broadcast(buff.get());
            ack_index++;
            sent_acks = true;
            ackdelay = 1000;
        }
#endif
        uint64_t now_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                              std::chrono::high_resolution_clock::now().time_since_epoch())
                              .count();
        if (it != calls.end())
        {
#ifdef FAILURE_MODE
            if (id == failed_node && std::distance(calls.begin(), it) >= calls.size() / 2)
            {
                std::cout << "Node " << id << " simulating failure at call index "
                          << std::distance(calls.begin(), it) << "/" << calls.size() << std::endl;
                
                hdl->setfailurenode(id);
                it = calls.end();
                
                std::cout << "Node " << id << " entering passive mode (receiving only)" << std::endl;
                continue;
            }
            
            if (!failed_node_detected && hdl->failed[failed_node - 1]) {
                failed_node_detected = true;
                std::cout << "Node " << id << " detected failure of node " << failed_node << std::endl;
            }
#endif

            if (preit != it)
            {
                early_start_time = std::chrono::duration_cast<std::chrono::nanoseconds>(
                                       std::chrono::high_resolution_clock::now().time_since_epoch())
                                       .count();
                next_op_time = std::max(next_op_time + op_interval_ns, now_ns);
            }
            preit = it;
            if (now_ns < next_op_time)
            {
                early_start_time = std::chrono::duration_cast<std::chrono::nanoseconds>(
                                       std::chrono::high_resolution_clock::now().time_since_epoch())
                                       .count();
                continue;
            }

            Call &req = *it;
#ifndef CRDT
            if (wait)
            {
                if (onetimeprint)
                {
                    onetimeprint = false;
                }
                if (stableindex < hdl->obj.waittobestable.load())
                {
                    wait = false;
                    onetimeprint = true;
                }
            }
#endif
            if (req.type != "Read")
            {
#if defined(OPTIMISTIC_REPLICATION)
                if (!wait)
                {
                    hdl->localHandler(req, send_flag, permiss, stableindex);

                    if (send_flag)
                    {
                        auto length = hdl->serializeCalls(req, payload);
                        std::string message(payload, payload + length);
                        payload_buffer.resize(length);
                        payload = &payload_buffer[0];
                        auto buff = std::make_unique<Buffer>();
                        buff->setContent(const_cast<char *>(message.c_str()), length);

                        sc->broadcast(buff.get());
                        sent++;
                        ++it;
                        early_response_time_totall += std::chrono::duration_cast<std::chrono::nanoseconds>(
                                                          std::chrono::high_resolution_clock::now().time_since_epoch())
                                                          .count() -
                                                      early_start_time;
                        delay = 10;
                        wait = false;
                    }
                    else if (permiss)
                    {
                        wait = true;
                        continue;
                    }
                    else
                    {
                        ++it;
                        early_response_time_totall += std::chrono::duration_cast<std::chrono::nanoseconds>(
                                                          std::chrono::high_resolution_clock::now().time_since_epoch())
                                                          .count() -
                                                      early_start_time;
                    }
                }
#endif
#if defined(ECROS)
                hdl->localHandler(req);

                auto length = hdl->serializeCalls(req, payload);
                std::string message(payload, payload + length);
                payload_buffer.resize(length);
                payload = &payload_buffer[0];
                auto buff = std::make_unique<Buffer>();
                buff->setContent(const_cast<char *>(message.c_str()), length);

                sc->broadcast(buff.get());
                sent++;
                ++it;
                early_response_time_totall += std::chrono::duration_cast<std::chrono::nanoseconds>(
                                                  std::chrono::high_resolution_clock::now().time_since_epoch())
                                                  .count() -
                                              early_start_time;
#endif
            }
            else
            {
                ++it;
                early_response_time_totall += std::chrono::duration_cast<std::chrono::nanoseconds>(
                                                  std::chrono::high_resolution_clock::now().time_since_epoch())
                                                  .count() -
                                              early_start_time;
            }
        }

#endif
        if (hdl->last_call_stable.load())
        {
            real_end_time = std::chrono::duration_cast<std::chrono::microseconds>(
                                std::chrono::high_resolution_clock::now().time_since_epoch())
                                .count();
            hdl->last_call_stable.store(false);
        }
    }

    uint64_t local_end = std::chrono::duration_cast<std::chrono::microseconds>(
                             std::chrono::high_resolution_clock::now().time_since_epoch())
                             .count();

    // Stop failure detector thread
    running.store(false);
    if (failure_detector.joinable()) {
        failure_detector.join();
    }

    std::cout << "issued " << sent << " operations" << std::endl;

    uint64_t global_end = std::chrono::duration_cast<std::chrono::microseconds>(
                              std::chrono::high_resolution_clock::now().time_since_epoch())
                              .count();

#ifdef FAILURE_MODE
    int actual_ops = numop - ((numop / numnodes) / 2);
    std::cout << "Adjusted operations for failure mode: " << actual_ops << std::endl;
#else
    int actual_ops = numop;
#endif

    if (ops_rate == 1e9)
        std::cout << "total average response time in milliseconds: "
                  << (static_cast<double>(real_end_time - local_start) / (static_cast<double>(actual_ops) / static_cast<double>(numnodes))) / 1000 << std::endl;
    
    std::cout << "early average response time in milliseconds: " << (static_cast<double>(early_response_time_totall) / (static_cast<double>(actual_ops) / static_cast<double>(numnodes))) / 1000000 << std::endl;

    if (ops_rate == 1e9)
        std::cout << "throughput: "
                  << (static_cast<double>(actual_ops) / static_cast<double>(local_end - local_start)) * 1000 << std::endl;

    std::cout << "total time for simulation = " <<  static_cast<double>(local_end - local_start) / 1000  << " milliseconds "<<std::endl;
    
    std::cout << "Failed nodes detected: " << hdl->failed_count.load() << std::endl;
    
    std::this_thread::sleep_for(std::chrono::seconds(120));
    return 0;
}