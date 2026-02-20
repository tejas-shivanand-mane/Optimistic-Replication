#include <cstdlib>
#include <vector>
#include <chrono>
#include <thread>
#include <fstream>
#include <atomic>
#include <memory>

#include "ServersCommunicationLayer.cpp"
#include "config.hpp"
// #include "ServerConnection.cpp"

// #include "../wellcoordination/benchmark/project.hpp"
// #include "../protocol2-partialsyn.cpp"

using namespace std;
using namespace amirmohsen;


std::atomic<int> *initcounter;

int main(int argc, char *argv[])
{

    // std::cout << "checkkk111" << std::endl;
    std::vector<string> hosts;
    int id = std::stoi(argv[1]);
    initcounter = new std::atomic<int>;
    initcounter->store(0);
    // std::cout << "checkkk222" << std::endl;
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
        // hoststr=hoststr+".clemson.cloudlab.us";
        std::cout << "hotsstr: " << hoststr << std::endl;
        hosts.push_back(hoststr);
    }

    std::string loc =
        "/home/tejas/Optimistic-Replication/wellcoordination/workload/"; /// scratch/user/u.js213354/
    loc += std::to_string(numnodes) + "-" + std::to_string(numop) + "-" +
           std::to_string(static_cast<int>(writep));
    loc += "/" + usecase + "/";
    //////////////////////////////////////
    Handler *hdl = new Handler(); // Ensure this is properly initialized
    // object->setID(id)->setNumProcess(numnodes)->finalize();
    //  start connections
    int syn_counter = 0;
    ServersCommunicationLayer *sc = new ServersCommunicationLayer(id, hosts, ports, hdl, initcounter);
    sc->start();
    // wait for all to connect
    /*if(numnodes<=4)
        std::this_thread::sleep_for(std::chrono::seconds(10));
    else*/

    std::this_thread::sleep_for(std::chrono::seconds(numnodes * 3)); // wait for all to connect
    // std::this_thread::sleep_for(std::chrono::seconds(180));

    // new added
    Call call;
    std::vector<Call> calls; // new added

    int call_id = 0;
    int sent = 0;
    std::string line;
    int expected_calls = 0;
    hdl->obj.readBenchFile((loc + std::to_string(id) + ".txt").c_str(), expected_calls, id, numnodes, call, calls); // new added
    hdl->setVars(id, numnodes, expected_calls, writep);



    // Heartbeat Sender
    std::thread([&]() {
        while (true) {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));

            Call hb;
            hb.type     = "Heartbeat";
            hb.node_id  = id;
            hb.call_id  = 0;
            hb.value1   = 0;
            hb.stable   = false;
            hb.call_vector_clock.resize(numnodes, 0);

            uint8_t buffer[4096];
            size_t len = hdl->serializeCalls(hb, buffer);
            Buffer* buf = new Buffer(len);
            buf->setContent(reinterpret_cast<char*>(buffer), len);
            sc->broadcast(buf);
            delete buf;
        }
    }).detach();

    // Heartbeat Monitor
    std::thread([&]() {
        const int TIMEOUT_MS = 1000;
        std::this_thread::sleep_for(std::chrono::seconds(numnodes * 3 + 2)); // wait for all nodes to connect first

        while (true) {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            auto now = std::chrono::steady_clock::now();

            for (int i = 0; i < numnodes; i++) {
                if (i == id - 1) continue;

                bool already_failed;
                long long elapsed;
                {
                    std::lock_guard<std::mutex> lock(hdl->mtx_heartbeat);
                    already_failed = hdl->node_failed[i];
                    elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                        now - hdl->last_heartbeat_time[i]).count();
                }

                if (!already_failed && elapsed > TIMEOUT_MS)
                    hdl->markNodeFailed(i + 1);
            }
        }
    }).detach();








    /*
    std::ifstream myfile;
    myfile.open((loc + std::to_string(id) + ".txt").c_str());

    std::vector<MethodCall> requests;
    while (getline(myfile, line)) {
        if (line.at(0) == '#') {
        expected_calls = std::stoi(line.substr(1, line.size()));
        continue;
        }
        std::string sequence_number =
            std::to_string(id) + "-" + std::to_string(call_id++);
        MethodCall call = ReplicatedObject::createCall(sequence_number, line);
        requests.push_back(call);
    }
    */

    std::vector<uint8_t> payload_buffer;
    uint8_t *payload;
    Call send;
    bool send_flag;
    bool permiss;
    payload_buffer.resize(256);
    payload = &payload_buffer[0];

    // begin sync phase-----------------------------------------------------------
    // std::cout << "begin sync phase" << std::endl;
    std::string init = "init";
    std::vector<uint8_t> idVector(init.begin(), init.end());
    idVector.push_back('\0'); // Ensure null-termination
    uint8_t *id_bytes = &idVector[0];
    uint64_t id_len = idVector.size();
    Buffer *initbuff = new Buffer();
    std::string initMsg(id_bytes, id_bytes + id_len);
    // std::cout<<"check2222222"<<std::endl;
    initbuff->setContent(const_cast<char *>(initMsg.c_str()), id_len);
    sc->broadcast(initbuff);
    while (initcounter->load() != numnodes - 1)
        ;
    //{
    // std::cout<<initcounter->load()<<std::endl;
    // std::this_thread::sleep_for(std::chrono::seconds(2));
    //}
    // std::cout << "end sync phase" << initcounter->load() << std::endl;
    // end sync phase-------------------------------------------------------------
    int delay = 10;         // Initial delay in microseconds
    int maxDelay = 10000;   // Maximum delay in microseconds
    int ackdelay = 1000;    // Initial delay in microseconds
    int ackmaxDelay = 8000; // Maximum delay in microseconds
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
    // Project pjc;
    uint64_t local_start = std::chrono::duration_cast<std::chrono::microseconds>(
                               std::chrono::high_resolution_clock::now().time_since_epoch())
                               .count();
    auto it = calls.begin();
    auto preit = calls.end();
    cout << "expected calls: " << expected_calls << endl;
    int ops_rate = oppersecond;          // desired ops/sec  //To measure throughput set tihs to 1e9
    int op_interval_ns = 1e9 / ops_rate; // nanoseconds per op
    int lasttimeprintstableindex=-1;
    uint64_t next_op_time = std::chrono::duration_cast<std::chrono::nanoseconds>(
                                std::chrono::high_resolution_clock::now().time_since_epoch())
                                .count();


    uint64_t simul_start = std::chrono::duration_cast<std::chrono::milliseconds>(
                             std::chrono::high_resolution_clock::now().time_since_epoch())
                             .count();

            
#ifdef FAILURE_MODE
    expected_calls -= (expected_calls / numnodes) / 2; // for failure mode we need to reduce the expected calls by numnodes
#endif
    while (hdl->obj.waittobestable.load() < (expected_calls)) // hdl->obj.stable_state.index < expected_calls //hdl->obj.waittobestable.load() < expected_calls
    {

// std::this_thread::sleep_for(std::chrono::microseconds(1000));
        if(lasttimeprintstableindex!=hdl->obj.waittobestable.load()){
            cout<<hdl->obj.waittobestable.load()<<endl;
            lasttimeprintstableindex=hdl->obj.waittobestable.load();
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
            // cout<<"sent ack index=  "<< ack_index<<endl;
            // cout<<"ceckkkackack222222222222222222"<<endl;
        }
        // else {
        // std::this_thread::sleep_for(std::chrono::microseconds(100));
        //}
#endif
        uint64_t now_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                              std::chrono::high_resolution_clock::now().time_since_epoch())
                              .count();
        if (it != calls.end())
        {
#ifdef FAILURE_MODE
            if (id == failed_node && std::distance(calls.begin(), it) >= calls.size() / 2)
            {
                std::cout << "Node " << id << " simulating failure mid-execution at call index "
                          << std::distance(calls.begin(), it) << "\n";
                sc->closeAllSockets();
                break;
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
                /*
                if (onetimeprint)
                {
                    std::cout << "local wait for stable index: " << hdl->obj.waittobestable.load() << std::endl;
                    onetimeprint = false;
                }*/
                if (stableindex < hdl->obj.waittobestable.load())
                {
                    wait = false;
                    onetimeprint = true;
                    // test_counter++;
                    // std::cout << "end sync phase" << test_counter<< std::endl;
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
                        // preit = it;
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
                        // preit = it;
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
                // preit = it;
                ++it;
                early_response_time_totall += std::chrono::duration_cast<std::chrono::nanoseconds>(
                                                  std::chrono::high_resolution_clock::now().time_since_epoch())
                                                  .count() -
                                              early_start_time;
                // delay = 10;
                // wait = false;
#endif
            }
            else
            {
                // preit = it;
                ++it;
                early_response_time_totall += std::chrono::duration_cast<std::chrono::nanoseconds>(
                                                  std::chrono::high_resolution_clock::now().time_since_epoch())
                                                  .count() -
                                              early_start_time;
            }
            /*if(it==calls.end())
                early_response_time_totall += std::chrono::duration_cast<std::chrono::microseconds>(
                                                  std::chrono::high_resolution_clock::now().time_since_epoch())
                                                  .count() - local_start;*/
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

    std::cout << "issued " << sent << " operations" << std::endl;

    uint64_t global_end = std::chrono::duration_cast<std::chrono::microseconds>(
                              std::chrono::high_resolution_clock::now().time_since_epoch())
                              .count();
    if (ops_rate == 1e9)
        std::cout << "total average response time in milliseconds: "
                  << (static_cast<double>(real_end_time - local_start) / (static_cast<double>(numop) / static_cast<double>(numnodes))) / 1000 << std::endl;

    

    uint64_t simul_end = std::chrono::duration_cast<std::chrono::milliseconds>(
                             std::chrono::high_resolution_clock::now().time_since_epoch())
                             .count();

    std::cout <<"simulation time in seconds: " << (static_cast<double>(simul_end - simul_start)) / 1000 << ", numop: " << numop << std::endl;



    std::cout << "early average response time in milliseconds: " << (static_cast<double>(early_response_time_totall) / (static_cast<double>(numop) / static_cast<double>(numnodes))) / 1000000 << std::endl;
#ifdef FAILURE_MODE
    numop -= (numop / numnodes) / 2; // for failure mode we need to reduce the expected calls by numnodes
#endif
    if (ops_rate == 1e9)
        std::cout << "throughput: "
                  << (static_cast<double>(numop) / static_cast<double>(local_end - local_start)) * 1000 << std::endl;
    // object->toString();
    std::this_thread::sleep_for(std::chrono::seconds(120));
    return 0;
}
