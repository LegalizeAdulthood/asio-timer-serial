/*
  Based on Analog Input example
*/

int sensorPin = A0;    // select the input pin for the potentiometer
int ledPin = 13;       // select the pin for the LED

void setup()
{
  pinMode(ledPin, OUTPUT);
  Serial.begin(9600);
}

bool on = true;

void loop()
{
  int sensorValue = analogRead(sensorPin);
  Serial.println(sensorValue);
  digitalWrite(ledPin, on ? HIGH : LOW);
  on = !on;
  delay(100);
}
