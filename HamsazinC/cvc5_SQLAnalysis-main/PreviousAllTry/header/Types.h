#ifndef TPC_CPP_TYPES_H
#define TPC_CPP_TYPES_H

#include <vector>
#include <variant>
#include <string>
#include "cvc5/cvc5.h"
#include "Table.h"

using RowData = std::vector<std::variant<int64_t, double, std::string>>;

using namespace std;
using namespace cvc5;


#endif //TPC_CPP_TYPES_H
