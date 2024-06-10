
#pragma once 
#include <array>
#include <string>
#include <numbers>
#include <iostream>
#include <vector>
#include <numeric>
#include <DirectXMath.h>

struct QuaternionPosition {
    std::string sourceName;
    float posX;
    float posY;
    float posZ;

    float oriW;
    float oriX;
    float oriY;
    float oriZ;
};

struct PRYPosition{
    std::string sourceName;
    float posX;
    float posY;
    float posZ;

    float pitch;
    float roll;
    float yaw;
};

struct AzimuthPosition{
    std::string sourceName;
    float posX;
    float posY;
    float posZ;

    float azimuth;
    float colatitude;
};


inline void normaliseQuaternion(QuaternionPosition* position){
     float magnitude = sqrtf( powf(position->oriW,2) + powf(position->oriX,2) + powf(position->oriY,2) + powf(position->oriZ,2));
    position->oriW = position->oriW / magnitude;
    position->oriX = position->oriX / magnitude;
    position->oriY = position->oriY / magnitude;
    position->oriZ = position->oriZ / magnitude;
}

inline PRYPosition convertQuat2PRY(QuaternionPosition quat){
    PRYPosition pry{quat.sourceName, quat.posX, quat.posY, quat.posZ};
    /*  double pitch = std::asin(2 * (quat.oriW * quat.oriX - quat.oriZ * quat.oriY)) *  (180.0f / std::numbers::pi) - 90;
     double roll = std::atan2(2 * (quat.oriW * quat.oriY + quat.oriX * quat.oriZ) ,  1 - 2 * (quat.oriX * quat.oriX + quat.oriY * quat.oriY)) * (180.0f / std::numbers::pi);
    double yaw = std::atan2(2 * (quat.oriW * quat.oriZ + quat.oriX * quat.oriY), 1 - 2 * (quat.oriY * quat.oriY + quat.oriZ * quat.oriZ))  * (180.0f / std::numbers::pi); */
    double w = quat.oriW;
    double x = quat.oriX;
    double y = quat.oriY;
    double z = quat.oriZ;
    double deg = (180 / std::numbers::pi); 
    double pitch, roll, yaw;
    
    // right handed ZYX
   /*  pitch = std::asin(2*(w*y - z*x));
    roll = std::atan2(2*(w*x + y*z), 1-2*(x*x + y*y));
    yaw = std::atan2(2*(w*z + x*y), 1 - 2*(y*y + z*z)); */
    yaw = std::asin(2*(w*y - z*x));
    pitch = std::atan2(2*(w*x + y*z), 1-2*(x*x + y*y));
    roll = std::atan2(2*(w*z + x*y), 1 - 2*(y*y + z*z)); 
   
    pitch *= deg;
    roll *= deg;
    yaw *= deg;
    
    /*  if (std::abs(w) > 0.5){
         yaw = 90 + (90 - yaw);
        }
        if (yaw < 0){
        yaw += 360;
    } */
    
    pry.pitch = pitch;
    pry.roll = roll;
    pry.yaw = yaw;
    std::cout<< "p: " << pitch << "  r: " << roll << "  y: " << yaw << '\n';
    std::cout << "w:" << w << " x: "<< x <<" y: "<< y << " z: " << z << '\n';
    return pry;
}

inline AzimuthPosition convertPRY2Azimuth(PRYPosition pry){
    AzimuthPosition pos {pry.sourceName, pry.posX, pry.posY, pry.posZ};
    float az = std::fmod((360.0f + pry.yaw) , 360.0f);
    az = std::fmod((az - 90), 360.0f);
    float el = pry.roll;
    pos.azimuth = az;
    pos.colatitude = el;
    return pos;


}

inline AzimuthPosition convertQuaternion2Azimuth(QuaternionPosition q){
    AzimuthPosition position = {
        .posX = q.posX,
        .posY = q.posY,
        .posZ = q.posZ
    };
    
    using namespace DirectX;
    // Quaternion in Direct X representation
    XMVECTOR quat = XMVectorSet(q.oriX, q.oriY, q.oriZ, q.oriW);

    // Tracker is rotated by 90° on the x Axis 
    // Rotate to get 0° elevation for the resting position
    XMVECTOR xAxis = XMVectorSet(1.0f, 0.0f,0.0f,1.0f);
    float angle =XMConvertToRadians(90.0f);
    XMVECTOR rotation = XMQuaternionRotationAxis(xAxis, angle);
    quat = XMQuaternionMultiply(quat, rotation);
    
    // Turn quaternion into a rotation maxtrix to extract the angles from
    XMMATRIX rot_mat =XMMatrixRotationQuaternion(quat);
    // azimuth
    float m12 = XMVectorGetY(rot_mat.r[0]);
    float m11 = XMVectorGetX(rot_mat.r[0]);
    float az = atan2f(m12, m11);
    float az_deg = XMConvertToDegrees(az);

    // elevation
    float m32 = XMVectorGetZ(rot_mat.r[1]);
    float el = -asinf(m32);
    float el_deg = XMConvertToDegrees(el);

    // map angles from +/- 180° to 360°
    az_deg = (int(az_deg) % 360 + 360 ) % 360;
    el_deg = (int(el_deg) % 360 + 360 ) % 360;

    // angles returned from DirecXMath are counterclockwise so they need to be flipped
    az_deg = (360 - int(az_deg)) % 360;
    el_deg = (360 - int(el_deg)) % 360;

    position.azimuth = az_deg;
    position.colatitude = el_deg;

    std::cout << "AZ: " << az_deg << "  EL: " << el_deg <<  '\n';

    return position;

}
