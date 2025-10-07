#ifndef TABLE_INFO_H
#define TABLE_INFO_H

#include <cvc5/cvc5.h>
#include <string>
#include <vector>

class TableInfo {
public:
    static std::vector<std::pair<std::string, cvc5::Sort>> getColumnInfo(const std::string& tableName);
};

#endif // TABLE_INFO_H
