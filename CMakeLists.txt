cmake_minimum_required(VERSION 3.10)

project(Hashcollision)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DNDEBUG -g2 -O2 -pthread")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY bin)
add_subdirectory(lib/cryptopp)
add_executable(diffie src/diffie.cpp)
target_include_directories(diffie PRIVATE include)
target_link_libraries(diffie cryptopp-static)

add_executable(collision src/mulmap.cpp src/threadpool.cpp)
target_include_directories(collision PRIVATE include)
target_compile_features(collision PRIVATE cxx_std_17)
target_link_libraries(collision cryptopp-static)

#add_executable(vectorcoll src/vectorcoll.cpp src/threadpool.cpp)
#target_include_directories(vectorcoll PRIVATE include)
#target_compile_features(vectorcoll PRIVATE cxx_std_17)
#target_link_libraries(vectorcoll cryptopp-static)

