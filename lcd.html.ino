#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Set LCD I2C address (usually 0x27 or 0x3F)
LiquidCrystal_I2C lcd(0x27, 16, 2);

const int trigPin = 10;
const int echoPin = 11;
const int alertPin = 6; // Signal pin to ESP32-CAM
const int buzzer = 4; // buzzer

void setup() {
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(alertPin, OUTPUT);
  pinMode(buzzer, OUTPUT);
 
  digitalWrite(alertPin, LOW); // Default No Obstacle
   digitalWrite(buzzer, HIGH);
  Serial.begin(9600);

  lcd.begin();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Obstacle System");
  lcd.setCursor(0, 1);
  lcd.print("Initializing...");
  delay(2000);
  lcd.clear();
}

void loop() {
  long duration;
  int distance;

  // Pulse to Ultrasonic
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  duration = pulseIn(echoPin, HIGH, 30000);
  distance = duration * 0.034 / 2;
  Serial.println(distance);

  // LCD Line 1: Distance Display
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Dist: ");
  if (distance > 0 && distance < 100) {
    lcd.print(distance);
    lcd.print(" cm   ");
  } else {
    lcd.print("Out Range");
  }

  // LCD Line 2 & Signal sending
  lcd.setCursor(0, 1);
  if (distance > 0 && distance < 20) {
    // Obstacle detected (<20 cm)
    lcd.print("OBSTACLE! STOP  ");
    digitalWrite(alertPin, HIGH); // Send HIGH Signal to ESP32-CAM
   digitalWrite(buzzer, 0);
   
  } else {
    // Path Clear
    lcd.print("PATH CLEAR      ");
    digitalWrite(alertPin, LOW);  // Send LOW Signal to ESP32-CAM
    digitalWrite(buzzer, HIGH);
  }
 Serial.print("alertPin=");
  Serial.println(alertPin);
  delay(150);
}