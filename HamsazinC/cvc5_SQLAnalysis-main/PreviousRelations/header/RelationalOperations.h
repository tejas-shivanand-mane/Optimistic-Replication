#ifndef TPC_CPP_RELATIONALOPERATIONS_H
#define TPC_CPP_RELATIONALOPERATIONS_H


#include <vector>
#include <string>
#include "cvc5/cvc5.h"
#include <iostream>
#include "Table.h"
#include "TableInfo.h"

using namespace std;

class RelationalOperations {
private:
    cvc5::Solver &slv;

public:
    explicit RelationalOperations(cvc5::Solver &slv);

    static Table selection(Table &table, const std::string &columnName,
                           const std::string &selectOperator, const Term &value);

    static Table projection(Table &table, const std::vector<std::string> &columnName);

    static Table rename(Table &table, const std::string &originColumnName, const std::string &renamedColumnName);

    static Table innerJoin(Table &table_first, Table &table_second, const std::string &columnNameFirst,
                           const std::string &columnNameSecond);

    static Table unionTwo(Table &table_first, Table &table_second);

    static Table groupByAggregate(Table &table, const pair<std::string, std::string>& aggregation, const std::string& groupedColumnName);
};


#endif //TPC_CPP_RELATIONALOPERATIONS_H
