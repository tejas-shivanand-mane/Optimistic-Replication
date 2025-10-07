#include "../wellcoordination/benchmark/crdt-counter.hpp"
#include "../wellcoordination/benchmark/crdt-gset.hpp"
#include "../wellcoordination/benchmark/crdt-reg.hpp"

#define COUNTER //COUNTER //GSET //REG

class Handler
{
public:
#ifdef COUNTER
    Counter obj;
#endif

#ifdef GSET
    Gset obj;
#endif

#ifdef REG
    Register obj;
#endif

    

public:

    int node_id;
    int number_of_nodes;

    void setVars(int id, int num_nodes)
    {
        node_id = id;
        number_of_nodes = num_nodes;
    }
    
    void localHandler(Call &call)
    {
        obj.updateLocal(call);
    }

    

    void internalDownstreamExecute(Call call)
    {
        obj.updateRemote(call);
    }
};
