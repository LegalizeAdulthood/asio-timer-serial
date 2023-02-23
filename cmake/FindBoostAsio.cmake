include(FindPackageHandleStandardArgs)

find_package(Boost)
if(Boost_FOUND)
    find_path(Boost_Asio_FOUND boost/asio.hpp PATHS ${Boost_INCLUDE_DIRS})
    if(Boost_Asio_FOUND)
	add_library(boost-asio INTERFACE)
	target_include_directories(boost-asio INTERFACE ${Boost_INCLUDE_DIRS})
	add_library(boost::asio ALIAS boost-asio)
    endif()
endif()

find_package_handle_standard_args(BoostAsio
    REQUIRED_VARS Boost_Asio_FOUND Boost_INCLUDE_DIRS)
