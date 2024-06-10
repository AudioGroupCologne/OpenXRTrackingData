#pragma once
#include <iostream>
#include "../include/OscMessenger.hpp"
# include "UdpSocket.h"
#include <chrono>
#define ADD "127.0.0.1"
#define PO 7000

OSCMessenger::OSCMessenger(const char* address, int port, int mssgType){
    std::cout << "Listening to " << address <<":"<< port <<'\n';
    messageType = mssgType;
    std::cout << "Messagetype: " ;
    switch (messageType)
    {
    case XYZPRY:
        std::cout << "xyzpry";
        break;
    case QUATERNION:
        std::cout << "quaternion";
        break;
    case AZIMUTH:
        std::cout << "Azimuth";
        break;
     case PYBINSIM:
        std::cout << "pybinsim";
        break;
    default:
        break;
    }
    std::cout << '\n';

    transmitSocket = std::make_unique<UdpTransmitSocket>(IpEndpointName( address, port ));

}

OSCMessenger::~OSCMessenger(){

}
void OSCMessenger::sendMessage(QuaternionPosition position){
    if (transmitSocket == nullptr){
        return;
    }
  
   auto time = std::chrono::system_clock::now();
   
     normaliseQuaternion(&position);
     switch (messageType) {
        case QUATERNION: {
        sendQuaternion(position);
            break;
        }
        case XYZPRY:{
            PRYPosition pry= convertQuat2PRY(position);
            sendPRY(pry);
           
            break;
        }
        case AZIMUTH:{
            AzimuthPosition az = convertQuaternion2Azimuth(position);
            sendAzimuth(az);
            break;
        }
        case PYBINSIM: {
             AzimuthPosition az =convertQuaternion2Azimuth(position);
            sendPyBinSim(az);
            break;
        }
      
        default:{
            break;
        }
     }
    
}


void OSCMessenger::sendQuaternion(QuaternionPosition position){
     
    char buffer[OUTPUT_BUFFER_SIZE];
    osc::OutboundPacketStream p( buffer, OUTPUT_BUFFER_SIZE );
    p << osc::BeginBundleImmediate
        << osc::BeginMessage( "/quaternion" ) 
        << (float)position.posX << (float)position.posY << (float)position.posZ << (float)position.oriW << (float)position.oriX << (float)position.oriY << (float)position.oriZ << osc::EndMessage
        << osc::EndBundle;
    
    transmitSocket->Send( p.Data(), p.Size() );
     std::cout << "send quaternion message" << '\n';

}
void OSCMessenger::sendPRY(PRYPosition position){
     char buffer[OUTPUT_BUFFER_SIZE];
    osc::OutboundPacketStream p( buffer, OUTPUT_BUFFER_SIZE );
    
       p << osc::BeginMessage( "/xyzpry" ) 
        << (float)position.posX << (float)position.posY << (float)position.posZ << (float)position.pitch << (float)position.roll << (float)position.yaw  << osc::EndMessage;
    transmitSocket->Send( p.Data(), p.Size() );
    
   
}

void OSCMessenger::sendAzimuth(AzimuthPosition position){
     char buffer[OUTPUT_BUFFER_SIZE];
    osc::OutboundPacketStream p( buffer, OUTPUT_BUFFER_SIZE );
    p << osc::BeginMessage( "/xyzazel" ) 
        << (float)position.posX << (float)position.posY << (float)position.posZ << (float)position.azimuth << (float)position.colatitude  << osc::EndMessage;
    transmitSocket->Send( p.Data(), p.Size() );
}
void OSCMessenger::sendPyBinSim(AzimuthPosition position){

    int angle = (int) position.azimuth;

   char buffer[OUTPUT_BUFFER_SIZE];
    osc::OutboundPacketStream p( buffer, OUTPUT_BUFFER_SIZE );
   
    p << osc::BeginMessage( "/pyBinSim" ) 
    << (int) 0 << (int) angle << (int) 0 << (int) 0 << (int) 0 << (int) 0 << (int) 0  <<osc::EndMessage;

    transmitSocket->Send( p.Data(), p.Size() );
   
}
