find_package(Vulkan REQUIRED)
# find_package(MPI REQUIRED)
# find_package(MKL CONFIG REQUIRED)


set(bigbackend_include "${bigbackend_SOURCE_DIR}/include")
set(bigbackend_layers_include "${bigbackend_SOURCE_DIR}/layers")


# include_directories("${bigbackend_SOURCE_DIR}/include")
include_directories(${bigbackend_include} ${bigbackend_layers_include})

file(GLOB backend_headers "${bigbackend_SOURCE_DIR}/include/*.h")
file(GLOB backend_layer_headers "${bigbackend_SOURCE_DIR}/layers/*/*.hpp")
file(GLOB backend_cpp "${bigbackend_SOURCE_DIR}/src/*.cpp")

# include_directories(SYSTEM ${MPI_INCLUDE_PATH})


add_library(backend STATIC  ${backend_headers} ${backend_layer_headers} ${backend_cpp})
target_include_directories(backend PUBLIC ${bigbackend_include})
target_include_directories(backend PUBLIC ${bigbackend_layers_include})
target_include_directories(backend PUBLIC $<BUILD_INTERFACE:${Vulkan_INCLUDE_DIR}>)
target_link_libraries(backend ${Vulkan_LIBRARY})

# target_compile_options(backend PUBLIC $<TARGET_PROPERTY:MKL::MKL_C,INTERFACE_COMPILE_OPTIONS>)
# target_include_directories(backend PUBLIC $<TARGET_PROPERTY:MKL::MKL_C,INTERFACE_INCLUDE_DIRECTORIES>)
# target_link_libraries(backend PUBLIC $<LINK_ONLY:MKL::MKL_C>)
# target_link_libraries(backend ${MPI_C_LIBRARIES})