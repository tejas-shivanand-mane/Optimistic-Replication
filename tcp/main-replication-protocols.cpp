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
    // Disable output buffering for immediate logging
    std::cout.setf(std::ios::unitbuf);
    std::cerr.setf(std::ios::unitbuf);
    
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
    int ops_rate = oppersecond;
    int op_interval_ns = 1e9 / ops_rate;
    uint64_t next_op_time = std::chrono::duration_cast<std::chrono::nanoseconds>(
                                std::chrono::high_resolution_clock::now().time_since_epoch())
                                .count();

#ifdef FAILURE_MODE
    int adjusted_expected = expected_calls;
    std::cout << "Running in CRASH FAULT TOLERANCE mode" << std::endl;
#else
    int adjusted_expected = expected_calls;
#endif

    cout << "expected calls: " << expected_calls << endl;
    cout << "adjusted_expected: " << adjusted_expected << endl;
    cout << "Node " << id << " starting with " << calls.size() << " local operations" << endl;

    // Start heartbeat and failure detection threads
    std::atomic<bool> running{true};
    
    // Heartbeat sender - sends "I'm alive" every 2 seconds
    std::thread heartbeat_sender([&]() {
        while (running.load()) {
            std::this_thread::sleep_for(std::chrono::seconds(2));
            
            std::string hb = "heartbeat";
            std::vector<uint8_t> hbVector(hb.begin(), hb.end());
            hbVector.push_back('\0');
            uint8_t *hb_bytes = &hbVector[0];
            uint64_t hb_len = hbVector.size();
            Buffer *hbBuff = new Buffer();
            std::string hbMsg(hb_bytes, hb_bytes + hb_len);
            hbBuff->setContent(const_cast<char *>(hbMsg.c_str()), hb_len);
            sc->broadcast(hbBuff);
            delete hbBuff;
        }
    });
    
    // Failure detector - checks if nodes stopped sending heartbeats
    std::thread failure_detector([&]() {
        std::this_thread::sleep_for(std::chrono::seconds(10));
        
        while (running.load()) {
            std::this_thread::sleep_for(std::chrono::seconds(3));
            hdl->checkForFailures(8); // 8 second timeout = 4 missed heartbeats
        }
    });

    auto it = calls.begin();
    auto preit = calls.end();
    
    auto main_loop_start = std::chrono::steady_clock::now();

    while (hdl->obj.waittobestable.load() < adjusted_expected)
    {
        static int last_printed = 0;
        static auto last_print_time = std::chrono::steady_clock::now();
        int current = hdl->obj.waittobestable.load();
        auto now = std::chrono::steady_clock::now();
        
#ifdef FAILURE_MODE
        static int last_failed_count = 0;
        int current_failed_count = hdl->failed_count.load();
        
        // Check if a new crash was detected
        if (current_failed_count > last_failed_count) {
            // Recalculate expected based on actual operations from crashed nodes
            int actual_expected = hdl->getActualExpectedOps();
            adjusted_expected = actual_expected;
            
            std::cout << "\n========================================" << std::endl;
            std::cout << "[MAIN LOOP] Crash detected - adjusting target" << std::endl;
            std::cout << "[MAIN LOOP] Crashed nodes: " << current_failed_count << std::endl;
            std::cout << "[MAIN LOOP] Old target: " << expected_calls << std::endl;
            std::cout << "[MAIN LOOP] New target: " << adjusted_expected << std::endl;
            std::cout << "[MAIN LOOP] Current progress: " << current << std::endl;
            
            // Show status of each node
            for (int i = 0; i < numnodes; i++) {
                std::cout << "[MAIN LOOP] Node " << (i+1) << ": " 
                          << (hdl->failed[i] ? "CRASHED" : "ACTIVE") 
                          << ", ops=" << hdl->highest_call_from_node[i].load() << std::endl;
            }
            std::cout << "========================================\n" << std::endl;
            
            last_failed_count = current_failed_count;
            last_print_time = now;
            
            // Check if we've already reached the new target
            if (current >= adjusted_expected) {
                std::cout << "[COMPLETION] Reached adjusted target!" << std::endl;
                break;
            }
        }
#endif
        
        // Progress logging
        if (current % 500 == 0 && current != last_printed) {
            std::cout << "[PROGRESS] " << current << " / " << adjusted_expected 
                      << " (crashed: " << hdl->failed_count.load() << ")" << std::endl;
            last_printed = current;
            last_print_time = now;
        }
        
        // Stuck detection
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - last_print_time).count();
        if (elapsed > 10) {
            std::cout << "[STATUS] current=" << current << "/" << adjusted_expected 
                      << ", stable_index=" << hdl->obj.stable_state.index 
                      << ", crashed=" << hdl->failed_count.load() << std::endl;
            last_print_time = now;
        }
        
        // Wait if no more work to do
        if (it == calls.end() && ack_index >= std::atomic_load(&hdl->obj.send_ack_call_list)->size()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
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
        // Send any pending acknowledgments
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

                        auto ct = std::chrono::duration_cast<std::chrono::seconds>(
                        std::chrono::steady_clock::now() - main_loop_start).count();
                        std::cout << "Time: " << ct << "; ops_count: " << std::distance(calls.begin(), it) << std::endl;

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


                        auto ct = std::chrono::duration_cast<std::chrono::seconds>(
                        std::chrono::steady_clock::now() - main_loop_start).count();
                        std::cout << "Time: " << ct << "; ops_count: " << std::distance(calls.begin(), it) << std::endl;

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


                auto ct = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::steady_clock::now() - main_loop_start).count();
                std::cout << "Time: " << ct << "; ops_count: " << std::distance(calls.begin(), it) << std::endl; 
                                            
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

    // Stop background threads
    running.store(false);
    if (heartbeat_sender.joinable()) {
        heartbeat_sender.join();
    }
    if (failure_detector.joinable()) {
        failure_detector.join();
    }

    std::cout << "\n========================================" << std::endl;
    std::cout << "SIMULATION COMPLETE" << std::endl;
    std::cout << "Issued operations: " << sent << std::endl;
    std::cout << "Stabilized operations: " << hdl->obj.waittobestable.load() << std::endl;
    std::cout << "Crashed nodes: " << hdl->failed_count.load() << std::endl;

    uint64_t global_end = std::chrono::duration_cast<std::chrono::microseconds>(
                              std::chrono::high_resolution_clock::now().time_since_epoch())
                              .count();

#ifdef FAILURE_MODE
    int actual_ops = hdl->getActualExpectedOps();
    std::cout << "Actual expected (with crashes): " << actual_ops << std::endl;
#else
    int actual_ops = numop;
#endif

    if (ops_rate == 1e9)
        std::cout << "Total average response time (ms): "
                  << (static_cast<double>(real_end_time - local_start) / (static_cast<double>(actual_ops) / static_cast<double>(numnodes))) / 1000 << std::endl;
    
    std::cout << "Early average response time (ms): " 
              << (static_cast<double>(early_response_time_totall) / (static_cast<double>(actual_ops) / static_cast<double>(numnodes))) / 1000000 << std::endl;

    if (ops_rate == 1e9)
        std::cout << "Throughput (ops/ms): "
                  << (static_cast<double>(actual_ops) / static_cast<double>(local_end - local_start)) * 1000 << std::endl;

    std::cout << "Total simulation time (ms): " 
              << static_cast<double>(local_end - local_start) / 1000 << std::endl;
    std::cout << "========================================\n" << std::endl;
    
    std::this_thread::sleep_for(std::chrono::seconds(5));
    return 0;
}