cmake_minimum_required(VERSION 3.0)
project(LearnVulkan)

set(CMAKE_CXX_STANDARD 17)

find_package(Vulkan REQUIRED FATAL_ERROR)
get_filename_component(VulkanSDK_LIBRARIES_PATH ${Vulkan_LIBRARY} DIRECTORY)

set(SDL2_LIBRARIES
    ${VulkanSDK_LIBRARIES_PATH}/SDL2.lib
    ${VulkanSDK_LIBRARIES_PATH}/SDL2main.lib)

add_executable(main main.cpp)

target_include_directories(main PUBLIC ${Vulkan_INCLUDE_DIRS})
target_link_libraries(main
    ${Vulkan_LIBRARIES}
    ${SDL2_LIBRARIES})

function(dump_cmake_variables)
    get_cmake_property(_variableNames VARIABLES)
    list(SORT _variableNames)
    foreach(_variableName ${_variableNames})
        if(ARGV0)
            unset(MATCHED)
            string(REGEX MATCH ${ARGV0} MATCHED ${_variableName})
            if(NOT MATCHED)
                continue()
            endif()
        endif()
        message(STATUS "${_variableName}=${${_variableName}}")
    endforeach()
endfunction()