/*#define TAP_PIN 8
#define MALAUHOG_PIN 9
#define MALAKATAD_PIN 10
#define MALAKANIN_PIN 11

void setup() {
  Serial.begin(9600);
  pinMode(TAP_PIN, OUTPUT);
  pinMode(MALAUHOG_PIN, OUTPUT);
  pinMode(MALAKATAD_PIN, OUTPUT);
  pinMode(MALAKANIN_PIN, OUTPUT);
}

void pulse(int pin) {
  digitalWrite(pin, HIGH);
  delay(200);
  digitalWrite(pin, LOW);
}

void loop() {
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();

    if (cmd == "MALAUHOG") pulse(MALAUHOG_PIN);
    else if (cmd == "MALAKATAD") pulse(MALAKATAD_PIN);
    else if (cmd == "MALAKANIN") pulse(MALAKANIN_PIN);
  }
}
*/

// coconut_sorter.ino
#define PHOTOELECTRIC_PIN 2      // Photoelectric sensor input
#define TAP_SOLENOID_PIN 8       // Push-pull solenoid for tapping
#define MALAUHOG_PIN 9           // Indicator/sorter for Malauhog
#define MALAKATAD_PIN 10         // Indicator/sorter for Malakatad
#define MALAKANIN_PIN 11         // Indicator/sorter for Malakanin

unsigned long lastDetection = 0;
bool sensorActive = false;

void setup() {
  Serial.begin(9600);
  
  pinMode(PHOTOELECTRIC_PIN, INPUT);
  pinMode(TAP_SOLENOID_PIN, OUTPUT);
  pinMode(MALAUHOG_PIN, OUTPUT);
  pinMode(MALAKATAD_PIN, OUTPUT);
  pinMode(MALAKANIN_PIN, OUTPUT);
  
  // Ensure all outputs are LOW initially
  digitalWrite(TAP_SOLENOID_PIN, LOW);
  digitalWrite(MALAUHOG_PIN, LOW);
  digitalWrite(MALAKATAD_PIN, LOW);
  digitalWrite(MALAKANIN_PIN, LOW);
}

void pulse(int pin) {
  digitalWrite(pin, HIGH);
  delay(300);
  digitalWrite(pin, LOW);
}

void activateTapSolenoid() {
  // Activate push-pull solenoid to tap coconut
  digitalWrite(TAP_SOLENOID_PIN, HIGH);
  delay(100);  // Tap duration
  digitalWrite(TAP_SOLENOID_PIN, LOW);
  delay(200);  // Wait for vibration to settle
}

void loop() {
  // Check photoelectric sensor
  int sensorState = digitalRead(PHOTOELECTRIC_PIN);
  
  if (sensorState == HIGH && !sensorActive) {
    sensorActive = true;
    lastDetection = millis();
    Serial.println("DETECTED");
  } else if (sensorState == LOW && sensorActive) {
    sensorActive = false;
  }
  
  // Handle serial commands from Python
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();

    if (cmd == "TAP") {
      // Activate solenoid to tap the coconut
      activateTapSolenoid();
      Serial.println("TAP_DONE");
      
    } else if (cmd == "MALAUHOG") {
      pulse(MALAUHOG_PIN);
      Serial.println("SORTED_MALAUHOG");
      
    } else if (cmd == "MALAKATAD") {
      pulse(MALAKATAD_PIN);
      Serial.println("SORTED_MALAKATAD");
      
    } else if (cmd == "MALAKANIN") {
      pulse(MALAKANIN_PIN);
      Serial.println("SORTED_MALAKANIN");
      
    } else if (cmd == "CLOSE") {
      // Cleanup on program exit
      digitalWrite(TAP_SOLENOID_PIN, LOW);
      digitalWrite(MALAUHOG_PIN, LOW);
      digitalWrite(MALAKATAD_PIN, LOW);
      digitalWrite(MALAKANIN_PIN, LOW);
    }
  }
}