add_executable(TCP TcpImg.cpp)
target_include_directories(TCP PUBLIC
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        ${PUBLIC_DEPEND_INCLUDES})
target_include_directories(TCP PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${PRIVATE_DEPEND_INCLUDES})
target_link_libraries(TCP
        PUBLIC ${PUBLIC_DEPEND_LIBS}
        PRIVATE ${PRIVATE_DEPEND_LIBS})
