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

aux_source_directory(tests TEST_FILES)

foreach(TEST_FILE ${TEST_FILES})
	get_filename_component(TEST_NAME ${TEST_FILE} NAME_WE)

	add_executable(${TEST_NAME}_tests ${TEST_FILE})
	target_link_libraries(${TEST_NAME}_tests PRIVATE ${LIBS})
	target_include_directories(${TEST_NAME}_tests PRIVATE ${LIBS})

	execute_process(COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/parser/build/parser.exe ${CMAKE_CURRENT_SOURCE_DIR}/${TEST_FILE} COMMAND_ECHO STDOUT OUTPUT_VARIABLE TEST_PARTS)

	foreach(PART ${TEST_PARTS})
		add_test(NAME ${TEST_NAME}_test_${PART} COMMAND $<TARGET_FILE:${TEST_NAME}_tests> ${PART})
	endforeach()
endforeach()

aux_source_directory(problems PROBLEM_FILES)

foreach(PROBLEM_FILE ${PROBLEM_FILES})
	get_filename_component(PROBLEM_NAME ${PROBLEM_FILE} NAME_WE)

	add_executable(${PROBLEM_NAME}_problem ${PROBLEM_FILE})

	execute_process(COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/parser/build/parser.exe ${CMAKE_CURRENT_SOURCE_DIR}/${PROBLEM_FILE} COMMAND_ECHO STDOUT OUTPUT_VARIABLE PROBLEM_PARTS)

	foreach(PART ${PROBLEM_PARTS})
		add_test(NAME ${PROBLEM_NAME}_problem_${PART} COMMAND $<TARGET_FILE:${PROBLEM_NAME}_problem> ${PART})
	endforeach()
endforeach()
