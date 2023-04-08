add_library(boost_sam_test_impl test_impl.cpp)
target_compile_definitions(boost_sam_test_impl PUBLIC -DBOOST_SAM_SEPARATE_COMPILATION=1)


if(NOT TARGET tests)
    add_custom_target(tests)
endif()


function(boost_sam_standalone_test name)
    add_executable(boost_sam_${name} ${name}.cpp)
    target_link_libraries(boost_sam_${name} boost_sam_test_impl boost_sam)
    add_test(NAME boost_sam_${name} COMMAND boost_sam_${name})
    add_dependencies(tests boost_sam_${name})
endfunction()

boost_sam_standalone_test(basic_semaphore)
boost_sam_standalone_test(basic_mutex)
boost_sam_standalone_test(basic_shared_mutex)
boost_sam_standalone_test(basic_condition_variable)
boost_sam_standalone_test(basic_barrier)
boost_sam_standalone_test(concurrency_hint)
boost_sam_standalone_test(guarded)
boost_sam_standalone_test(lock_guard)

