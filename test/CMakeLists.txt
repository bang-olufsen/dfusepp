set(MODULE_NAME test-dfusepp)

add_executable(${MODULE_NAME} src/TestDfusepp.cpp)
target_link_libraries(${MODULE_NAME} catch turtle dfusepp)

add_test(${MODULE_NAME} ${MODULE_NAME})
