#ifndef TPC_CPP_SQLQUERIES_H
#define TPC_CPP_SQLQUERIES_H

#include "cvc5/cvc5.h"
#include "Table.h"
#include "VisitorNodes.h"
#include "RelationalProver.h"

using namespace cvc5;
using Kwargs = std::map<string, vector<string>>;

class SqlQueries {
public:
    //// Selection on Projection: SelectPredicate(ProjectColumns(Ta))
    static Context Query1(Solver& slv, map<string, pair<Table, Term>> initialState, const vector<string>& tableNames, const Kwargs& kwargs);

    //// Projection on Selection: ProjectColumns(SelectPredicate(Ta))
    static Context Query2(Solver& slv, map<string, pair<Table, Term>> initialState, const vector<string>& tableNames, const Kwargs& kwargs);

    //// Projection: ProjectColumns(Ta)
    static Context Project(Solver& slv, map<string, pair<Table, Term>> initialState, const vector<string>& tableNames, const Kwargs& kwargs);

    //// Selection: selectPredicate(Ta)
    static Context Select(Solver& slv, map<string, pair<Table, Term>> initialState, const vector<string>& tableNames, const Kwargs& kwargs);

    //// Product: Product(Ta, Tb)
    static Context Product(Solver& slv, map<string, pair<Table, Term>> initialState, const vector<string>& tableNames, const Kwargs& kwargs);

    //// Insert: InsertRow(Row -> Ta)
    static Context Insert(Solver& slv, map<string, pair<Table, Term>> initialState, const vector<string>& tableNames, const Kwargs& kwargs);

    //// Delete: DeleteRow(Row -> Ta)
    static Context Delete(Solver& slv, map<string, pair<Table, Term>> initialState, const vector<string>& tableNames, const Kwargs& kwargs);

    //// Update: Update(updateRow(col, val), ProjectColumns(selectPredicate(Ta)))
    static Context Update(Solver& slv, map<string, pair<Table, Term>> initialState, const vector<string>& tableNames, const Kwargs& kwargs);

    //// Join Then Update: Update(updateRow(col, val), ProjectColumns(selectPredicate(Ta)))
    static Context JoinAndUpdate(Solver& slv, map<string, pair<Table, Term>> initialState, const vector<string>& tableNames, const Kwargs& kwargs);
};


#endif //TPC_CPP_SQLQUERIES_H
