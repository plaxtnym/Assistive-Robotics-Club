#include <Wire.h>
#include <MPU6050.h>

MPU6050 mpu;

void setup() {
    Serial.begin(9600);
    Wire.begin();
    mpu.initialize();
}

void loop() {
    int16_t ax, ay, az;
    mpu.getAcceleration(&ax, &ay, &az);

    float Ax = ax / 16384.0 * 9.81;
    float Ay = ay / 16384.0 * 9.81;
    float Az = az / 16384.0 * 9.81;

    float A_total = sqrt(Ax * Ax + Ay * Ay + Az * Az);

    Serial.println(A_total); // Monitor acceleration magnitude

    delay(500);
}