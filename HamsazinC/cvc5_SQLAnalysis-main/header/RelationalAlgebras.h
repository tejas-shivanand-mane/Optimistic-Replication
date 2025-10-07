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

public:
    static Term Insert(Solver &slv, Table table, const Term &tableSet, const RowData &newRow);
    static Term Delete(Solver &slv, Table table, const Term &tableSet, const RowData &rowToDelete);
    static Term Update(Solver &slv, Table table, const Term &tableSet, const std::string &columnName,
                       const std::variant<std::string, int> &value, const std::string &selectOperation, const pair<string, variant<std::string, int>> &updateValue);

    static Term Select(Solver &slv, Table table, const Term &tableSet, const std::string &columnName,
                       const std::variant<std::string, int>& value, const std::string &selectOperation);

    static ProjectionReturn Project(Solver &slv, Table originalTable, const Term &originalSet, const std::vector<std::string> &desiredColumns);
    static ProjectionReturn Product(Solver &slv, Table tableA, Table tableB, const Term &setA, const Term &setB, string columTableA, string columTableB);
    static Term Aggregation(Solver &slv, const Table& table, const Term &tableTerm, uint32_t columnIndex);

    static ProjectionReturn Product2(Solver &slv, Table tableA, Table tableB, const Term &setA, const Term &setB);
};


#endif //TPC_CPP_RELATIONALALGEBRAS_H
