add_executable(UDP Udp.cpp)
target_include_directories(UDP PUBLIC
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        ${PUBLIC_DEPEND_INCLUDES})
target_include_directories(UDP PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${PRIVATE_DEPEND_INCLUDES})
target_link_libraries(UDP
        PUBLIC ${PUBLIC_DEPEND_LIBS}
        PRIVATE ${PRIVATE_DEPEND_LIBS})
