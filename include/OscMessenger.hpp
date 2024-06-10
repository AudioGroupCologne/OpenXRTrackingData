#pragma once
#include "PositionData.hpp"
#include <vector>
#include <string>
#include "oscpack/osc/include/OscOutboundPacketStream.h"

#define OUTPUT_BUFFER_SIZE 256
enum messageTypes {
    QUATERNION,
    XYZPRY,
    AZIMUTH,
    PYBINSIM
};

class UdpTransmitSocket;

class OSCMessenger {
public:
OSCMessenger(const char* address, int port, int mssgType);
~OSCMessenger();
void sendMessage(QuaternionPosition position);
void sendQuaternion(QuaternionPosition position);
void sendPRY(PRYPosition position);
void sendAzimuth(AzimuthPosition position);
void sendPyBinSim(AzimuthPosition position);
private:

std::unique_ptr<UdpTransmitSocket> transmitSocket;
int messageType;


};