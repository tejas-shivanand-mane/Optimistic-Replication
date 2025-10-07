#ifndef TPC_CPP_RELATIONALPROVER_H
#define TPC_CPP_RELATIONALPROVER_H

#include "VisitorNodes.h"
#include "TableInfo.h"

// Define a function type for queries
class Context;
using Kwargs = std::map<string, vector<string>>;
using QueryFunction = Context (*)(Solver&, map<string, pair<Table, Term>>, const vector<string>& , const Kwargs& );

class RelationalProver {
private:
public:
    static Term referentialIntegrity(Solver& slv, map<string, pair<Table, Term>> initialState);
    static Term InvariantSufficientCheck(Solver &slv, map<string, pair<Table, Term>> initialState, map<string, pair<Table, Term>> state1);
    static Term StateCommutativeCheck(Solver &slv, const map<string, pair<Table, Term>>& state1, map<string, pair<Table, Term>> state2);
    static Term PRCommutativeCheck(Solver &slv, const map<string, pair<Table, Term>>& state1, const map<string, pair<Table, Term>>& state2);
};


#endif //TPC_CPP_RELATIONALPROVER_H
