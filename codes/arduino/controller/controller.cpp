#include <Arduino.h>

void setup()
{
	pinMode(LED_BUILTIN, OUTPUT);
	Serial.begin(115200);
}

static int loop_counter = 0;

void loop()
{
	digitalWrite(LED_BUILTIN, ((++loop_counter)&1)?HIGH:LOW);
	delay(10);
	while (Serial.available() > 0) {
		Serial.read();
	}
	const int value = analogRead(A0);
	Serial.print(value, DEC);
	Serial.print("\r\n");
	Serial.flush();
}
