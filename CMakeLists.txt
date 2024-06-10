cmake_minimum_required(VERSION 3.5.0)
project(RecievePositionData VERSION 0.1.0 LANGUAGES C CXX)

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR})

include(CTest)
enable_testing()

#includes for oscpack
set(OSCPackPath_osc ${CMAKE_SOURCE_DIR}/oscpack/osc)
set(OSCPackPath_ip ${CMAKE_SOURCE_DIR}/oscpack/ip)

IF(WIN32)
 set(IpSystemTypePath ${OSCPackPath_ip}/src/win32)
 set(LIBS ${LIBS} Ws2_32 winmm)
ELSE(WIN32)
 set(IpSystemTypePath ${OSCPackPath_ip}/src/posix)
ENDIF(WIN32)

ADD_LIBRARY(oscpack 
${OSCPackPath_ip}/include/IpEndpointName.h
${OSCPackPath_ip}/src/IpEndpointName.cpp
${OSCPackPath_ip}/include/NetworkingUtils.h
${IpSystemTypePath}/NetworkingUtils.cpp
${OSCPackPath_ip}/include/UdpSocket.h
${IpSystemTypePath}/UdpSocket.cpp
${OSCPackPath_ip}/include/PacketListener.h
${OSCPackPath_ip}/include/TimerListener.h

${OSCPackPath_osc}/include/OscTypes.h
${OSCPackPath_osc}/src/OscTypes.cpp 
${OSCPackPath_osc}/include/OscHostEndianness.h
${OSCPackPath_osc}/include/OscException.h
${OSCPackPath_osc}/include/OscPacketListener.h
${OSCPackPath_osc}/include/MessageMappingOscPacketListener.h
${OSCPackPath_osc}/include/OscReceivedElements.h
${OSCPackPath_osc}/src/OscReceivedElements.cpp
${OSCPackPath_osc}/include/OscPrintReceivedElements.h
${OSCPackPath_osc}/src/OscPrintReceivedElements.cpp
${OSCPackPath_osc}/include/OscOutboundPacketStream.h
${OSCPackPath_osc}/src/OscOutboundPacketStream.cpp
)
#========================================================

add_executable(RecievePositionData main.cpp)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)



include_directories(RecievePositionData PRIVATE ${CMAKE_SOURCE_DIR}/oscpack/osc/include ,
                                                ${CMAKE_SOURCE_DIR}/oscpack/ip/include ,
                                                ${CMAKE_SOURCE_DIR}/openxr/include)
target_include_directories(RecievePositionData PRIVATE "${CMAKE_SOURCE_DIR}/openxr/include" ,
                                                ${CMAKE_SOURCE_DIR}/include )
                                    
file (GLOB SOURCES "${CMAKE_SOURCE_DIR}/src/*cpp")                                         
#target_sources(RecievePositionData PRIVATE ${CMAKE_SOURCE_DIR}/src/OscMessenger.cpp)
target_sources(RecievePositionData PRIVATE ${SOURCES})
target_link_libraries(RecievePositionData PRIVATE "${CMAKE_SOURCE_DIR}/openxr/lib/openxr_loader.lib"
                     oscpack ${LIBS}
                    )

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
