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
    std::cout << "Starting Handler" << std::endl;

    if (usecase == "project")
    {
        // object = new Project();
    }
    
    int syn_counter = 0;
    ServersCommunicationLayer *sc = new ServersCommunicationLayer(id, hosts, ports, hdl, initcounter);
    std::cout << "Starting ServersCommunicationLayer" << std::endl;
    sc->start();
    
    std::this_thread::sleep_for(std::chrono::seconds(numnodes * 3)); // wait for all to connect

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

    // begin sync phase-----------------------------------------------------------
    std::cout << "begin sync phase" << std::endl;
    std::string init = "init";
    std::vector<uint8_t> idVector(init.begin(), init.end());
    idVector.push_back('\0');
    uint8_t *id_bytes = &idVector[0];
    uint64_t id_len = idVector.size();
    Buffer *initbuff = new Buffer();
    std::string initMsg(id_bytes, id_bytes + id_len);
    initbuff->setContent(const_cast<char *>(initMsg.c_str()), id_len);
    sc->broadcast(initbuff);
    while (initcounter->load() != numnodes - 1)
        ;
    std::cout << "end sync phase" << initcounter->load() << std::endl;
    // end sync phase-------------------------------------------------------------
    
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

    // Timeout and dynamic expected calls handling
    uint64_t loop_start_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    const uint64_t MAX_WAIT_MS = 180000; // 3 minutes timeout







    // Helper function for dynamic expected calls
    auto getAdjustedExpectedCalls = [&]() -> int {
    #ifdef FAILURE_MODE
        int alive_nodes = sc->getAliveNodeCount();
        // expected_calls = 720 total writes
        // If a node fails, we lose its writes
        int writes_per_node = expected_calls / numnodes;  // 720 / 4 = 180
        int adjusted = writes_per_node * alive_nodes;     // 180 * 4 = 720 (or less if nodes fail)
        return adjusted;
    #else
        return expected_calls;  // 720
    #endif
    };

#ifdef FAILURE_MODE
    std::cout << "[FAILURE_MODE] Initial expected_calls: " << expected_calls 
              << ", will adjust dynamically based on alive nodes" << std::endl;
#endif

    // Progress tracking
    int last_logged_stable = -1;





    std::cout << "[DEBUG] Node " << id << " starting main loop:" << std::endl;
    std::cout << "  - calls.size() = " << calls.size() << std::endl;
    std::cout << "  - numop = " << numop << std::endl;
    std::cout << "  - numnodes = " << numnodes << std::endl;
    std::cout << "  - expected_calls = " << expected_calls << std::endl;
    std::cout << "  - getAdjustedExpectedCalls() = " << getAdjustedExpectedCalls() << std::endl;
    std::cout << "  - Initial waittobestable = " << hdl->obj.waittobestable.load() << std::endl;








    
    // Main loop with dynamic termination condition
    while (hdl->obj.waittobestable.load() < getAdjustedExpectedCalls())
    {
        // Timeout check
        uint64_t current_time = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch()).count();
        
        if (current_time - loop_start_time > MAX_WAIT_MS) {
            std::cout << "[TIMEOUT] Node " << id << " exiting after " 
                      << (current_time - loop_start_time) / 1000 << " seconds" << std::endl;
            std::cout << "[TIMEOUT] Expected: " << getAdjustedExpectedCalls() 
                      << ", Stable: " << hdl->obj.waittobestable.load() << std::endl;
            break;
        }
        
        // Progress logging every 1000 stable operations
        int current_stable = hdl->obj.waittobestable.load();
        if (current_stable > 0 && current_stable / 1000 > last_logged_stable / 1000) {
            std::cout << "[PROGRESS] Node " << id << " - Stable: " << current_stable 
                      << "/" << getAdjustedExpectedCalls() 
                      << " (Alive nodes: " << sc->getAliveNodeCount() << ")" << std::endl;
            last_logged_stable = current_stable;
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



            // ADD THIS DEBUG BLOCK
            static int debug_counter = 0;
            static int last_it_pos = 0;
            int current_it_pos = std::distance(calls.begin(), it);
            
            if (debug_counter % 1000 == 0 || current_it_pos != last_it_pos) {
                std::cout << "[IT_DEBUG] Node " << id 
                        << " it_pos=" << current_it_pos 
                        << "/" << calls.size()
                        << " wait=" << wait 
                        << " stableindex=" << stableindex
                        << " waittobestable=" << hdl->obj.waittobestable.load()
                        << std::endl;
            }
            last_it_pos = current_it_pos;
            debug_counter++;
            // END DEBUG BLOCK

            // Manual failure simulation removed - real failures are detected automatically

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



                    // ADD THIS BEFORE localHandler
                    static int handler_calls = 0;
                    std::cout << "[HANDLER_CALL " << ++handler_calls << "] Node " << id 
                            << " calling localHandler for it_pos=" 
                            << std::distance(calls.begin(), it) << std::endl;
                    
                    hdl->localHandler(req, send_flag, permiss, stableindex);
                    
                    // ADD THIS AFTER localHandler
                    std::cout << "[HANDLER_RESULT] send_flag=" << send_flag 
                            << " permiss=" << permiss << std::endl;


                    
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

    // Final status logging
    std::cout << "[COMPLETE] Node " << id << " finished with " 
              << hdl->obj.waittobestable.load() << " stable operations out of " 
              << getAdjustedExpectedCalls() << " expected" << std::endl;
    std::cout << "[COMPLETE] Alive nodes at completion: " << sc->getAliveNodeCount() << std::endl;

    uint64_t local_end = std::chrono::duration_cast<std::chrono::microseconds>(
                             std::chrono::high_resolution_clock::now().time_since_epoch())
                             .count();

    std::cout << "issued " << sent << " operations" << std::endl;

    // Calculate adjusted numop for failures
    int adjusted_numop = numop;
#ifdef FAILURE_MODE
    int alive_nodes = sc->getAliveNodeCount();
    adjusted_numop = (numop / numnodes) * alive_nodes;
    std::cout << "Adjusted operations for " << alive_nodes << " alive nodes: " 
              << adjusted_numop << std::endl;
#endif

    uint64_t global_end = std::chrono::duration_cast<std::chrono::microseconds>(
                              std::chrono::high_resolution_clock::now().time_since_epoch())
                              .count();
    
    // Use adjusted_numop instead of numop
    if (ops_rate == 1e9)
        std::cout << "total average response time in milliseconds: "
                  << (static_cast<double>(real_end_time - local_start) / 
                      (static_cast<double>(adjusted_numop) / static_cast<double>(numnodes))) / 1000 
                  << std::endl;
    
    std::cout << "early average response time in milliseconds: " 
              << (static_cast<double>(early_response_time_totall) / 
                  (static_cast<double>(adjusted_numop) / static_cast<double>(numnodes))) / 1000000 
              << std::endl;

    if (ops_rate == 1e9)
        std::cout << "throughput: "
                  << (static_cast<double>(adjusted_numop) / 
                      static_cast<double>(local_end - local_start)) * 1000000 
                  << " ops/sec" << std::endl;

    std::cout << "total time for simulation = " 
              << static_cast<double>(local_end - local_start) / 1000  
              << " milliseconds" << std::endl;
    
    std::this_thread::sleep_for(std::chrono::seconds(120));
    return 0;
}