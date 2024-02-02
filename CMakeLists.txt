cmake_minimum_required(VERSION 3.20)
project(DarkMxCompiler)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -g")

include_directories(/usr/include/antlr4-runtime/)
include_directories(include)
include_directories(include/AST)
include_directories(include/IR)
include_directories(include/OPT)
include_directories(src/generated)

file(GLOB_RECURSE sources src/*.cpp)
add_executable(mxc ${sources})
target_link_libraries(mxc PUBLIC antlr4-runtime)

# Testing Part
set(TestRootPath "${PROJECT_SOURCE_DIR}/Local/testcases/sema")
file(GLOB_RECURSE test_files RELATIVE ${TestRootPath} ${TestRootPath}/**.mx)
foreach (test_file ${test_files})
    if (${test_file} MATCHES ".*lambda.*")
        continue()
    endif ()
    execute_process(COMMAND grep "Verdict: " "${TestRootPath}/${test_file}" COMMAND sed "s/Verdict: //"
            OUTPUT_VARIABLE verdict)
    add_test(NAME ${test_file}
            COMMAND bash -c "$<TARGET_FILE:mxc> <${TestRootPath}/${test_file}"
            WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/Local/run")
    if (NOT "${verdict}" STREQUAL "Success\n")
        set_tests_properties(${test_file} PROPERTIES WILL_FAIL true)
    endif ()
endforeach ()

enable_testing()
