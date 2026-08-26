include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        v1.17.0
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(hyperloglog_tests test_hyperloglog.cpp)
target_link_libraries(hyperloglog_tests PRIVATE hyperloglog murmur3 GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(hyperloglog_tests)