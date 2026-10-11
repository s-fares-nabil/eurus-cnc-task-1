

const uint8_t PIN_VOLT   = A0;
const uint8_t PIN_GAS    = A1;
const uint8_t PIN_MOTOR  = 4;
const uint8_t PIN_BUZZER = 2;

// Motor polarity(active low)
const uint8_t MOTOR_ON  = HIGH;
const uint8_t MOTOR_OFF = LOW;

const float DIV_RATIO = 2.5;      // (15k + 10k) / 10k
const float V_ON      = 4.8;      // alarm ON at 8 V
const float V_OFF     = 3.2;      // alarm OFF below 7.8 V
const int   GAS_HYST  = 20;

int  gasThreshold = 400;          
bool manualOn     = false;
bool voltAlarm    = false;
bool gasAlarm     = false;

unsigned long lastSend = 0;
char rxBuf[12];
uint8_t rxLen = 0;

int readAvg(uint8_t pin) {
  int sum = 0;
  for (uint8_t i = 0; i < 4; i++) sum += analogRead(pin);
  return sum / 4;
}

void handleCommand() {
  rxBuf[rxLen] = '\0';
  if (strcmp(rxBuf, "M1") == 0)      manualOn = true;
  else if (strcmp(rxBuf, "M0") == 0) manualOn = false;
  else if (rxBuf[0] == 'T')          gasThreshold = constrain(atoi(rxBuf + 1), 0, 1023);
}

void readSerial() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      if (rxLen > 0) handleCommand();
      rxLen = 0;
    } else if (rxLen < sizeof(rxBuf) - 1) {
      rxBuf[rxLen++] = c;
    }
  }
}

void setup() {
  digitalWrite(PIN_MOTOR, MOTOR_OFF);   \
  digitalWrite(PIN_BUZZER, LOW);
  pinMode(PIN_MOTOR, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  Serial.begin(9600);
}

void loop() {
  readSerial();

  int   gas     = readAvg(PIN_GAS);
  float voltage = readAvg(PIN_VOLT) * (5.0 / 1023.0) * DIV_RATIO;

  if (voltage >= V_ON)       voltAlarm = true;
  else if (voltage < V_OFF)  voltAlarm = false;

  if (gas >= gasThreshold)                  gasAlarm = true;
  else if (gas < gasThreshold - GAS_HYST)   gasAlarm = false;

  bool alarm = voltAlarm || gasAlarm || manualOn;
  digitalWrite(PIN_BUZZER, alarm ? HIGH : LOW);
  digitalWrite(PIN_MOTOR,  alarm ? MOTOR_ON : MOTOR_OFF);

  if (millis() - lastSend >= 200) {
    lastSend = millis();
    uint8_t reason = (voltAlarm ? 1 : 0) | (gasAlarm ? 2 : 0) | (manualOn ? 4 : 0);
    Serial.print(voltage, 2);        Serial.print(',');
    Serial.print(gas);               Serial.print(',');
    Serial.print(alarm ? 1 : 0);     Serial.print(',');
    Serial.print(reason);            Serial.print(',');
    Serial.println(gasThreshold);
  }

  delay(20);   // for lower cpu consumption 
}
