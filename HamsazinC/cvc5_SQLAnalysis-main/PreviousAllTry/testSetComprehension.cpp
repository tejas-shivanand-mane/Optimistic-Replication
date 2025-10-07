#include <cvc5/cvc5.h>
#include <iostream>

using namespace cvc5;
using namespace std;

int old2() {
    Solver slv;

    slv.setOption("sets-ext", "true");
    // Define the element type (Integer in this case)
    Sort integerType = slv.getIntegerSort();
    Sort setSort = slv.mkSetSort(integerType);

    Term EmptySet = slv.mkEmptySet(setSort);
    Term SetTerm = slv.mkConst(setSort, "integerSet");

    // Create a bound variable to represent each element in the set
    Term boundVar = slv.mkVar(integerType, "x");
    std::vector<Term> vars = {boundVar};
    Term varList = slv.mkTerm(Kind::VARIABLE_LIST, vars);

    // Define a simple condition for the set comprehension
    Term condition = slv.mkTerm(Kind::LT, {boundVar, slv.mkInteger(5)}); // x < 5

    // Create the set comprehension
    Term setComprehension = slv.mkTerm(Kind::SET_COMPREHENSION, {varList, condition, boundVar});

    // Define a new set (for example, a set containing a single integer 3)
    Term newSet = slv.mkTerm(Kind::SET_SINGLETON, {slv.mkInteger(3)});
    Term mySet = slv.mkTerm(Kind::SET_INTER, {SetTerm, setComprehension});
    // Check if the new set is a member of the set defined by set comprehension
    Term isMember = slv.mkTerm(Kind::SET_MEMBER, {slv.mkInteger(1), mySet});

    cout << SetTerm << endl;
    cout << mySet << endl;
    cout << isMember << endl;
    // Assert the condition
    slv.assertFormula(isMember);

    // Check satisfiability
    Result result = slv.checkSat();
    std::cout << "Result is: " << result << std::endl;

    return 0;
}
