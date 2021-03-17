
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
//BLE pins
#define RX 0
#define TX 1

#define LED_OUTPUTS_SIZE 2

const int LED_OUTPUTS[LED_OUTPUTS_SIZE] = {A0, A2};
const int HAZARDS_INTERVAL = 500; //ms
const char* BLE_DELIMITER = "|";

enum communication {
  COMMAND_REQUEST_STATUS = 100,
  COMMAND_REQUEST_STATUS_REPLY,
  COMMAND_DISCONNECT = 102,
  COMMAND_SPEED_CHANGE = 200,
  COMMAND_FORWARD = 300,
  COMMAND_REVERSE,
  COMMAND_LEFT,
  COMMAND_RIGHT,
  COMMAND_STOP,
  COMMAND_REVERSE_LEFT,
  COMMAND_REVERSE_RIGHT,
  COMMAND_LIGHTS_ON = 400,
  COMMAND_LIGHTS_OFF,
  COMMAND_HAZARDS_ON,
  COMMAND_HAZARDS_OFF
};

int headlightsState, lastHeadlightsState;
int reverseState, lastReverseState;
int hazardsState, lastHazardsState;
int gasState, lastGasState;
int previousHazardsMillis = 0;
boolean hazardsPositionalState = false;

bool bleConnected = false;
char appData;  
String inData = "";
int currentHeading = COMMAND_STOP;
int previousHeading = COMMAND_STOP;
int lastBLEHeadlightsState = LOW;

unsigned long debounceDelay = 50;
unsigned long lastHeadlightsDebounceTime = 0, lastReverseDebounceTime = 0,
    lastHazardsDebounceTime = 0, lastGasDebounceTime = 0;
int maxMotorSpeed = 25;

Cytron_SmartDriveDuo motor(PWM_INDEPENDENT, MOTOR_A_INPUT_1, MOTOR_A_INPUT_2, MOTOR_B_INPUT_1, MOTOR_B_INPUT_2);
SoftwareSerial HM10(RX, TX);

bool toggleOnLights = false;
bool toggleOffLights = false;
bool toggleOnHazards = false;
bool toggleOffHazards = false;

void processBLECommand(String data) {
  //Serial.println("prcessing " + data);
  char str_array[data.length()];
  int parameterIndex = data.indexOf("=");
  int command;
  String value;
  if (parameterIndex > -1) {
      String parsed = data.substring(0, parameterIndex);
      parsed.trim();
      command = parsed.toInt();
      parsed = data.substring(parameterIndex+1, data.length());
      parsed.trim();
      value = parsed;
  } else {
    data.trim();
    command = data.toInt();
  }
  Serial.println("Command: " + String(command));

  switch(command) {
    case COMMAND_REQUEST_STATUS: {
      String response = BLE_DELIMITER + String(COMMAND_REQUEST_STATUS_REPLY) + "&speed=" + String(maxMotorSpeed);
      String respHeadlightsState = "false";
      if (headlightsState) {
        respHeadlightsState = "true";
      }
      response += "&lights=" + respHeadlightsState;
      String respHazardsState = "false";
      if (!hazardsState) {
        respHazardsState = "true";
      }
      response += "&hazards=" + respHazardsState + BLE_DELIMITER;
      Serial.println(response);
      break;
    }
    case COMMAND_DISCONNECT: {
      bleConnected = false;
      motor.control(0, 0);
      break;
    }
    case COMMAND_SPEED_CHANGE: {
      maxMotorSpeed = value.toInt();
      EEPROM.put(0, maxMotorSpeed);
      break;
    }
    case COMMAND_FORWARD: {
      currentHeading = command;
      break;
    }
    case COMMAND_REVERSE: {
      currentHeading = command;
      break;
    }
    case COMMAND_LEFT: {
      currentHeading = command;
      break;
    }
    case COMMAND_RIGHT: {
      currentHeading = command;
      break;
    }
    case COMMAND_REVERSE_LEFT: {
      currentHeading = command;
      break;
    }
    case COMMAND_REVERSE_RIGHT: {
      currentHeading = command;
      break;
    }
    case COMMAND_LIGHTS_ON: {
      toggleOnLights = true;
      break;
    }
    case COMMAND_LIGHTS_OFF: {
      toggleOffLights = true;
      break;
    }
    case COMMAND_HAZARDS_ON: {
      toggleOnHazards = true;
      break;
    }
    case COMMAND_HAZARDS_OFF: {
       toggleOffHazards = true;
      break;
    }
    case COMMAND_STOP: {
      currentHeading = command;
      motor.control(0, 0);
      break;
    }
  }
}

void BLEListener() {
  String inData = "";
  while (HM10.available() > 0) {   // if HM10 sends something then read
    if (!bleConnected) {
      bleConnected = true;
      motor.control(0, 0);
    }
    appData = HM10.read();
    if (String(appData) != String(BLE_DELIMITER)) {
      inData += String(appData);  // save the data in string format
      delay(1);
    } else {
      if (inData.length()) {
        processBLECommand(inData);
        inData = "";
      }
    }
  }
}

void setup()
{
  Serial.begin(9600);
  int tempSpeed;
  EEPROM.get(0, tempSpeed);
  Serial.println("initial speed " + String(tempSpeed));
  if (tempSpeed >= 0) {
    maxMotorSpeed = int(tempSpeed);
  }
  Serial.println("max speed " + String(maxMotorSpeed));
  HM10.begin(9600);
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

  HM10.listen();
}

void toggleAllLights(int dir) {
  for (int i=0; i < LED_OUTPUTS_SIZE; i++) {
    digitalWrite(LED_OUTPUTS[i], dir);
  }
}

void runHazardsSequence() {
  Serial.println("in haz seq");
  unsigned long currentMillis = millis();
  int diff = currentMillis - previousHazardsMillis;
  if (diff >= HAZARDS_INTERVAL ) {
    if (!hazardsPositionalState) {
      digitalWrite(LED_OUTPUTS[1], LOW);
      digitalWrite(LED_OUTPUTS[0], HIGH);
    } else {
      digitalWrite(LED_OUTPUTS[0], LOW);
      digitalWrite(LED_OUTPUTS[1], HIGH);
    }
    hazardsPositionalState = !hazardsPositionalState;
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

  BLEListener();

  if (bleConnected) {
    if (currentHeading != previousHeading) {
      switch(currentHeading) {
        case COMMAND_FORWARD:
          Serial.println("moving forward");
          motor.control(maxMotorSpeed, maxMotorSpeed);
          break;
        case COMMAND_LEFT:
          motor.control(maxMotorSpeed, 0);
          break;
        case COMMAND_RIGHT:
          motor.control(0, maxMotorSpeed);
          break;
        case COMMAND_REVERSE:
          motor.control(-maxMotorSpeed, -maxMotorSpeed);
          break;
        case COMMAND_REVERSE_LEFT:
          motor.control(-maxMotorSpeed, 0);
          break;
        case COMMAND_REVERSE_RIGHT:
          motor.control(0, -maxMotorSpeed);
          break;
        default:
          motor.control(0, 0);
      }
      previousHeading = currentHeading;
    }
    if (toggleOnLights) {
      toggleAllLights(HIGH);
      lastBLEHeadlightsState = HIGH;
      toggleOnLights = false;
    }
    if (toggleOffLights) {
      toggleAllLights(LOW);
      lastBLEHeadlightsState = LOW;
      toggleOffLights = false;
    }
    if (toggleOnHazards) {
      runHazardsSequence();
    }
    if (toggleOffHazards) {
      toggleOnHazards = false;
      toggleOffHazards = false;
      toggleAllLights(lastBLEHeadlightsState);
    }
    return;
  }

  int gasReading = digitalRead(GAS_PEDAL_INPUT);
  int headlightsReading = digitalRead(HEADLIGHTS_SWITCH_INPUT);
  int reverseReading = digitalRead(DIRECTION_SWITCH_INPUT);
  int hazardsReading = digitalRead(HAZARDS_SWITCH_INPUT);
  //Serial.println("headlights status: " + String(headlightsReading));
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
    //Serial.println("curr reading " + String(headlightsReading) + ", last " + String(headlightsReading));
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
        //moving forward
        Serial.println("hazOn");
        //
      } else {
        Serial.println("hazOff");
        toggleAllLights(headlightsState);
       // 
      }
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
