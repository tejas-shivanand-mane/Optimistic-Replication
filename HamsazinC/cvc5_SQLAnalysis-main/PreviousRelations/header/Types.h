#ifndef TPC_CPP_TYPES_H
#define TPC_CPP_TYPES_H

#include <vector>
#include <variant>
#include <string>

using RowData = std::vector<std::variant<int64_t, double, std::string>>;

#endif //TPC_CPP_TYPES_H
