#ifndef TPC_CPP_RELATIONALPROVER_H
#define TPC_CPP_RELATIONALPROVER_H

#include "cvc5/cvc5.h"
#include "Table.h"
#include "Types.h"
#include "RelationalAlgebras.h"


class RelationalProver {
public:
    static Term stateOfInsertOverInsert(Solver &slv, Table tableClass, Term table, RowData row11, RowData row2);
};


#endif //TPC_CPP_RELATIONALPROVER_H
