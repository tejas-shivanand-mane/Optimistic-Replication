#include "../header/RelationalOperations.h"

RelationalOperations::RelationalOperations(cvc5::Solver &slv) : slv(slv) {}

Table RelationalOperations::selection(Table &table, const std::string &columnName,
                                      const std::string &selectOperator, const Term &value) {
    std::vector<cvc5::Term> selectedRows;
    std::vector<cvc5::Term> rows = table.getRows();

    std::vector<std::string> columnNamesVector(1, columnName);
    int columnIndex = table.getColumnIndices(columnNamesVector)[0];
    for (const auto &row: rows) {
        // Evaluate the row against the predicate.
        if (selectOperator == "lt") {
            if (row[columnIndex + 1] < value) { selectedRows.push_back(row); }
        } else if (selectOperator == "lte") {
            if (row[columnIndex + 1] <= value) { selectedRows.push_back(row); }
        } else if (selectOperator == "gt") {
            if (row[columnIndex + 1] > value) { selectedRows.push_back(row); }
        } else if (selectOperator == "gte") {
            if (row[columnIndex + 1] >= value) { selectedRows.push_back(row); }
        } else if (selectOperator == "eq") {
            if (row[columnIndex + 1] == value) { selectedRows.push_back(row); }
        }
    }
    table.resetRows(selectedRows);
    return table;
}

Table RelationalOperations::projection(Table &table, const std::vector<std::string> &columnNames) {
    // Retrieve the original table's name and solver
    Solver &slv = table.getSolver();

    // Access the current table's datatype and constructor
    const Datatype &currentDatatype = table.getTableSort().getDatatype();
    const DatatypeConstructor &currentConstructor = currentDatatype[0];

    // Create a new table name for the subset
    std::string subTableName = table.getTableName() + "_sub";

    // Start defining a new datatype for the sub-table
    DatatypeDecl subTableDatatype = slv.mkDatatypeDecl(subTableName);
    DatatypeConstructorDecl subTableConstructor = slv.mkDatatypeConstructorDecl("make-" + subTableName);

    // Map column names to their indices for sorting
    std::map<size_t, std::pair<std::string, Sort>> indexToColumnMap;
    for (const auto &columnName: columnNames) {
        for (size_t i = 0; i < currentConstructor.getNumSelectors(); ++i) {
            if (currentConstructor[i].getName() == columnName) {
                indexToColumnMap[i] = {columnName, currentConstructor[i].getCodomainSort()};
                break;
            }
        }
    }

    // Add only the selected columns to the new table's constructor in original order
    for (const auto &[index, column]: indexToColumnMap) {
        subTableConstructor.addSelector(column.first, column.second);
    }

    // Add the constructor to the datatype and create the datatype sort
    subTableDatatype.addConstructor(subTableConstructor);
    Sort subTableSort = slv.mkDatatypeSort(subTableDatatype);

    // Reset the table's datatype
    table.resetDatatype(subTableSort);

    // Prepare new rows for the modified table
    std::vector<Term> newRows;

    // Iterate over the original rows to create new rows for the modified table
    for (const auto &row: table.getRows()) {
        std::vector<Term> termArguments;
        termArguments.reserve(indexToColumnMap.size() + 1);

        // Get the constructor term for the new datatype
        DatatypeConstructor subTableCons = subTableSort.getDatatype()[0];
        Term makeSubTableTerm = subTableCons.getTerm();
        termArguments.push_back(makeSubTableTerm);

        // Collect the data from the selected columns in original order and build new terms
        for (const auto &[index, column]: indexToColumnMap) {
            Term dataTerm = row[index + 1]; // +1 because the 0th index is the constructor
            termArguments.push_back(dataTerm);
        }

        // Create a new row term and add it to the newRows
        Term newRow = slv.mkTerm(Kind::APPLY_CONSTRUCTOR, termArguments);
        newRows.push_back(newRow);
    }

    // Reset the table's rows with the new subset
    table.resetRows(newRows);
    return table;
}


Table RelationalOperations::rename(Table &table, const string &originColumnName, const string &renamedColumnName) {
    // Retrieve the solver from the table
    Solver &slv = table.getSolver();

    // Retrieve the current datatype of the table
    const Datatype &currentDatatype = table.getTableSort().getDatatype();
    const DatatypeConstructor &constructor = currentDatatype[0];

    // Start defining a new datatype for the table with the renamed column
    DatatypeDecl renamedTableDatatype = slv.mkDatatypeDecl(table.getTableName());
    DatatypeConstructorDecl renamedTableConstructor = slv.mkDatatypeConstructorDecl("make-" + table.getTableName());

    // Add columns to the new constructor, renaming as necessary
    for (size_t i = 0; i < constructor.getNumSelectors(); ++i) {
        std::string columnName = constructor[i].getName();
        Sort columnSort = constructor[i].getCodomainSort();

        // Check if this is the column to rename
        if (columnName == originColumnName) {
            columnName = renamedColumnName;
        }

        renamedTableConstructor.addSelector(columnName, columnSort);
    }

    // Add the constructor to the datatype and create the datatype sort
    renamedTableDatatype.addConstructor(renamedTableConstructor);
    Sort renamedTableSort = slv.mkDatatypeSort(renamedTableDatatype);

    // Replace the table's datatype with the renamed datatype
    table.resetDatatype(renamedTableSort);
    return table;
}

Table RelationalOperations::innerJoin(Table &table_first, Table &table_second, const std::string &columnNameFirst,
                                      const std::string &columnNameSecond) {
    // Retrieve the original table's name and solver
    Solver &slv = table_first.getSolver();

    // Get indices of the join columns
    std::vector<std::string> columnNamesFirst(1, columnNameFirst);
    int columnIndexFirst = table_first.getColumnIndices(columnNamesFirst)[0];

    std::vector<std::string> columnNamesSecond(1, columnNameSecond);
    int columnIndexSecond = table_second.getColumnIndices(columnNamesSecond)[0];

    // Prepare datatype for the joined table
    std::string joinedTableName = table_first.getTableName() + "_join_" + table_second.getTableName();
    DatatypeDecl joinedTableDatatype = slv.mkDatatypeDecl(joinedTableName);
    DatatypeConstructorDecl joinedTableConstructor = slv.mkDatatypeConstructorDecl("make-" + joinedTableName);

    // Define the new datatype by combining the datatypes from both tables
    // add columns from the first table
    const Datatype &firstTableDatatype = table_first.getTableSort().getDatatype();
    const DatatypeConstructor &firstTableConstructor = firstTableDatatype[0];

    const Datatype &secondTableDatatype = table_second.getTableSort().getDatatype();
    const DatatypeConstructor &secondTableConstructor = secondTableDatatype[0];

    // add columns from the first table
    for (size_t i = 0; i < firstTableConstructor.getNumSelectors(); ++i) {
        std::string columnName = firstTableConstructor[i].getName();
        Sort columnSort = firstTableConstructor[i].getCodomainSort();
        joinedTableConstructor.addSelector(columnName, columnSort);
    }
    // add columns from the second table, excluding the join column
    for (size_t i = 0; i < secondTableConstructor.getNumSelectors(); ++i) {
        std::string columnName = secondTableConstructor[i].getName();
        Sort columnSort = secondTableConstructor[i].getCodomainSort();
        if (columnName != columnNameSecond) { // Skip join column of second table
            joinedTableConstructor.addSelector(columnName, columnSort);
        }
    }
    joinedTableDatatype.addConstructor(joinedTableConstructor);
    Sort joinedTableSort = slv.mkDatatypeSort(joinedTableDatatype);

    // Create the joined table
    Table joinedTable(slv, joinedTableName, joinedTableSort);

    // Prepare vector to store the new rows
    std::vector<Term> joinedRows;

    // Join operation
    for (const auto &rowFirst: table_first.getRows()) {
        for (const auto &rowSecond: table_second.getRows()) {
            // Check if the join condition is satisfied
            if (rowFirst[columnIndexFirst + 1] == rowSecond[columnIndexSecond + 1]) {
                // Create vector to store the terms of the new joined row
                std::vector<Term> joinedRowTerms;

                // Get the constructor term for the new datatype
                DatatypeConstructor joinedTableCons = joinedTableSort.getDatatype()[0];
                Term makeJoinedTableTerm = joinedTableCons.getTerm();
                joinedRowTerms.push_back(makeJoinedTableTerm);

                // Append all terms from the first table row
                for (size_t i = 1; i <= table_first.getTableSort().getDatatype()[0].getNumSelectors(); ++i) {
                    joinedRowTerms.push_back(rowFirst[i]);
                }

                // Append terms from the second table row, excluding the join column
                for (size_t j = 1; j <= table_second.getTableSort().getDatatype()[0].getNumSelectors(); ++j) {
                    if (j != columnIndexSecond + 1) {
                        joinedRowTerms.push_back(rowSecond[j]);
                    }
                }

                // Create a new term for the joined row and add to the vector of new rows
                Term newJoinedRow = slv.mkTerm(Kind::APPLY_CONSTRUCTOR, joinedRowTerms);
                joinedRows.push_back(newJoinedRow);
            }
        }
    }

    // Use resetRows to set the joined rows to the new table
    joinedTable.resetRows(joinedRows);

    return joinedTable;
}

Table RelationalOperations::unionTwo(Table &table_first, Table &table_second) {
    // First, check if both tables have the same number of columns
    if (table_first.getNumColumns() != table_second.getNumColumns()) {
        throw std::invalid_argument("Tables have different numbers of columns.");
    }

    // Check if the data types of the columns are the same and in the same order
    const DatatypeConstructor &firstTableConstructor = table_first.getTableSort().getDatatype()[0];
    const DatatypeConstructor &secondTableConstructor = table_second.getTableSort().getDatatype()[0];

    for (size_t i = 0; i < firstTableConstructor.getNumSelectors(); ++i) {
        Sort firstColumnSort = firstTableConstructor[i].getCodomainSort();
        Sort secondColumnSort = secondTableConstructor[i].getCodomainSort();

        if (firstColumnSort != secondColumnSort) {
            throw std::invalid_argument("Column data types do not match.");
        }
    }

    // Assuming column names and their order are the same, proceed with the union
    Solver &slv = table_first.getSolver();
    std::string unionTableName = table_first.getTableName() + "_union_" + table_second.getTableName();
    Sort unionTableSort = table_first.getTableSort(); // Assuming both tables have the same sort

    // Create the union table
    Table unionTable(slv, unionTableName, unionTableSort);

    // Get rows from the first table
    std::vector<Term> unionRows = table_first.getRows();

    // Add rows from the second table
    std::vector<Term> secondTableRows = table_second.getRows();
    unionRows.insert(unionRows.end(), secondTableRows.begin(), secondTableRows.end());

    // Use resetRows to set the union rows to the new table
    // Assuming resetRows takes care of duplicates. If not, you'll need to remove duplicates manually
    unionTable.resetRows(unionRows);

    return unionTable;
}

Table RelationalOperations::groupByAggregate(Table &table, const std::pair<std::string, std::string> &aggregation,
                                             const std::string &groupedColumnName) {

    // Validate groupedColumnName is a valid column in the table
    std::vector<std::string> tableColumnNames = table.getColumnNames();
    if (std::find(tableColumnNames.begin(), tableColumnNames.end(), groupedColumnName) == tableColumnNames.end()) {
        throw std::invalid_argument("The grouped column name is not present in the table.");
    }

    // Validate the aggregation column is valid, if it's not "all"
    if (aggregation.first != "all" &&
        std::find(tableColumnNames.begin(), tableColumnNames.end(), aggregation.first) == tableColumnNames.end()) {
        throw std::invalid_argument("The aggregation column name is not present in the table.");
    }

    // Prepare for grouping
    std::map<Term, std::vector<Term>> groups; // For storing grouped rows

    // Retrieve the original table's name and solver
    Solver &slv = table.getSolver();

    // Group rows by the groupedColumnName
    int groupedColumnIndex = table.getColumnIndex(groupedColumnName);
    for (const auto &row: table.getRows()) {
        Term groupKey = row[groupedColumnIndex + 1]; // +1 offset if needed depending on how getRows is implemented
        groups[groupKey].push_back(row);
    }


    // Assuming groupedColumnName and aggregation result both return the same sort
    Sort groupColumnSort = table.getColumnSort(groupedColumnName);
    int aggregateColumnIndex = table.getColumnIndex(aggregation.first);
    Sort aggregateResultSort = slv.getStringSort();

    // Determine the aggregate operation
    std::function<Term(const std::vector<Term> &)> aggregateFunc;
    if (aggregation.second == "count") {
        aggregateResultSort = slv.getIntegerSort();
        aggregateFunc = [&slv](const std::vector<Term> &rows) -> Term {
            return slv.mkInteger(rows.size()); // Create a term from the count
        };
    }
        // Define the aggregateFunc for max
    else if (aggregation.second == "max") {
        // We assume that the aggregation is over a column with integer sort
        aggregateResultSort = slv.getIntegerSort();

        aggregateFunc = [&slv, aggregateColumnIndex](const std::vector<Term> &rows) -> Term {
            if (rows.empty()) {
                throw std::runtime_error("Cannot compute max on an empty set.");
            }

            // Start with the value of the aggregated column in the first row
            Term maxTerm = rows[0][aggregateColumnIndex + 1];

            // Iteratively apply the ITE operator to find the max
            for (const Term &row : rows) {
                Term currentTerm = row[aggregateColumnIndex + 1];
                maxTerm = slv.mkTerm(Kind::ITE, {slv.mkTerm(Kind::GT, {currentTerm, maxTerm}), currentTerm, maxTerm});
                Result result = slv.checkSat();
                if(result.isSat()){
                    maxTerm = slv.getValue(maxTerm);
                }
            }

            // maxTerm now represents the max value symbolically
            return maxTerm;
        };
    }
// Define the aggregateFunc for min
    else if (aggregation.second == "min") {
        // We assume that the aggregation is over a column with integer sort
        aggregateResultSort = slv.getIntegerSort();

        aggregateFunc = [&slv, aggregateColumnIndex](const std::vector<Term> &rows) -> Term {
            if (rows.empty()) {
                throw std::runtime_error("Cannot compute min on an empty set.");
            }

            // Start with the first value
            Term minTerm = rows[0][aggregateColumnIndex + 1];
            for (const Term &row : rows) {
                Term currentTerm = row[aggregateColumnIndex + 1];
                // Compare and assign the lesser value to minTerm
                minTerm = slv.mkTerm(Kind::ITE,
                                     {slv.mkTerm(Kind::LT, {currentTerm, minTerm}),
                                      currentTerm,
                                      minTerm});
                Result result = slv.checkSat();
                if(result.isSat()){
                    minTerm = slv.getValue(minTerm);
                }
            }
            return minTerm;
        };
    }
    else {
        aggregateResultSort = slv.getStringSort();
    }
    // You would add more conditions here for different types of aggregations like "sum", "min", "max", etc.

    // Define a new datatype for the aggregated result table
    std::string aggregatedTableName = "Aggregated_" + table.getTableName();
    DatatypeDecl aggregatedTableDatatype = slv.mkDatatypeDecl(aggregatedTableName);
    DatatypeConstructorDecl aggregatedTableConstructor = slv.mkDatatypeConstructorDecl("mk-" + aggregatedTableName);

    // Add grouped column selector to the constructor
    aggregatedTableConstructor.addSelector("groupedColumn", groupColumnSort);

    // Add aggregated result selector to the constructor
    aggregatedTableConstructor.addSelector("aggregateResult", aggregateResultSort);

    aggregatedTableDatatype.addConstructor(aggregatedTableConstructor);
    Sort aggregatedTableSort = slv.mkDatatypeSort(aggregatedTableDatatype);

    // Create the aggregated table
    Table aggregatedTable(slv, aggregatedTableName, aggregatedTableSort);

    std::vector<Term> aggregatedRows;

    // Create a term for the constructor of the new aggregated table datatype
    DatatypeConstructor aggregatedTableCons = aggregatedTableSort.getDatatype()[0];
    Term aggregatedTableConsTerm = aggregatedTableCons.getTerm();
    // Iterate through each group to create the rows
    for (const auto &group: groups) {
        std::vector<Term> newRowTerms;

        // Add the constructor term at the beginning of the newRowTerms
        newRowTerms.push_back(aggregatedTableConsTerm);

        // Add the group key term
        newRowTerms.push_back(group.first); // Assuming group.first is a Term with sort groupColumnSort

        // Add the aggregated result term
        newRowTerms.push_back(
                aggregateFunc(group.second)); // Assuming group.second is a Term with sort aggregateResultSort

        // Create a new term for the aggregated row
        Term aggregatedRow = slv.mkTerm(Kind::APPLY_CONSTRUCTOR, newRowTerms);
        aggregatedRows.push_back(aggregatedRow);
    }

    // Use the appropriate method to add rows to the aggregated table
    aggregatedTable.resetRows(aggregatedRows);

    // Return the aggregated table
    return aggregatedTable;

}



