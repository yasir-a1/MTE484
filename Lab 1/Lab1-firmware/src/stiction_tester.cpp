#include <Arduino.h>
#include "geeWhiz.h"

// ======================================================
// STICTION TEST SETTINGS
// ======================================================

// Starting magnitude
const float START_VOLTAGE = 0.065f;

// Change voltage by 0.005 V each step
const float STEP_SIZE = 0.005f;

// Wait 2 seconds between voltage steps
const unsigned long STEP_TIME = 3000;

// Wait 4 seconds between positive and negative tests
const unsigned long PAUSE_TIME = 4000;

// Safety limit so the test cannot ramp forever
const float MAX_TEST_VOLTAGE = 0.50f;


// ======================================================
// MOTOR COMMAND
// ======================================================

// This is the voltage continuously applied by the
// geeWhiz control interrupt.
volatile float motor_voltage = 0.0f;


// ======================================================
// TIMING VARIABLES
// ======================================================

unsigned long previousStepTime = 0;
unsigned long pauseStartTime = 0;


// ======================================================
// TEST STATES
// ======================================================

enum TestState {

  POSITIVE_RAMP,
  PAUSE,
  NEGATIVE_RAMP,
  FINISHED

};

TestState state = POSITIVE_RAMP;


// ======================================================
// SETUP
// ======================================================

void setup() {

  // Same ADC resolution used in your working code
  analogReadResolution(14);

  // Timing/debug pin used by your existing code
  pinMode(A5, OUTPUT);

  // Start Serial
  Serial.begin(115200);
  delay(300);

  // Start geeWhiz library
  geeWhizBegin();

  // Run interrupt every 100 ms
  set_control_interval_ms(100);


  // --------------------------------------------------
  // START POSITIVE TEST
  // --------------------------------------------------

  motor_voltage = START_VOLTAGE;

  previousStepTime = millis();


  Serial.println();
  Serial.println("================================");
  Serial.println("       MOTOR STICTION TEST");
  Serial.println("================================");
  Serial.println();

  Serial.println("POSITIVE VOLTAGE TEST");
  Serial.println("Watch the LARGE GEAR.");
  Serial.println("Press any key when it begins moving consistently.");
  Serial.println();

  Serial.print("Motor Voltage: ");
  Serial.print(motor_voltage, 3);
  Serial.println(" V");
}


// ======================================================
// MAIN LOOP
// ======================================================

void loop() {


  // ====================================================
  // 1. POSITIVE VOLTAGE RAMP
  // ====================================================

  if (state == POSITIVE_RAMP) {


    // --------------------------------------------------
    // Check for first keypress
    // --------------------------------------------------

    if (Serial.available() > 0) {

      // Remove everything currently in Serial buffer
      while (Serial.available() > 0) {
        Serial.read();
      }


      // Save voltage before zeroing it
      float positive_stiction = motor_voltage;


      // Immediately stop motor
      motor_voltage = 0.0f;


      Serial.println();
      Serial.println("================================");
      Serial.println("POSITIVE STICTION FOUND");
      Serial.println("================================");

      Serial.print("Positive stiction voltage = ");
      Serial.print(positive_stiction, 3);
      Serial.println(" V");

      Serial.println();
      Serial.println("Motor Voltage: 0.000 V");
      Serial.println();
      Serial.println("Waiting 4 seconds...");
      Serial.println();


      // Start 4-second pause
      pauseStartTime = millis();

      state = PAUSE;

      return;
    }


    // --------------------------------------------------
    // Increase voltage every 2 seconds
    // --------------------------------------------------

    if (millis() - previousStepTime >= STEP_TIME) {

      previousStepTime = millis();

      motor_voltage += STEP_SIZE;


      Serial.print("Motor Voltage: ");
      Serial.print(motor_voltage, 3);
      Serial.println(" V");


      // ----------------------------------------------
      // Safety cutoff
      // ----------------------------------------------

      if (motor_voltage >= MAX_TEST_VOLTAGE) {

        motor_voltage = 0.0f;

        Serial.println();
        Serial.println("MAXIMUM TEST VOLTAGE REACHED.");
        Serial.println("Motor stopped for safety.");
        Serial.println("Test terminated.");

        state = FINISHED;
      }
    }
  }



  // ====================================================
  // 2. FOUR-SECOND PAUSE
  // ====================================================

  else if (state == PAUSE) {

    // Keep motor at zero during entire pause
    motor_voltage = 0.0f;


    if (millis() - pauseStartTime >= PAUSE_TIME) {


      // ----------------------------------------------
      // Start negative-direction test
      // ----------------------------------------------

      motor_voltage = -START_VOLTAGE;

      previousStepTime = millis();


      Serial.println("================================");
      Serial.println("NEGATIVE VOLTAGE TEST");
      Serial.println("================================");

      Serial.println();
      Serial.println("Watch the LARGE GEAR.");
      Serial.println("Press any key when it begins moving consistently.");
      Serial.println();


      Serial.print("Motor Voltage: ");
      Serial.print(motor_voltage, 3);
      Serial.println(" V");


      state = NEGATIVE_RAMP;
    }
  }



  // ====================================================
  // 3. NEGATIVE VOLTAGE RAMP
  // ====================================================

  else if (state == NEGATIVE_RAMP) {


    // --------------------------------------------------
    // Check for second keypress
    // --------------------------------------------------

    if (Serial.available() > 0) {


      // Remove everything currently in Serial buffer
      while (Serial.available() > 0) {
        Serial.read();
      }


      // Save voltage before zeroing it
      float negative_stiction = motor_voltage;


      // Immediately stop motor
      motor_voltage = 0.0f;


      Serial.println();
      Serial.println("================================");
      Serial.println("NEGATIVE STICTION FOUND");
      Serial.println("================================");

      Serial.print("Negative stiction voltage = ");
      Serial.print(negative_stiction, 3);
      Serial.println(" V");


      Serial.println();
      Serial.println("Motor Voltage: 0.000 V");

      Serial.println();
      Serial.println("================================");
      Serial.println("          TEST COMPLETE");
      Serial.println("================================");


      state = FINISHED;

      return;
    }


    // --------------------------------------------------
    // Make voltage increasingly negative every 2 sec
    // --------------------------------------------------

    if (millis() - previousStepTime >= STEP_TIME) {

      previousStepTime = millis();

      motor_voltage -= STEP_SIZE;


      Serial.print("Motor Voltage: ");
      Serial.print(motor_voltage, 3);
      Serial.println(" V");


      // ----------------------------------------------
      // Safety cutoff
      // ----------------------------------------------

      if (motor_voltage <= -MAX_TEST_VOLTAGE) {

        motor_voltage = 0.0f;

        Serial.println();
        Serial.println("MAXIMUM NEGATIVE TEST VOLTAGE REACHED.");
        Serial.println("Motor stopped for safety.");
        Serial.println("Test terminated.");

        state = FINISHED;
      }
    }
  }



  // ====================================================
  // 4. TEST FINISHED
  // ====================================================

  else if (state == FINISHED) {

    // Ensure motor remains off
    motor_voltage = 0.0f;
  }
}


// ======================================================
// geeWhiz CONTROL INTERRUPT
// ======================================================

void interval_control_code(void) {

  digitalWrite(A5, HIGH);

  // Continuously apply whichever voltage the
  // main test program currently requests.
  setMotorVoltage(motor_voltage);

  digitalWrite(A5, LOW);
}