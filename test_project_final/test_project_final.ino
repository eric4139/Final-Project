const int BUZZER = 2;

void setup() {
  pinMode(BUZZER, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);   // la petite LED "L" de la carte
  Serial.begin(9600);
  Serial.println("Demarrage OK");
}

void loop() {
  digitalWrite(LED_BUILTIN, HIGH);   // allume la LED pendant le bip
  Serial.println("Bip");

  // Fait vibrer le buzzer "à la main" pendant 1 seconde (500 Hz)
  for (int i = 0; i < 500; i++) {
    digitalWrite(BUZZER, HIGH);
    delayMicroseconds(1000);
    digitalWrite(BUZZER, LOW);
    delayMicroseconds(1000);
  }

  digitalWrite(LED_BUILTIN, LOW);
  delay(1000);                       // silence 1 seconde
}