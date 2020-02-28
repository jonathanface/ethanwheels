
#include <SoftwareSerial.h>
SoftwareSerial HM10(0, 1); // RX = 2, TX = 3
char appData;  
String inData = "";

const int LED_BUTTON_INPUT = 8;
const int OUTPUTS_SIZE = 4;
const int LED_OUTPUTS[OUTPUTS_SIZE] = {A0,A1,A2,A3};

const int MOTOR_ENABLE_A = 5;
const int MOTOR_ENABLE_B = 6;
const int MOTOR_A_DIR1 = 12;
const int MOTOR_A_DIR2 = 13;

int bleMotorSpeed = 0;

int buttonState = 0;
int lightsOnState = 0;
bool buttonChanged = false;

String getValue(String data, char separator, int index)
{
    int found = 0;
    int strIndex[] = { 0, -1 };
    int maxIndex = data.length() - 1;

    for (int i = 0; i <= maxIndex && found <= index; i++) {
        if (data.charAt(i) == separator || i == maxIndex) {
            found++;
            strIndex[0] = strIndex[1] + 1;
            strIndex[1] = (i == maxIndex) ? i+1 : i;
        }
    }
    return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

void BLEListener() {
  HM10.listen();  // listen the HM10 port
  String inData = "";
  while (HM10.available() > 0) {   // if HM10 sends something then read
    delay(1);
    appData = HM10.read();
    inData += String(appData);  // save the data in string format
  }
  if (inData.length()) {
    Serial.print("Got: " + inData);
    String speedval = getValue(inData, 'speed_', 1);
    bleMotorSpeed = speedval.toInt();
    Serial.print("speed " + bleMotorSpeed);
    Serial.println();
    inData = "";
  }
}

void runMotorBLEMotors()
{
  
  // this function will run the motors in both directions at a fixed speed
  // turn on motor A
  /*
  digitalWrite(MOTOR_A_DIR1, HIGH);
  digitalWrite(MOTOR_A_DIR2, LOW);
  // set speed to 200 out of possible range 0~255
  analogWrite(MOTOR_ENABLE_A, 30);
  delay(2000);
  Serial.print("spun one way");
  Serial.println();
  // now change motor directions
  digitalWrite(MOTOR_A_DIR1, LOW);
  digitalWrite(MOTOR_A_DIR2, HIGH); 
  delay(2000);
  Serial.print("spun the other way");
  Serial.println();
  // now turn off motors
  digitalWrite(MOTOR_A_DIR1, LOW);
  digitalWrite(MOTOR_A_DIR2, LOW); */
}

void setup() {
  HM10.begin(9600);
  Serial.begin(9600);
  Serial.print("hello");
  Serial.println();

  
  
  for (int i=0; i < OUTPUTS_SIZE; i++) {
    pinMode(LED_OUTPUTS[i], OUTPUT);
    digitalWrite(LED_OUTPUTS[i], LOW);
  }
  pinMode(LED_BUTTON_INPUT, INPUT);
  digitalWrite(LED_BUTTON_INPUT, LOW);

  pinMode(MOTOR_ENABLE_A, OUTPUT);
  pinMode(MOTOR_A_DIR1, OUTPUT);
  pinMode(MOTOR_A_DIR2, OUTPUT);

  

  for(;;) {

    BLEListener();
    //demoOne();
    //delay(1000);
    //continue;
    
    int newState = digitalRead(LED_BUTTON_INPUT);
    //Serial.print("state " + String(newState));
    //Serial.println();
    if (newState != buttonState && !buttonChanged) {
      //button is pressed in
      buttonChanged = true;
      buttonState = newState;
      continue;
    }
    if (newState != buttonState && buttonChanged) {
      //button is released
      buttonChanged = false;
      lightsOnState = !lightsOnState;
      for (int i=0; i < OUTPUTS_SIZE; i++) {
        Serial.print("updating button state on pin " + String(LED_OUTPUTS[i]) + " to " + String(lightsOnState));
        Serial.println();
        digitalWrite(LED_OUTPUTS[i], lightsOnState);
      }
      buttonState = newState;
    }
  }
}

void loop() {
  
}
