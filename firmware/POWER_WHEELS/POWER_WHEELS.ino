
#include <Cytron_SmartDriveDuo.h>
#include <EEPROM.h>

#define BLE_RX 0
#define BLE_TX 1
#define GAS_PEDAL_INPUT 11
#define HEADLIGHTS_SWITCH_INPUT A5
#define DIRECTION_SWITCH_INPUT 2
#define MOTOR_A_INPUT_1 4
#define MOTOR_A_INPUT_2 6
#define MOTOR_B_INPUT_1 9
#define MOTOR_B_INPUT_2 10
#define HAZARDS_SWITCH_INPUT 7

#define LED_OUTPUTS_SIZE 2

const int LED_OUTPUTS[LED_OUTPUTS_SIZE] = {A0, A2};
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
  int tempSpeed;
  EEPROM.get(0, tempSpeed);
  Serial.println("initial speed " + String(tempSpeed));
  if (tempSpeed != -1) {
    maxMotorSpeed = int(tempSpeed);
  }
  Serial.println("max speed " + String(maxMotorSpeed));
  motor.control(0, 0);
  
  pinMode(GAS_PEDAL_INPUT, INPUT_PULLUP);
  pinMode(HEADLIGHTS_SWITCH_INPUT, INPUT);
  pinMode(DIRECTION_SWITCH_INPUT, INPUT_PULLUP);
  pinMode(HAZARDS_SWITCH_INPUT, INPUT_PULLUP);

  for (int i=0; i < LED_OUTPUTS_SIZE; i++) {
    pinMode(LED_OUTPUTS[i], OUTPUT);
  }

  

  //toggles and buttons can be left on in various states so read in initial values
  headlightsState = lastHeadlightsState = digitalRead(HEADLIGHTS_SWITCH_INPUT);
  toggleAllLights(headlightsState);
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

/**
 *  Just a test function, sending output power to 
 *  all pins so I can verify with a multimeter that all my
 *  board pin connections at least have a signal
 */
void powerTest() {
  pinMode(GAS_PEDAL_INPUT, OUTPUT);
  pinMode(BLE_TX, OUTPUT);
  pinMode(BLE_RX, OUTPUT);
  pinMode(HEADLIGHTS_SWITCH_INPUT, OUTPUT);
  pinMode(DIRECTION_SWITCH_INPUT, OUTPUT);
  pinMode(HAZARDS_SWITCH_INPUT, OUTPUT);
  pinMode(MOTOR_A_INPUT_1, OUTPUT);
  pinMode(MOTOR_A_INPUT_2, OUTPUT);
  pinMode(MOTOR_B_INPUT_1, OUTPUT);
  pinMode(MOTOR_B_INPUT_2, OUTPUT);
  
  digitalWrite(GAS_PEDAL_INPUT, HIGH);
  digitalWrite(BLE_TX, HIGH);
  digitalWrite(BLE_RX, HIGH);
  digitalWrite(HEADLIGHTS_SWITCH_INPUT, HIGH);
  digitalWrite(DIRECTION_SWITCH_INPUT, HIGH);
  digitalWrite(HAZARDS_SWITCH_INPUT, HIGH);
  digitalWrite(MOTOR_A_INPUT_1, HIGH);
  digitalWrite(MOTOR_A_INPUT_2, HIGH);
  digitalWrite(MOTOR_B_INPUT_1, HIGH);
  digitalWrite(MOTOR_B_INPUT_2, HIGH);
  digitalWrite(LED_OUTPUTS[0], HIGH);
  digitalWrite(LED_OUTPUTS[1], HIGH);
}

void loop()
{
  //powerTest();

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

  

  if ((millis() - lastHeadlightsDebounceTime) > debounceDelay) {
    if (headlightsReading != headlightsState) {
      headlightsState = headlightsReading;
      toggleAllLights(headlightsState);
    }
  }

  if ((millis() - lastReverseDebounceTime) > debounceDelay) {
    if (reverseReading != reverseState) {
      reverseState = reverseReading;
      if (reverseState == LOW) {
        //moving forward
        Serial.println("Direction forward");
        //
      } else {
        Serial.println("direction reverse");
       // 
      }
    }
  }

  if ((millis() - lastGasDebounceTime) > debounceDelay) {
    if (gasReading != gasState) {
      gasState = gasReading;
      if (gasState == HIGH) {
        Serial.println("giving gas");
        if (reverseState == LOW) {
          //motor.control(maxMotorSpeed, maxMotorSpeed);
        } else {
          //motor.control(-maxMotorSpeed, -maxMotorSpeed);
        }
      } else {
        Serial.println("let off gas");
        //motor.control(0, 0);
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
      }
      toggleAllLights(headlightsState);
    }
  }

  lastGasState = gasReading;
  lastHeadlightsState = headlightsReading;
  lastReverseState = reverseReading;
  lastHazardsState = hazardsReading;

  if (!hazardsState) {
    runHazardsSequence();
  }
}
