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
    // For real node killing test: expect full operations initially
    // System will detect failure and continue with reduced operations
    int adjusted_expected = expected_calls;
    
    // NOTE: When killing a node externally, we don't know when it will die
    // So start with full expectations and let failure detection handle it
    std::cout << "Running in FAILURE_MODE - will handle node failures dynamically" << std::endl;
    
    bool failed_node_detected = false;
#else
    int adjusted_expected = expected_calls;
#endif

    cout << "expected calls: " << expected_calls << endl;
    cout << "adjusted_expected: " << adjusted_expected << endl;
    cout << "Node " << id << " starting with " << calls.size() << " local operations" << endl;
    cout << "failed_node parameter: " << failed_node << endl;
    
#ifdef FAILURE_MODE
    if (id == failed_node) {
        cout << "*** THIS NODE (" << id << ") IS DESIGNATED TO FAIL AT 50% ***" << endl;
    }
#endif

    cout << "expected calls: " << expected_calls << endl;
    cout << "adjusted_expected: " << adjusted_expected << endl;
    cout << "Node " << id << " starting with " << calls.size() << " local operations" << endl;

    // Start failure detection thread
    std::atomic<bool> running{true};
    std::thread failure_detector([&]() {
        // Wait for initial sync to complete before starting failure detection
        std::this_thread::sleep_for(std::chrono::seconds(15));
        
        while (running.load()) {
            std::this_thread::sleep_for(std::chrono::seconds(5));
            
#ifdef FAILURE_MODE
            // Only check for failures if we're still actively processing operations
            // Don't check if we're near completion
            if (hdl->obj.waittobestable.load() < (adjusted_expected * 0.95)) {
                hdl->checkForFailures(10);
            }
#else
            hdl->checkForFailures(10);
#endif
        }
    });

    auto it = calls.begin();
    auto preit = calls.end();
    
    auto main_loop_start = std::chrono::steady_clock::now();
    const int MAX_LOOP_TIME_SECONDS = 60; // Maximum time to wait

    while (hdl->obj.waittobestable.load() < adjusted_expected)
    {
        std::cout<< "Hello T" << std::endl;
        // Check for timeout
        auto elapsed_total = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now() - main_loop_start).count();
        
        if (elapsed_total > MAX_LOOP_TIME_SECONDS) {
            std::cout << "[TIMEOUT] Main loop exceeded " << MAX_LOOP_TIME_SECONDS 
                      << " seconds. Current: " << hdl->obj.waittobestable.load() 
                      << "/" << adjusted_expected 
                      << ", Failed nodes: " << hdl->failed_count.load() << std::endl;
            std::cout.flush();
            break;
        }
        
        // Debug progress
        static int last_printed = 0;
        static auto last_print_time = std::chrono::steady_clock::now();
        static int stuck_counter = 0;
        int current = hdl->obj.waittobestable.load();
        auto now = std::chrono::steady_clock::now();
        
#ifdef FAILURE_MODE
        // Dynamically adjust expectations when failures are detected
        static int last_failed_count = 0;
        int current_failed_count = hdl->failed_count.load();
        if (current_failed_count > last_failed_count) {
            // A node failed - we don't know how many operations it completed
            // Assume it completed roughly half (worst case for mid-execution failure)
            int ops_per_node = expected_calls / numnodes;
            int estimated_lost_ops = ops_per_node / 2;
            adjusted_expected -= estimated_lost_ops;
            
            std::cout << "[DYNAMIC ADJUST] Node failure detected. New adjusted_expected: " 
                      << adjusted_expected << " (reduced by " << estimated_lost_ops << ")" << std::endl;
            std::cout.flush();
            last_failed_count = current_failed_count;
            last_print_time = now; // Reset timer after adjustment
            stuck_counter = 0;
            
            // Check if we've already passed the adjusted target
            if (current >= adjusted_expected) {
                std::cout << "[IMMEDIATE EXIT] Already past adjusted target. Current: " 
                          << current << ", Target: " << adjusted_expected << std::endl;
                std::cout.flush();
                break;
            }
        }
        
        // Emergency exit: if no progress for 3 consecutive checks with failures detected
        static int last_progress = 0;
        if (current == last_progress && current_failed_count > 0) {
            stuck_counter++;
            if (stuck_counter >= 3) {
                std::cout << "[EMERGENCY EXIT] No progress for " << (stuck_counter * 5) 
                          << " seconds with " << current_failed_count << " failed nodes. "
                          << "Completing at " << current << "/" << adjusted_expected << std::endl;
                std::cout.flush();
                break;
            }
        } else {
            stuck_counter = 0;
        }
        last_progress = current;
#endif
        
        if (current % 100 == 0 && current != last_printed) {
            std::cout << "[PROGRESS] waittobestable: " << current 
                      << " / " << adjusted_expected 
                      << " (failed: " << hdl->failed_count.load() << ")" << std::endl;
            last_printed = current;
            last_print_time = now;
        }
        
        // If stuck for more than 10 seconds, print debug info
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - last_print_time).count();
        if (elapsed > 10) {
            std::cout << "[STUCK] waittobestable: " << current << " / " << adjusted_expected 
                      << ", stable_index: " << hdl->obj.stable_state.index 
                      << ", execution_list_size: " << hdl->executionList.size() 
                      << ", failed_nodes: " << hdl->failed_count.load() << std::endl;
            std::cout.flush();
            
#ifdef FAILURE_MODE
            // If we've detected failures and been stuck, adjust and possibly exit
            if (hdl->failed_count.load() > 0) {
                // Calculate how close we are to realistic completion
                int ops_per_node = expected_calls / numnodes;
                int max_missing = ops_per_node * hdl->failed_count.load();
                int realistic_target = expected_calls - max_missing;
                
                std::cout << "[ANALYSIS] Realistic target with failures: " << realistic_target 
                          << ", current: " << current << std::endl;
                
                if (current >= realistic_target * 0.95) {
                    std::cout << "[AUTO-COMPLETE] Within 95% of realistic target. Completing..." << std::endl;
                    break;
                }
                
                // Adjust expectations if not already done
                if (adjusted_expected > realistic_target) {
                    adjusted_expected = realistic_target;
                    std::cout << "[FORCED ADJUST] Adjusting to realistic target: " << adjusted_expected << std::endl;
                }
            }
#endif
            last_print_time = now;
        }
        
        // Small sleep to prevent busy-waiting when no work to do
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
            // COMMENT OUT simulated failure for testing real kills
            /*
            // Debug: check progress toward failure point
            if (id == failed_node && std::distance(calls.begin(), it) % 50 == 0) {
                cout << "[FAILURE CHECK] Progress: " << std::distance(calls.begin(), it) 
                     << " / " << calls.size() << " (will fail at " << (calls.size() / 2) << ")" << endl;
            }
            
            if (id == failed_node && std::distance(calls.begin(), it) >= calls.size() / 2)
            {
                std::cout << "Node " << id << " simulating failure at call index "
                          << std::distance(calls.begin(), it) << "/" << calls.size() << std::endl;
                
                hdl->setfailurenode(id);
                it = calls.end();
                
                std::cout << "Node " << id << " entering passive mode (receiving only)" << std::endl;
                continue;
            }
            */
            
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