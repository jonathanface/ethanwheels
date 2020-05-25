#include <Cytron_SmartDriveDuo.h>
#include <SoftwareSerial.h>

//Motor pins
#define BLE_MOTOR_A_INPUT_1 4 
#define BLE_MOTOR_B_INPUT_1 5 
#define BLE_MOTOR_B_INPUT_2 6 
#define BLE_MOTOR_A_INPUT_2 7

//BLE pins
#define RX 0
#define TX 1

// LED pins
#define LED_BUTTON_INPUT A5
#define HAZARDS_SWITCH  A3
#define LED_OUTPUTS_SIZE 2

const int LED_OUTPUTS[LED_OUTPUTS_SIZE] = {A0, A1};
const int HAZARD_LIGHTS_INTERVAL = 500; //ms
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
  COMMAND_STOP
};

// Globals to keep track of settings
int lightSwitchState = LOW;
bool inHazardsMode = false;
char appData;  
String inData = "";
bool bleConnected = false;
int maxMotorSpeed = 25;
int currentDirection = COMMAND_FORWARD;

String currentHeading = "F";



Cytron_SmartDriveDuo bleMotor(PWM_INDEPENDENT, BLE_MOTOR_A_INPUT_1, BLE_MOTOR_A_INPUT_2, BLE_MOTOR_B_INPUT_1, BLE_MOTOR_B_INPUT_2);
SoftwareSerial HM10(RX, TX);

void processBLECommand(String data) {
  consolePrint("prcessing " + data);
  char str_array[data.length()];
  int parameterIndex = data.indexOf("=");
  int command;
  String value;
  consolePrint("found eq at " + String(parameterIndex));
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
  consolePrint("Command: " + String(command) + " VS " + String(COMMAND_SPEED_CHANGE));

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
      Serial.println(response);
      break;
    }
    case COMMAND_DISCONNECT: {
      bleConnected = false;
      disableMotors();
      break;
    }
    case COMMAND_SPEED_CHANGE: {
      consolePrint("VAL " + value);
      maxMotorSpeed = value.toInt();
      break;
    }
    case COMMAND_FORWARD: {
      currentDirection = command;
      break;
    }
    case COMMAND_REVERSE: {
      currentDirection = command;
      break;
    }
    case COMMAND_LEFT: {
      currentDirection = command;
      break;
    }
    case COMMAND_RIGHT: {
      currentDirection = command;
      break;
    }
    case COMMAND_STOP: {
      currentDirection = command;
      disableMotors();
      break;
    }
  }
}

void consolePrint(String message) {
  Serial.println(message + "|");
  delay(1);
}

void BLEListener() {
  HM10.listen();  // listen the HM10 port
  
  String inData = "";
  while (HM10.available() > 0) {   // if HM10 sends something then read
    if (!bleConnected) {
      bleConnected = true;
      //consolePrint("BLE Connected!");
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
  pinMode(13, OUTPUT);
  HM10.begin(9600);
  Serial.begin(9600);
  // set all the motor control pins to outputs
  bleMotorInit();

  // Set all the LEDs to output mode.
  for (int i=0; i < LED_OUTPUTS_SIZE; i++) {
    pinMode(LED_OUTPUTS[i], OUTPUT);
  }

  // Hazard Light switch to input mode.
  pinMode(HAZARDS_SWITCH, INPUT);
}

void testMotor() {
  bleMotor.control(50, 0);
  delay(2000); // Delay for 5 seconds.
  disableMotors();
}

void bleMotorInit() {
  digitalWrite(13, HIGH);
  delay(2000); // Delay for 5 seconds.
  digitalWrite(13, LOW);
  disableMotors();
}
void disableMotors() {
  //consolePrint("turning off all motors");
  bleMotor.control(0, 0);
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
// Each headlight toggles on and off in alternating
// sequence while in hazard mode
void runHazardsSequence() {
  Serial.println("in hazards routine");
  digitalWrite(LED_OUTPUTS[1], 0);
  digitalWrite(LED_OUTPUTS[0], 1);
  delay(HAZARD_LIGHTS_INTERVAL);
  digitalWrite(LED_OUTPUTS[1], 1);
  digitalWrite(LED_OUTPUTS[0], 0);
  delay(HAZARD_LIGHTS_INTERVAL);
}

void toggleAllLights(int dir) {
    for (int i=0; i < LED_OUTPUTS_SIZE; i++) {
      digitalWrite(LED_OUTPUTS[i], dir);
    }
}

void loop()
{
  BLEListener();
  /*
  int actualLightSwitchState = digitalRead(LED_BUTTON_INPUT);
  if (digitalRead(HAZARDS_SWITCH) == HIGH && !inHazardsMode) {
    consolePrint("Hazard lights active");
    inHazardsMode = true;
    runHazardsSequence();
    return;
  } else if (digitalRead(HAZARDS_SWITCH) == LOW && inHazardsMode) {
    inHazardsMode = false;
    lightSwitchState = actualLightSwitchState;
    toggleAllLights(actualLightSwitchState);
  }
  if (actualLightSwitchState != lightSwitchState) {
    lightSwitchState = actualLightSwitchState;
    toggleAllLights(lightSwitchState);
  }*/
}
