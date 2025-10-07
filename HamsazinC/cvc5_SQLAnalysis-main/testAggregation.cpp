#include <cvc5/cvc5.h>
#include <vector>
#include <iostream>
#include "header/Table.h"

using namespace cvc5;
using namespace std;

Term Aggregation(Solver &slv, const Table& table, const Term &tableTerm, uint32_t columnIndex) {
    Sort intSort = slv.getIntegerSort();
    Sort tupleSort = tableTerm.getSort().getBagElementSort();

    // Define the types for the lambda parameters (tuple and integer)
    std::vector<Sort> lambdaParamSorts = {tupleSort, intSort};
    std::vector<uint32_t> indices = {columnIndex};

    // Create the variable for the tuple and the running sum
    Term tupleVar = slv.mkVar(tupleSort, "tupleVar");
    Term sumVar = slv.mkVar(intSort, "sumVar");

    // Create the TABLE_PROJECT operator
    Op tableProjectOp = slv.mkOp(Kind::TABLE_PROJECT, indices);
    // Create the term for table projection
    Term projectTerm = slv.mkTerm(tableProjectOp, {tableTerm});
    Term element = slv.mkVar(projectTerm.getSort(), "element");

    cout << "WTF" << endl;
    // Create the lambda expression for summation
    Term sumExpr = slv.mkTerm(Kind::ADD, {sumVar, element});
    Term sumLambda = slv.mkTerm(Kind::LAMBDA, {slv.mkTerm(Kind::VARIABLE_LIST, {tupleVar, sumVar}), sumExpr});

    // Initial value for sum
    Term zero = slv.mkInteger(0);

    // Define the aggregate operation with the index of the column to sum
    Op aggregateOp = slv.mkOp(Kind::TABLE_AGGREGATE, indices);

    // Create the aggregation term with the lambda function
    Term aggregateTerm = slv.mkTerm(aggregateOp, {sumLambda, zero, tableTerm});
    return aggregateTerm;
}




int temp5() {
    Solver slv;
    slv.setLogic("HO_ALL"); // Set the logic
    slv.setOption("produce-models", "true");
    slv.setOption("output-language", "smt2");

    Table addressTable(slv, "address");
    Table zipCodeTable(slv, "zip_code");

    Term addressTableTerm = addressTable.createBag();
    Term zipCodeTableTerm = zipCodeTable.createBag();

    // Example usage
    // Assuming you have a tableTerm representing your table and a column index to sum
    Term tableTerm = addressTableTerm; // This should be your table term
    uint32_t columnIndex = 0; // Index of the column to sum
    Sort columnSort = slv.getIntegerSort(); // Assuming the column is of integer type

    // Create the sum aggregation
    Term sumAggregate = Aggregation(slv, addressTable, tableTerm, columnIndex);

    Term expectedSum = slv.mkInteger(-2); // Adjust this value based on your test data
    Sort tempSum = slv.mkBagSort(slv.getIntegerSort());
    Term tempSumValue = slv.mkConst(tempSum, "tempSumValue");
    Term sumEquality = slv.mkTerm(Kind::EQUAL, {sumAggregate, tempSumValue});

    Sort intSort = slv.getIntegerSort();
    Term element = slv.mkVar(intSort, "element");
    Term elementBoud = slv.mkTerm(Kind::VARIABLE_LIST, {element});

    // Create a predicate that states element is in the bag and greater than 10
    Term isInBagAndGreaterThan10 = slv.mkTerm(Kind::AND,
                                              {slv.mkTerm(Kind::BAG_MEMBER, {element, tempSumValue}),
                                               slv.mkTerm(Kind::GT, {element, slv.mkInteger(1000)})});

    // Create a universal quantifier to assert this for all elements
    Term forall = slv.mkTerm(Kind::FORALL, {elementBoud, isInBagAndGreaterThan10});

// Assert the quantified formula
    slv.assertFormula(forall);
    // Assert the formula
    slv.assertFormula(sumEquality);
    // Check satisfiability
    Result result = slv.checkSat();
    if (result.isSat()) {
        cout << "The formula is satisfiable. The sum can be " << slv.getValue(tableTerm) <<  "." << endl;
    } else if (not result.isSat()) {
        cout << "The formula is unsatisfiable." << endl;
    } else {
        cout << "The satisfiability of the formula is unknown." << endl;
    }


    return 0;
}
