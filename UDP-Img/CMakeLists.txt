add_executable(UDP-Img Udp-Img.cpp)
target_include_directories(UDP-Img PUBLIC
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        ${PUBLIC_DEPEND_INCLUDES})
target_include_directories(UDP-Img PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${PRIVATE_DEPEND_INCLUDES})
target_link_libraries(UDP-Img
        PUBLIC ${PUBLIC_DEPEND_LIBS}
        PRIVATE ${PRIVATE_DEPEND_LIBS})
