#ifndef TABLE_INFO_H
#define TABLE_INFO_H

#include <cvc5/cvc5.h>
#include <string>
#include <vector>

using namespace cvc5;
using namespace std;

class TableInfo {
public:
    static vector<pair<string, cvc5::Sort>> getColumnInfo(const string& tableName);
    static vector<pair<vector<string>, vector<string>>> getReferentialIntegrityInfo(const string& tableName);
};

#endif // TABLE_INFO_H
