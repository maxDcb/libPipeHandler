add_executable(PipeHandlerClientTest "testClient.cpp" )
if(WIN32)
	target_link_libraries(PipeHandlerClientTest PipeHandler)
else()
endif()


add_executable(PipeHandlerServerTest "testServer.cpp" )
if(WIN32)
	target_link_libraries(PipeHandlerServerTest PipeHandler)
else()
endif()
