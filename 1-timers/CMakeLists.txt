add_executable(1-timers timers.cpp)
target_compile_definitions(1-timers PUBLIC _WIN32_WINNT=0x0601)
target_link_libraries(1-timers PUBLIC boost::asio)
