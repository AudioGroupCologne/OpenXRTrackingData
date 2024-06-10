#include <iostream>
#include "XrProgram.hpp"
#include <vector>
#include <thread>
#include <chrono>
#include <string>


#include "oscpack/osc/include/OscOutboundPacketStream.h"
#include "oscpack/ip/include/UdpSocket.h"
#include "OscMessenger.hpp"

bool isIPAdressValid(char* address){
  std ::string add = address;
    if((add.length() >= 7)&&(add.length() <= 15)){
    int dotc = 0;
    for (char ch : add){
      if (ch == '.'){
        dotc++;
      }
    }
    if (dotc == 3){
      return true;
    }
  }
  return false;
};

int main(int argc, char* argsv[]){



  const char* address {"127.0.0.1"};
  int port = 7000;
 int messageType = AZIMUTH;

 for (int i = 0 ; i < argc; i++){
    if (!strcmp(argsv[i],"-ip")&& isIPAdressValid(argsv[i + 1])){
      address = argsv[i + 1];
    }
    if ( !strcmp(argsv[i],"-p")){
     std::cout << argsv[i] << '\n';
     port = std::stoi(argsv[i +1]);
    }
    if (!strcmp(argsv[i],"-m")){
  
     if (!strcmp(argsv[i + 1],"azimuth")){
       messageType = AZIMUTH;
     }
       if (!strcmp(argsv[i + 1],"quaternion")){
       messageType = QUATERNION;
     }
      if (!strcmp(argsv[i + 1],"pybinsim")){
        messageType = PYBINSIM;
     }
   }
   
 }

  using namespace std::chrono_literals;
  std::cout << "Application Started...  \n";
  OSCMessenger messenger(address, port, messageType);
  XrProgram program(&messenger);
 if(program.init() == 1){
  std::cout << "abort..." << '\n';
  return 1;
 };  

static bool quitKeyPressed = false;
auto exitPollingThread = std::thread{[] {
(void)getchar();
quitKeyPressed = false;
}};
exitPollingThread.detach();
auto time = std::chrono::system_clock::now();
std::cout << "press enter key to stop application" << '\n';
do {
  auto t2 = std::chrono::system_clock::now();
 auto r =  std::chrono::duration_cast<std::chrono::duration<float>>(t2 - time).count() * 1000.0f;
  if (r >= 20.0f){
    std::cout<< r <<"  lag" <<'\n';
 
  }
 
   program.pollEvents();
    if (program.isSessionRunning()){
      program.getPositionData();   
    }
     time = t2;

} while(!quitKeyPressed);

std::cout << "applicaion finished" << '\n';

}

