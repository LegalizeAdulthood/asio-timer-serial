add_executable(2-serial-ports serial.cpp asio.cpp)
target_compile_definitions(2-serial-ports PUBLIC BOOST_ASIO_SEPARATE_COMPILATION)
target_link_libraries(2-serial-ports PUBLIC boost::asio)
