cmake_minimum_required(VERSION 3.18)

project(leetcode)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_C_STANDARD_REQUIRED ON)

execute_process(COMMAND rm parser/build/ && mkdir parser/build/)
execute_process(COMMAND ${CMAKE_COMMAND} -B ./parser/build/ -S ./parser)
execute_process(COMMAND ${CMAKE_COMMAND} --build ./parser/build/)

set(CORE_SOURCES
	core/binsearch.c
	core/sliding_window.c
	core/dfs.c
)
add_library(core ${CORE_SOURCES})
set(LIBS core)

enable_testing()

set(TESTS
	binsearch
	sliding_window
	dfs
)

foreach(TEST ${TESTS})
	add_executable(${TEST}_tests tests/${TEST}.c)
	target_link_libraries(${TEST}_tests PRIVATE ${LIBS})
	target_include_directories(${TEST}_tests PRIVATE ${LIBS})

	execute_process(COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/parser/build/parser.exe ${CMAKE_CURRENT_SOURCE_DIR}/tests/${TEST}.c COMMAND_ECHO STDOUT OUTPUT_VARIABLE TEST_PARTS)

	foreach(PART ${TEST_PARTS})
		add_test(NAME ${TEST}_test_${PART} COMMAND $<TARGET_FILE:${TEST}_tests> ${PART})
	endforeach()
endforeach()

message(WARNING "Add output generation without source files in CMakeLists.txt")

set(PROBLEMS
	35
	69
	1763
	100
	94
	101
	104
	219
	594
	222
	643
)

foreach(PROBLEM ${PROBLEMS})
	add_executable(${PROBLEM}_problem problems/${PROBLEM}.c)

	execute_process(COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/parser/build/parser.exe ${CMAKE_CURRENT_SOURCE_DIR}/problems/${PROBLEM}.c COMMAND_ECHO STDOUT OUTPUT_VARIABLE PROBLEM_PARTS)

	foreach(PART ${PROBLEM_PARTS})
		add_test(NAME ${PROBLEM}_problem_${PART} COMMAND $<TARGET_FILE:${PROBLEM}_problem> ${PART})
	endforeach()
endforeach()
