cmake_minimum_required(VERSION 3.18)

project(leetcode)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CORE_SOURCES
	core/binsearch.c
	core/sliding_window.c
	core/dfs.c
	core/bfs.c
)

add_library(core ${CORE_SOURCES})

set(LIBS core)

enable_testing()

set(TESTS
	binsearch
	sliding_window
	dfs
)

set(binsearch_parts 1 2 3 4 5 6 7 8 9 10 11 12 13)
set(sliding_window_parts 1 2 3 4 5 6 7 8 9 10 11 12)
set(dfs_parts 1 2)

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
