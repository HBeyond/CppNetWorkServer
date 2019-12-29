add_executable(Udp-LargeFile Udp-LargeFile.cpp)
target_include_directories(Udp-LargeFile PUBLIC
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        ${PUBLIC_DEPEND_INCLUDES})
target_include_directories(Udp-LargeFile PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${PRIVATE_DEPEND_INCLUDES})
target_link_libraries(Udp-LargeFile
        PUBLIC ${PUBLIC_DEPEND_LIBS}
        PRIVATE ${PRIVATE_DEPEND_LIBS})
