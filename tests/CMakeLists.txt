include_directories(backend_include)
add_executable(tensor_test_cpp   "./tensor_test.cpp" )
target_link_libraries(tensor_test_cpp backend)
add_test(tensor_fns tensor_test_cpp)
