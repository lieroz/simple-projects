cmake_minimum_required(VERSION 3.18)

project(leetcode)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_library(core core/binsearch.c)

set(LIBS core)

enable_testing()

set(TESTS
	binsearch)

set(binsearch_parts 1 2 3 4 5 6 7 8 9 10 11 12 13)

foreach(TEST ${TESTS})
	add_executable(${TEST}_tests tests/${TEST}.c)
	target_link_libraries(${TEST}_tests PRIVATE ${LIBS})
	target_include_directories(${TEST}_tests PRIVATE ${LIBS})

	if(NOT DEFINED "${TEST}_parts")
		set(${TEST}_parts "1")
	endif()
	foreach(PART ${${TEST}_parts})
		add_test(NAME ${TEST}_test_${PART} COMMAND $<TARGET_FILE:${TEST}_tests> ${PART})
	endforeach()
endforeach()
