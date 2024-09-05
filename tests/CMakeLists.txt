add_executable(PipeHandlerClientTest "testClient.cpp" )
if(WIN32)
	set_property(TARGET PipeHandlerClientTest PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded")
	target_link_libraries(PipeHandlerClientTest PipeHandler)
else()
endif()


add_executable(PipeHandlerServerTest "testServer.cpp" )
if(WIN32)
	set_property(TARGET PipeHandlerServerTest PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded")
	target_link_libraries(PipeHandlerServerTest PipeHandler)
else()
endif()
