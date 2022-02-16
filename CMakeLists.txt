cmake_minimum_required(VERSION 3.0)

project(i3ds-wisdom LANGUAGES CXX)

set(PROJECT_AUTHOR "SINTEF Digital")
set(PROJECT_URL "http://i3ds-h2020.eu/")
set(PROJECT_LICENSE "MPLv2")
set(PROJECT_VERSION "1.0")

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package (Boost COMPONENTS program_options log REQUIRED)

add_executable(i3ds_wisdom main.cpp wisdom_i3ds_wrapper.cpp)

set (LIBS
    zmq
    i3ds
    i3ds_asn1
    i3ds_clients
    ${Boost_LIBRARIES}
  )

target_link_libraries(i3ds_wisdom ${LIBS})

add_executable(wisdom_ack_service wisdom_ack_service.cpp)
target_link_libraries(wisdom_ack_service ${LIBS})
