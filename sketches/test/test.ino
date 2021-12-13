#include <RunningAverage.h>
#include <elapsedMillis.h>

RunningAverage myRA(30);

void setup() {
  Serial.begin(9600);
  Serial.println('!');
}

void loop() {
  while (Serial.available() > 0) {
    const int read_byte = Serial.read();
    if (read_byte == 'x') {
      myRA.addValue(2);
      Serial.println(myRA.getAverage());
    }
  }
}
