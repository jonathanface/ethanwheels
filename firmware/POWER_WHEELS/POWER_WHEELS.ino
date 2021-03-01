
#include <Cytron_SmartDriveDuo.h>

#define GAS_PEDAL_INPUT 11
#define HEADLIGHTS_SWITCH_INPUT A5
#define DIRECTION_SWITCH_INPUT 3
#define MOTOR_A_INPUT_1 4
#define MOTOR_A_INPUT_2 7
#define MOTOR_B_INPUT_1 5 
#define MOTOR_B_INPUT_2 6 
#define HAZARDS_SWITCH_INPUT 9

#define LED_OUTPUTS_SIZE 2

const int LED_OUTPUTS[LED_OUTPUTS_SIZE] = {A0, A1};
const int HAZARDS_INTERVAL = 500; //ms
const char* BLE_DELIMITER = "|";

int headlightsState, lastHeadlightsState;
int reverseState, lastReverseState;
int hazardsState, lastHazardsState;
int gasState, lastGasState;
int previousHazardsMillis = 0;
bool hazardsSequence = false;

unsigned long debounceDelay = 50;
unsigned long lastHeadlightsDebounceTime = 0, lastReverseDebounceTime = 0,
    lastHazardsDebounceTime = 0, lastGasDebounceTime = 0;

int maxMotorSpeed = 25;
Cytron_SmartDriveDuo motor(PWM_INDEPENDENT, MOTOR_A_INPUT_1, MOTOR_A_INPUT_2, MOTOR_B_INPUT_1, MOTOR_B_INPUT_2);

void setup()
{
  Serial.begin(9600);

  pinMode(GAS_PEDAL_INPUT, INPUT_PULLUP);
  pinMode(HEADLIGHTS_SWITCH_INPUT, INPUT);
  pinMode(DIRECTION_SWITCH_INPUT, INPUT_PULLUP);
  pinMode(HAZARDS_SWITCH_INPUT, INPUT_PULLUP);

  //toggles and buttons can be left on in various states so read in initial values
  headlightsState = lastHeadlightsState = digitalRead(HEADLIGHTS_SWITCH_INPUT);
  reverseState = lastReverseState = digitalRead(DIRECTION_SWITCH_INPUT);
  hazardsState = lastHazardsState = digitalRead(HAZARDS_SWITCH_INPUT);
  //gas should always initialize unpressed regardless of pedal state
  gasState = lastGasState = LOW;
}

void toggleAllLights(int dir) {
  for (int i=0; i < LED_OUTPUTS_SIZE; i++) {
    digitalWrite(LED_OUTPUTS[i], dir);
  }
}

void runHazardsSequence() {
  unsigned long currentMillis = millis();
  // consolePrint(String(currentMillis) + " " + (previousMillis));
  // consolePrint("DIFF : " + String(currentMillis - previousMillis));
  int diff = currentMillis - previousHazardsMillis;
  if (diff >= HAZARDS_INTERVAL ) {
    if (!hazardsSequence) {
      digitalWrite(LED_OUTPUTS[1], LOW);
      digitalWrite(LED_OUTPUTS[0], HIGH);
    } else {
      digitalWrite(LED_OUTPUTS[0], LOW);
      digitalWrite(LED_OUTPUTS[1], HIGH);
    }
    hazardsSequence = !hazardsSequence;
    previousHazardsMillis = currentMillis;
  }
}


void loop()
{
  //BLE STUFF HERE WHICH EXITS THE FUNCTION IF CONNECTED

  int gasReading = digitalRead(GAS_PEDAL_INPUT);
  int headlightsReading = digitalRead(HEADLIGHTS_SWITCH_INPUT);
  int reverseReading = digitalRead(DIRECTION_SWITCH_INPUT);
  int hazardsReading = digitalRead(HAZARDS_SWITCH_INPUT);

  if (gasReading != lastGasState) {
    lastGasDebounceTime = millis();
  }
  if (headlightsReading != lastHeadlightsState) {
    lastHeadlightsDebounceTime = millis();
  }
  if (reverseReading != lastReverseState) {
    lastReverseDebounceTime = millis();
  }
  if (hazardsReading != lastHazardsState) {
    lastHazardsDebounceTime = millis();
  }

  if ((millis() - lastGasDebounceTime) > debounceDelay) {
    if (gasReading != gasState) {
      gasState = gasReading;
      if (gasState == HIGH) {
        Serial.println("giving gas");
      } else {
        Serial.println("let off gas");
      }
    }
  }

  if ((millis() - lastHeadlightsDebounceTime) > debounceDelay) {
    if (headlightsReading != headlightsState) {
      headlightsState = headlightsReading;
      //toggleAllLights(headlightsState);
      if (headlightsState == HIGH) {
        Serial.println("lights on");
      } else {
        Serial.println("lights off");
      }
    }
  }

  if ((millis() - lastReverseDebounceTime) > debounceDelay) {
    if (reverseReading != reverseState) {
      reverseState = reverseReading;
      if (gasState) {
        if (reverseState == LOW) {
          //moving forward
          Serial.println("Direction forward");
          motor.control(maxMotorSpeed, maxMotorSpeed);
        } else {
          Serial.println("direction reverse");
          motor.control(-maxMotorSpeed, -maxMotorSpeed);
        }
      } else {
        Serial.println("directional change but gas pedal off");
      }
    }
  }

  if ((millis() - lastHazardsDebounceTime) > debounceDelay) {
    if (hazardsReading != hazardsState) {
      hazardsState = hazardsReading;
      if (hazardsState == LOW) {
        Serial.println("haz on");
        previousHazardsMillis = millis();
      } else {
        Serial.println("haz off");
        toggleAllLights(headlightsState);
      }
    }
  }

  lastGasState = gasReading;
  lastHeadlightsState = headlightsReading;
  lastReverseState = reverseReading;
  lastHazardsState = hazardsReading;

  if (hazardsState) {
    runHazardsSequence();
  }
}
