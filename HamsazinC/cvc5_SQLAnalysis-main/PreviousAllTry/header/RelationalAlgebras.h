#ifndef TPC_CPP_RELATIONALALGEBRAS_H
#define TPC_CPP_RELATIONALALGEBRAS_H

#include "cvc5/cvc5.h"
#include <iostream>
#include "Table.h"
#include "Types.h"

using namespace cvc5;

struct ProjectionReturn {
    Term setComprehension;
    Table newTable;
};

class RelationalAlgebras {
private:
    static Term selectPredicate(Solver &slv, Table table, const Term &boundVar, const std::string &columnName,
                                std::variant<std::string, int>(value), const std::string &selectOperation);

    static Term TransformRow(Solver &slv, const Term &originalRow, Table originalTable, Table newTable, const std::vector<std::string> &desiredColumns);

public:
    static Term Insert(Solver &slv, const Table &table, const Term &tableSet, const Term &newRowSet);

    static Term Select(Solver &slv, Table table, const Term &tableSet, const std::string &columnName,
                       const std::variant<std::string, int>& value, const std::string &selectOperation);

    static ProjectionReturn Project(Solver &slv, Table originalTable, const Term &originalSet, const std::vector<std::string> &desiredColumns);
    static ProjectionReturn Product(Solver &slv, Table tableA, Table tableB, const Term &setA, const Term &setB);
};


#endif //TPC_CPP_RELATIONALALGEBRAS_H
