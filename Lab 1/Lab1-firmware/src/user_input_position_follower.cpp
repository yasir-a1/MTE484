#include <Arduino.h>
#include "geeWhiz.h"

// ================== Pins ==================
int MOT_PIN = A0;   // motor angle sensor
int BAL_PIN = A1;   // ball position sensor

// ================== Constants ==================
const float MAX_ANGLE = PI / 4.0f;   // +45 degrees = +0.7854 rad
const float MIN_ANGLE = -PI / 4.0f;  // -45 degrees = -0.7854 rad

const float TOLERANCE = 0.035f;      // radians
const float MOTOR_VOLTAGE = 0.5f;    // volts

// ================== Target ==================
volatile float target_angle = 0.0f;


// ================== Setup ==================
void setup() {

  analogReadResolution(14);

  pinMode(A5, OUTPUT);

  Serial.begin(115200);
  delay(300);

  geeWhizBegin();

  // Run controller every 100 ms
  set_control_interval_ms(100);

  setMotorVoltage(0.0f);

  Serial.println("geeWhiz Started");
  Serial.println("Enter target angle in radians:");
  Serial.println("Allowed range: -0.7854 to +0.7854");
}


// ================== Main Loop ==================
void loop() {

  // Check if user entered something into Serial Monitor
  if (Serial.available() > 0) {

    // Read entered number
    float requested_angle = Serial.parseFloat();

    // Clamp target between -pi/4 and +pi/4
    target_angle = constrain(requested_angle, MIN_ANGLE, MAX_ANGLE);

    Serial.print("New target angle: ");
    Serial.print(target_angle, 4);
    Serial.println(" rad");

    // Remove remaining newline characters
    while (Serial.available() > 0) {
      Serial.read();
    }
  }
}


// ================== Control ISR ==================
void interval_control_code(void) {

  digitalWrite(A5, HIGH);

  // -------- Read Motor Sensor --------
  int motor_raw = analogRead(MOT_PIN);

  // Convert ADC reading to radians
  float motor_angle =
      -0.0003683f * (motor_raw - 4880);


  // -------- Read Ball Sensor --------
  int ball = analogRead(BAL_PIN);


  // -------- Calculate Error --------
  float error = target_angle - motor_angle;


  // -------- Motor Control --------

  // Motor is within tolerance
  if (fabs(error) <= TOLERANCE) {

    setMotorVoltage(0.0f);

  }

  // Motor needs to move in positive direction
  else if (error > 0.0f) {

    setMotorVoltage(-0.5f);

  }

  // Motor needs to move in negative direction
  else {

    setMotorVoltage(0.5f);

  }


  // -------- Serial Output --------
  Serial.print("Target: ");
  Serial.print(target_angle, 4);

  Serial.print(" rad, Motor: ");
  Serial.print(motor_angle, 4);

  Serial.print(" rad, Error: ");
  Serial.print(error, 4);

  Serial.print(" rad, Ball: ");
  Serial.println(ball);


  digitalWrite(A5, LOW);
}