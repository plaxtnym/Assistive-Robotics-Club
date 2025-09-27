#include <Wire.h>
#include <Adafruit_VL53L1X.h>
#include <MPU6050.h>
#include <math.h>

// -------- Pin map (your request) --------
const int trigPin   = D0;   // Ultrasonic TRIG
const int echoPin   = D1;   // Ultrasonic ECHO
const int vibPin    = D6;   // Vibration motor PWM (via MOSFET driver!)
const int buzzerPin = D8;   // Optional piezo buzzer

// -------- Ultrasonic constants --------
const float SPEED_OF_SOUND_CM_PER_US = 0.0343f;  // 343 m/s
const unsigned long ECHO_TIMEOUT_US  = 30000UL;  // 30 ms (~5m)

// >>> NEW: extend active haptic range to ~1.3 m
const int ACTIVE_MAX_CM = 70;                   // vibrate only up to this distance

// -------- Sensors --------
Adafruit_VL53L1X tof;
MPU6050 mpu;

// -------- State --------
int   prevDistToF_cm = 0;
float prevAccel_g    = 0.0f;

void alertUser1() {           // short A4 beep (~440 Hz)
  tone(buzzerPin, 440, 200);  // ESP32 Arduino core provides tone()
  delay(200);
}
void alertUser2() {           // short B4 beep (~494 Hz)
  tone(buzzerPin, 494, 500);
  delay(500);
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);

  // Ultrasonic
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  digitalWrite(trigPin, LOW);

  // Vibration PWM — use analogWrite wrapper (ESP32 core maps to LEDC under the hood)
  pinMode(vibPin, OUTPUT);
  analogWrite(vibPin, 0);   // start off

  // I2C on your pins
  Wire.begin(D3, D2);       // SDA=D3, SCL=D2

  // MPU6050
  mpu.initialize();
  if (!mpu.testConnection()) {
    Serial.println("MPU6050 not found! Check wiring/power.");
  }

  // VL53L1X ToF
  if (!tof.begin()) {
    Serial.println("Failed to find VL53L1X! Check wiring/power.");
    while (true) { delay(10); }
  }
  // Avoid optional API differences; just start ranging
  tof.startRanging();

  delay(100);
}

void loop() {
  // ---------- Ultrasonic read ----------
  // (tiny pre-low is optional but tidy)
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  unsigned long duration = pulseIn(echoPin, HIGH, ECHO_TIMEOUT_US);

  int distanceUltrasonic_cm = -1;
  if (duration > 0) {
    distanceUltrasonic_cm = (int)(duration * SPEED_OF_SOUND_CM_PER_US / 2.0f);
  }

  Serial.print("Ultrasonic: ");
  if (distanceUltrasonic_cm >= 0) {
    Serial.print(distanceUltrasonic_cm);
    Serial.println(" cm");
  } else {
    Serial.println("no echo");
  }

  // ---------- Vibration mapping ----------
  // Now scales 0..ACTIVE_MAX_CM to 255..0 (closer = stronger),
  // and turns OFF if no echo or beyond ACTIVE_MAX_CM.
  int vibrationStrength = 0;
  if (distanceUltrasonic_cm >= 0 && distanceUltrasonic_cm <= ACTIVE_MAX_CM) {
    vibrationStrength = map(distanceUltrasonic_cm, 0, ACTIVE_MAX_CM, 255, 0);
    vibrationStrength = constrain(vibrationStrength, 0, 255);
  } else {
    vibrationStrength = 0; // out of range → off
  }
  analogWrite(vibPin, vibrationStrength);

  // ---------- ToF read ----------
  if (tof.dataReady()) {
    int distanceToF_mm = tof.distance(); // mm
    tof.clearInterrupt();

    int distanceToF_cm = (distanceToF_mm > 0) ? (distanceToF_mm / 10) : -1;

    Serial.print("ToF: ");
    if (distanceToF_cm >= 0) {
      Serial.print(distanceToF_cm);
      Serial.println(" cm");
    } else {
      Serial.println("invalid");
    }

    // Simple step detection
    if (prevDistToF_cm > 0 && distanceToF_cm > 0) {
      if ((distanceToF_cm - prevDistToF_cm) > 10) {
        Serial.println("Step Down Detected!");
        alertUser1();
      } else if ((prevDistToF_cm - distanceToF_cm) > 10) {
        Serial.println("Step Up Detected!");
        alertUser2();
      }
    }
    if (distanceToF_cm > 0) prevDistToF_cm = distanceToF_cm;
  }

  // ---------- MPU6050 Accel magnitude ----------
  int16_t ax, ay, az;
  mpu.getAcceleration(&ax, &ay, &az);

  // Convert to g's (raw/16384 ≈ g)
  float Ax_g = ax / 16384.0f;
  float Ay_g = ay / 16384.0f;
  float Az_g = az / 16384.0f;

  float A_total_g = sqrtf(Ax_g*Ax_g + Ay_g*Ay_g + Az_g*Az_g);
  Serial.print("Accel |g|: ");
  Serial.println(A_total_g, 3);

  // Simple fall detection (tune threshold)
  if (prevAccel_g > 0 && (A_total_g - prevAccel_g) > 1.0f) {
    Serial.println("Stick fell!");
  }
  prevAccel_g = A_total_g;

  delay(50);
}
