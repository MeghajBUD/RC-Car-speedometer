void setup() {
  Serial.begin(115200);  // USB → Serial Monitor
  Serial1.begin(9600);   // GPS UART
}

void loop() {
  if (Serial1.available() > 0) {
    char c = Serial1.read();
    Serial.print(c);      // forward raw NMEA to Serial Monitor
  }
}