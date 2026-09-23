#include <Arduino.h>
#include "geeWhiz.h"

// ======================================================
// PINS
// ======================================================

const int MOT_PIN = A0;   // Motor angle potentiometer


// ======================================================
// SAMPLING
// ======================================================

// Part (e) sampling interval
const unsigned long SAMPLE_PERIOD_MS = 50;

// 50 ms = 0.050 seconds
const float SAMPLE_PERIOD_S = 0.050f;


// ======================================================
// MOTOR SENSOR CALIBRATION
// ======================================================

// Your existing calibration:
//
// motor_angle = -0.0003683 * (motor_raw - 4880)
//
// 4880 ADC counts corresponds approximately to theta = 0 rad

const float MOTOR_SCALE = -0.0003683f;
const float MOTOR_ZERO  = 4880.0f;


// ======================================================
// SYSTEM IDENTIFICATION REFERENCE
// ======================================================

// Recommended small reference signal from the manual:
//
// -0.1 rad  --> +0.1 rad
//
// Step magnitude = 0.2 rad

const float REF_LOW  = -0.400f;
const float REF_HIGH =  0.400f;


// Full square-wave period
//
// 2 seconds means:
// 1 second at -0.1 rad
// 1 second at +0.1 rad
//
// You can adjust this if needed.

const float SQUARE_WAVE_PERIOD_S = 3.0f;


// ======================================================
// PROPORTIONAL CONTROLLER
// ======================================================

// IMPORTANT:
//
// The manual states that the proportional gains used
// for system identification will be NEGATIVE for this
// hardware.
//
// Start here and adjust experimentally.
//
// You eventually need THREE different Kp values.

const float KP = -10.0f;


// ======================================================
// STICTION COMPENSATION
// ======================================================

// Your experimentally determined values

const float STICTION_POSITIVE = 0.120f;
const float STICTION_NEGATIVE = 0.110f;


// ======================================================
// VARIABLES SHARED WITH INTERRUPT
// ======================================================

volatile float target_angle = REF_LOW;

volatile float motor_angle = 0.0f;
volatile int motor_raw = 0;

volatile float error_angle = 0.0f;

// Raw P-controller output BEFORE stiction compensation
volatile float controller_voltage = 0.0f;

// Actual voltage sent to motor AFTER stiction compensation
volatile float motor_voltage = 0.0f;


// Sample number
volatile unsigned long sample_count = 0;


// Used to tell loop() that new data is ready
volatile bool sample_ready = false;


// ======================================================
// SETUP
// ======================================================

void setup() {

  // Arduino UNO R4 ADC
  analogReadResolution(14);

  pinMode(A5, OUTPUT);

  Serial.begin(115200);

  delay(500);

  geeWhizBegin();


  // --------------------------------------------------
  // Control loop every 50 ms
  // --------------------------------------------------

  set_control_interval_ms(SAMPLE_PERIOD_MS);


  // Start motor stopped
  setMotorVoltage(0.0f);


  // --------------------------------------------------
  // CSV HEADER
  // --------------------------------------------------

  Serial.println();
  Serial.println("MTE 484 Lab 1 Part E - Motor System Identification");
  Serial.println();

  Serial.print("Kp = ");
  Serial.println(KP, 6);

  Serial.print("Sampling period = ");
  Serial.print(SAMPLE_PERIOD_MS);
  Serial.println(" ms");

  Serial.print("Reference = ");
  Serial.print(REF_LOW, 3);
  Serial.print(" to ");
  Serial.print(REF_HIGH, 3);
  Serial.println(" rad");

  Serial.println();

  Serial.println(
    "time_s,"
    "sample,"
    "target_rad,"
    "motor_rad,"
    "error_rad,"
    "controller_V,"
    "motor_V,"
    "motor_raw"
  );
}


// ======================================================
// MAIN LOOP
// ======================================================

void loop() {

  // Only print when a new 50 ms control sample exists
  if (sample_ready) {

    // Copy values
    //
    // This keeps Serial printing outside the control ISR.

    noInterrupts();

    unsigned long local_sample = sample_count;

    float local_target = target_angle;
    float local_motor = motor_angle;
    float local_error = error_angle;

    float local_controller_voltage = controller_voltage;
    float local_motor_voltage = motor_voltage;

    int local_motor_raw = motor_raw;

    sample_ready = false;

    interrupts();


    // Calculate experiment time
    float time_s =
        local_sample * SAMPLE_PERIOD_S;


    // --------------------------------------------------
    // CSV OUTPUT
    // --------------------------------------------------

    Serial.print(time_s, 3);
    Serial.print(",");

    Serial.print(local_sample);
    Serial.print(",");

    Serial.print(local_target, 5);
    Serial.print(",");

    Serial.print(local_motor, 5);
    Serial.print(",");

    Serial.print(local_error, 5);
    Serial.print(",");

    Serial.print(local_controller_voltage, 5);
    Serial.print(",");

    Serial.print(local_motor_voltage, 5);
    Serial.print(",");

    Serial.println(local_motor_raw);
  }
}


// ======================================================
// CONTROL ISR
// ======================================================

void interval_control_code(void) {

  digitalWrite(A5, HIGH);


  // ==================================================
  // 1. GENERATE SQUARE-WAVE REFERENCE
  // ==================================================

  // Number of samples in HALF a square-wave period

  const unsigned long HALF_PERIOD_SAMPLES =
      (unsigned long)(
          (SQUARE_WAVE_PERIOD_S / 2.0f)
          / SAMPLE_PERIOD_S
      );


  // Alternate between REF_LOW and REF_HIGH

  if (
      (sample_count / HALF_PERIOD_SAMPLES) % 2 == 0
  ) {

    target_angle = REF_LOW;

  }

  else {

    target_angle = REF_HIGH;

  }


  // ==================================================
  // 2. READ MOTOR SENSOR
  // ==================================================

  motor_raw = analogRead(MOT_PIN);


  // Convert ADC reading to radians

  motor_angle =
      MOTOR_SCALE *
      ((float)motor_raw - MOTOR_ZERO);


  // ==================================================
  // 3. CALCULATE POSITION ERROR
  // ==================================================

  error_angle =
      target_angle - motor_angle;


  // ==================================================
  // 4. PROPORTIONAL CONTROLLER
  // ==================================================

  // Vc = Kp * error

  controller_voltage =
      KP * error_angle;


  // ==================================================
  // 5. STICTION COMPENSATION
  // ==================================================

  if (controller_voltage > 0.0f) {

    motor_voltage =
        controller_voltage
        + STICTION_POSITIVE;

  }

  else if (controller_voltage < 0.0f) {

    motor_voltage =
        controller_voltage
        - STICTION_NEGATIVE;

  }

  else {

    motor_voltage = 0.0f;

  }


  // ==================================================
  // 6. APPLY MOTOR VOLTAGE
  // ==================================================

  setMotorVoltage(motor_voltage);


  // ==================================================
  // 7. UPDATE SAMPLE COUNTER
  // ==================================================

  sample_count++;

  sample_ready = true;


  digitalWrite(A5, LOW);
}