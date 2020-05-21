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

// Globals to keep track of settings
int lightSwitchState = LOW;
bool inHazardsMode = false;
char appData;  
String inData = "";
bool bleConnected = false;
int maxMotorSpeed = 25;

const int TEST_SPEED = 100; //0-255
const int HAZARD_LIGHTS_INTERVAL = 500; //ms
const String BLE_TERM_SIGNAL = "OK+LOS";
const String BLE_CONN_SIGNAL = "OK+CONN";

const int COMMAND_SPEEDCHANGE = 100;

String currentHeading = "F";



Cytron_SmartDriveDuo bleMotor(PWM_INDEPENDENT, BLE_MOTOR_A_INPUT_1, BLE_MOTOR_A_INPUT_2, BLE_MOTOR_B_INPUT_1, BLE_MOTOR_B_INPUT_2);
SoftwareSerial HM10(RX, TX);

void BLEListener() {
  HM10.listen();  // listen the HM10 port
  String inData = "";
  while (HM10.available() > 0) {   // if HM10 sends something then read
    delay(1);
    appData = HM10.read();
    inData += String(appData);  // save the data in string format
  }
  if (inData.length()) {
    if (!bleConnected) {
      bleConnected = true;
      consolePrint("BLE Connected!");
      disableMotors();
    }
    char str_array[inData.length()];
    inData.toCharArray(str_array, inData.length());
    Serial.print("Data: ");
    Serial.print(inData);
    Serial.println();

    char* messageTok = strtok(str_array, "|");
    String message = String(messageTok);

    while (message != NULL ) {
      int ind = message.indexOf("=");
      int command = message.substring(0, ind).toInt();
      switch(command) {
        case COMMAND_SPEEDCHANGE:
          maxMotorSpeed = message.substring(ind+1, message.length()).toInt();
          consolePrint("new max speed: " + String(maxMotorSpeed));
          break;
      }
      message = strtok(NULL, "&");
    }
    inData = "";
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
  consolePrint("turning off all motors");
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
  consolePrint("in hazards routine");
  digitalWrite(LED_OUTPUTS[1], 0);
  digitalWrite(LED_OUTPUTS[0], 1);
  delay(HAZARD_LIGHTS_INTERVAL);
  digitalWrite(LED_OUTPUTS[1], 1);
  digitalWrite(LED_OUTPUTS[0], 0);
  delay(HAZARD_LIGHTS_INTERVAL);
}

// Just a helper function b/c I got tired of writing
// println() after everything
void consolePrint(String text) {
  Serial.print(text);
  Serial.println();
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
