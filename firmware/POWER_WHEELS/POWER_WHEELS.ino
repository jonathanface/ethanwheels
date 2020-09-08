#include <Cytron_SmartDriveDuo.h>
#include <SoftwareSerial.h>
#include <EEPROM.h>

//Motor pins
#define BLE_MOTOR_A_INPUT_1 4 
#define BLE_MOTOR_B_INPUT_1 5 
#define BLE_MOTOR_B_INPUT_2 6 
#define BLE_MOTOR_A_INPUT_2 7

#define PILOT_MOTOR_A_INPUT_1 8 
#define PILOT_MOTOR_B_INPUT_1 9 
#define PILOT_MOTOR_B_INPUT_2 10 
#define PILOT_MOTOR_A_INPUT_2 11

#define PILOT_MOTOR_REVERSE_SWITCH 12

//BLE pins
#define RX 0
#define TX 1

// LED pins
#define LED_BUTTON_INPUT A5
#define HAZARDS_SWITCH  A4
#define LED_OUTPUTS_SIZE 2

const int LED_OUTPUTS[LED_OUTPUTS_SIZE] = {A0, A1};
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

// Globals to keep track of settings
int lightSwitchState = LOW;
unsigned long previousMillis = millis();
bool hazardState = false;
bool inHazardsMode = false;
char appData;  
String inData = "";
bool bleConnected = false;
int maxMotorSpeed = 25;

int currentHeading = COMMAND_STOP;
int previousHeading = COMMAND_STOP;
bool toggleOnLights = false;
bool toggleOffLights = false;
bool toggleOnHazards = false;
bool toggleOffHazards = false;

Cytron_SmartDriveDuo bleMotor(PWM_INDEPENDENT, BLE_MOTOR_A_INPUT_1, BLE_MOTOR_A_INPUT_2, BLE_MOTOR_B_INPUT_1, BLE_MOTOR_B_INPUT_2);
Cytron_SmartDriveDuo pilotMotor(PWM_INDEPENDENT, PILOT_MOTOR_A_INPUT_1, PILOT_MOTOR_A_INPUT_2, PILOT_MOTOR_B_INPUT_1, PILOT_MOTOR_B_INPUT_2);
SoftwareSerial HM10(RX, TX);

void processBLECommand(String data) {
  consolePrint("prcessing " + data);
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
  consolePrint("Command: " + String(command));

  switch(command) {
    case COMMAND_REQUEST_STATUS: {
      String response = String(COMMAND_REQUEST_STATUS_REPLY) + "&speed=" + String(maxMotorSpeed);
      String lightState = "false";
      if (lightSwitchState != LOW) {
        lightState = "true";
      }
      response += "&lights=" + lightState;
      String hazardsState = "false";
      if (inHazardsMode) {
        hazardsState = "true";
      }
      response += "&hazards=" + hazardsState + BLE_DELIMITER;
      consolePrint(response);
      break;
    }
    case COMMAND_DISCONNECT: {
      bleConnected = false;
      disableMotors();
      break;
    }
    case COMMAND_SPEED_CHANGE: {
      maxMotorSpeed = value.toInt();
      EEPROM.write(0, maxMotorSpeed);
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
      disableMotors();
      break;
    }
  }
}

// Special formatting and a brief delay to keep debug
// printing from mixing with the BLE print messages.
void consolePrint(String message) {
  Serial.println(message + BLE_DELIMITER);
  delay(1);
}

void BLEListener() {
  String inData = "";
  while (HM10.available() > 0) {   // if HM10 sends something then read
    if (!bleConnected) {
      bleConnected = true;
      disableMotors();
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
  int tempSpeed = EEPROM.read(0);
  if (tempSpeed != 255) {
    maxMotorSpeed = tempSpeed;
  }
  //pinMode(13, OUTPUT);
  HM10.begin(9600);
  Serial.begin(9600);
  // set all the motor control pins to outputs
  motorControllersInit();

  // Set all the LEDs to output mode.
  for (int i=0; i < LED_OUTPUTS_SIZE; i++) {
    pinMode(LED_OUTPUTS[i], OUTPUT);
  }

  // Hazard Light switch to input mode.
  pinMode(HAZARDS_SWITCH, INPUT);
  pinMode(LED_BUTTON_INPUT, INPUT);
  pinMode(PILOT_MOTOR_REVERSE_SWITCH, INPUT);
  digitalWrite(13, HIGH);

  
  
  HM10.listen();
}

void motorControllersInit() {
  digitalWrite(13, LOW);
  disableMotors();
}
void disableMotors() {
  consolePrint("turning off all motors");
  bleMotor.control(0, 0);
  pilotMotor.control(0, 0);
  digitalWrite(13, LOW);
}

/*
int accelerate(int target, int count) {
  delay(10);
  int divisor = (target/10)*count;
  consolePrint("accelerating to " + String(divisor));
  analogWrite(BLE_MOTOR_A_ENABLE, divisor);
  if (divisor < target) {
    count++;
    return accelerate(target, count);
  }
  return 1;
}
*/

void toggleAllLights(int dir) {
    for (int i=0; i < LED_OUTPUTS_SIZE; i++) {
      digitalWrite(LED_OUTPUTS[i], dir);
    }
}


void loop()
{
  BLEListener();
  //consolePrint("Switch: " + String(digitalRead(HAZARDS_SWITCH)));

  if (bleConnected) {
    if (currentHeading != previousHeading) {
      switch(currentHeading) {
        case COMMAND_FORWARD:
          consolePrint("moving forward");
          bleMotor.control(maxMotorSpeed, maxMotorSpeed);
          digitalWrite(13, HIGH);
          break;
        case COMMAND_LEFT:
          bleMotor.control(maxMotorSpeed, 0);
          break;
        case COMMAND_RIGHT:
          bleMotor.control(0, maxMotorSpeed);
          break;
        case COMMAND_REVERSE:
          bleMotor.control(-maxMotorSpeed, -maxMotorSpeed);
          break;
        case COMMAND_REVERSE_LEFT:
          bleMotor.control(-maxMotorSpeed, 0);
          break;
        case COMMAND_REVERSE_RIGHT:
          bleMotor.control(0, -maxMotorSpeed);
          break;
        default:
          disableMotors();
      }
      previousHeading = currentHeading;
    }
    if (toggleOnLights) {
      toggleAllLights(true);
      toggleOnLights = false;
    }
    if (toggleOffLights) {
      toggleAllLights(false);
      toggleOffLights = false;
    }
    if (toggleOnHazards) {
       unsigned long currentMillis = millis();
       // consolePrint(String(currentMillis) + " " + (previousMillis));
       consolePrint("DIFF : " + String(currentMillis - previousMillis));
       int diff = currentMillis - previousMillis;
       if (diff >= HAZARDS_INTERVAL ) {
         if (!hazardState) {
           digitalWrite(LED_OUTPUTS[1], LOW);
           digitalWrite(LED_OUTPUTS[0], HIGH);
         } else {
           digitalWrite(LED_OUTPUTS[0], LOW);
           digitalWrite(LED_OUTPUTS[1], HIGH);
         }
         hazardState = !hazardState;
         previousMillis = currentMillis;
      }
    }
    return;
  }

  /*
   * 
   * pilot motor controll should go here
   */
  
  int actualLightSwitchState = digitalRead(LED_BUTTON_INPUT);
  if (digitalRead(HAZARDS_SWITCH) == HIGH && !inHazardsMode) {
   // consolePrint("Hazard lights active");
    inHazardsMode = true;
    previousMillis = millis();
  } else if (digitalRead(HAZARDS_SWITCH) == LOW && inHazardsMode) {
    //consolePrint("Hazard lights inactive");
    inHazardsMode = false;
    
    lightSwitchState = actualLightSwitchState;
    toggleAllLights(actualLightSwitchState);
  }
  if (actualLightSwitchState != lightSwitchState) {
    lightSwitchState = actualLightSwitchState;
    toggleAllLights(lightSwitchState);
  }
  
  if (inHazardsMode) {
    unsigned long currentMillis = millis();
   // consolePrint(String(currentMillis) + " " + (previousMillis));
    consolePrint("DIFF : " + String(currentMillis - previousMillis));
    int diff = currentMillis - previousMillis;
    if (diff >= HAZARDS_INTERVAL ) {
      if (!hazardState) {
        digitalWrite(LED_OUTPUTS[1], LOW);
        digitalWrite(LED_OUTPUTS[0], HIGH);
      } else {
        digitalWrite(LED_OUTPUTS[0], LOW);
        digitalWrite(LED_OUTPUTS[1], HIGH);
      }
      hazardState = !hazardState;
      previousMillis = currentMillis;
    }
  }
}
