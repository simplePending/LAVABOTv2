#define IR_SENSOR_PIN 2
#define SOUND_SENSOR_PIN A0
#define TAP_SOLENOID_PIN 8
#define MALAUHOG_PIN 9
#define MALAKATAD_PIN 10
#define MALAKANIN_PIN 11

bool sensorActive = false;

void setup() {
  Serial.begin(9600);

  pinMode(IR_SENSOR_PIN, INPUT_PULLUP);
  pinMode(TAP_SOLENOID_PIN, OUTPUT);
  pinMode(MALAUHOG_PIN, OUTPUT);
  pinMode(MALAKATAD_PIN, OUTPUT);
  pinMode(MALAKANIN_PIN, OUTPUT);
}

int readSoundEnergy() {
  long sum = 0;
  int samples = 300;
  int minVal = 1023;
  int maxVal = 0;

  for (int i = 0; i < samples; i++) {
    int val = analogRead(A0);

    if (val < minVal) minVal = val;
    if (val > maxVal) maxVal = val;

    delayMicroseconds(200);
  }

  return maxVal - minVal;  // Peak-to-peak amplitude
}

void activateTapAndMeasure() {
  // Tap
  digitalWrite(TAP_SOLENOID_PIN, HIGH);
  delay(80);
  digitalWrite(TAP_SOLENOID_PIN, LOW);
  delay(50);

  // Measure sound
  int energy = readSoundEnergy();

  Serial.print("SOUND:");
  Serial.println(energy);

  Serial.println("TAP_DONE");
}

void pulse(int pin) {
  digitalWrite(pin, HIGH);
  delay(300);
  digitalWrite(pin, LOW);
}

void loop() {
  int ir = digitalRead(IR_SENSOR_PIN);

  if (ir == LOW && !sensorActive) {
    sensorActive = true;
    Serial.println("DETECTED");
    activateTapAndMeasure();
  }

  if (ir == HIGH && sensorActive) {
    sensorActive = false;
  }

  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();

    if (cmd == "MALAUHOG") pulse(MALAUHOG_PIN);
    else if (cmd == "MALAKATAD") pulse(MALAKATAD_PIN);
    else if (cmd == "MALAKANIN") pulse(MALAKANIN_PIN);
  }
}
