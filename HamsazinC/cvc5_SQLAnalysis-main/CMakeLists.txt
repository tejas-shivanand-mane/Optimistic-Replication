cmake_minimum_required(VERSION 3.26)
project(TPC_cpp)

set(CMAKE_CXX_STANDARD 17)

# Use find_package to locate CVC5
find_package(cvc5 REQUIRED)
find_package(SQLite3 REQUIRED)

add_executable(TPC_cpp
        testSetOperations.cpp
        src/Table.cpp
        src/TableInfo.cpp
        testSetOperations.cpp
        src/RelationalAlgebras.cpp
        header/RelationalAlgebras.h
        src/RelationalProver.cpp
        header/RelationalProver.h
        header/Visitor.h
        src/VisitorNodes.cpp
        header/VisitorNodes.h
        testVisitor.cpp
        testAggregation.cpp
        src/SqlQueries.cpp
        header/SqlQueries.h
        refIntegrity.cpp
)

# Link your executable to the CVC5 library
target_link_libraries(TPC_cpp cvc5::cvc5 SQLite::SQLite3)