#include <cvc5/cvc5.h>
#include <iostream>

using namespace cvc5;
using namespace std;

int old3() {
    Solver slv;

    // Define the element type (Integer in this case)
    Sort integerType = slv.getIntegerSort();
    Sort setSort = slv.mkSetSort(integerType);

    // Define a bound variable to represent each element in the set
    Term element = slv.mkVar(integerType, "x");
    std::vector<Term> vars = {element};
    Term varList = slv.mkTerm(Kind::VARIABLE_LIST, vars);

    // Define a set variable
    Term mySet = slv.mkVar(setSort, "mySet");

    // Define the property: for all elements in mySet, the element is less than 5
    Term condition = slv.mkTerm(Kind::LT, {element, slv.mkInteger(5)});
    Term forall = slv.mkTerm(Kind::FORALL, {varList, slv.mkTerm(Kind::SET_MEMBER, {element, mySet}).impTerm(condition)});

    // Assert the condition
    cout << forall << endl;
    Term newSet = slv.mkTerm(Kind::SET_SINGLETON, {slv.mkInteger(3)});

    Term isMember = slv.mkTerm(Kind::SET_MEMBER, {newSet, forall});
    slv.assertFormula(isMember);


    // Check satisfiability
    Result result = slv.checkSat();
    std::cout << "Result is: " << result << std::endl;

    return 0;
}
