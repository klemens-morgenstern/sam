enable_testing()

if (BOOST_ASEM_STANDALONE)
    find_package(Boost REQUIRED unit_test_framework)
endif()

add_library(boost_asem_test_impl test_impl.cpp)
target_link_libraries(boost_asem_test_impl Boost::unit_test_framework)
target_compile_definitions(boost_asem_test_impl PUBLIC -DBOOST_asem_SEPARATE_COMPILATION=1)


function(boost_asem_standalone_test name)
    add_executable(boost_asem_${name} ${name}.cpp)
    target_link_libraries(boost_asem_${name} boost_asem_test_impl)
    add_test(NAME boost_asem_${name} COMMAND $<TARGET_FILE:boost_asem_${name}>)
endfunction()

boost_asem_standalone_test(basic_semaphore)
boost_asem_standalone_test(basic_mutex)
boost_asem_standalone_test(basic_condition_variable)
boost_asem_standalone_test(guarded)