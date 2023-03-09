#include "RunningAverage.h"

#define ENABLE_PIN 8
#define DIRECTION_PIN 9
#define PWM_PIN 10
#define FEEDBACK_PIN 11
#define analogResolution 16
#define STRAINGAUGE_PIN 14
#define POT_PIN 15

// get PWM from desired grams: y = a*x + b
//float a = 19.466;
//float b = 84.972;
float a = 18.793;
float b = 32.662;
float PWM_offset = 0;
float PWM_COEF[] = {a, b, PWM_offset};


// get grams from strain gauge: y = a*x^6 + b*x^5 ...
//float aa = -1.9808e-12;
//float bb = 4.3003e-9;
//float cc = -3.6026e-6;
//float dd = 1.4185e-3;
//float ee = -2.3771e-1;
//float ff = 2.3673e1;
//float gg = 7.8009e2;
// these are to calculate the predicted PWM from strain gauge reading
float aaa = -3.1578e-19;
float bbb = 1.265e-14;
float ccc = -2.0307e-10;
float ddd = 1.6669e-6;
float eee = -7.3641e-3;
float fff = 1.7537e1;
float ggg = -1.3234e4;
float Strain_COEF[] = {aaa, bbb, ccc, ddd, eee, fff, ggg};

// these are to calculate the predicted grams from the strain gauge reading
float aa = -1.4799e-20;
float bb = 6.0668e-16;
float cc = -9.9804e-12;
float dd = 8.4016e-8;
float ee = -3.8043e-4;
float ff = 9.2546e-1;
float gg = -7.0872e2;
float STAIN_COEF[] = {aa, bb, cc, dd, ee, ff, gg};

int command_grams = 0;
float current_average = 0;
float calculated_grams;
float calculated_pwm;

int DIRECTION;

RunningAverage myRA(30);
int samples = 0;


void setup() {

DIRECTION = 1;
  
Serial.begin(9600);

analogReadAveraging(32);
analogReadResolution(analogResolution);
analogWriteResolution(analogResolution);

pinMode(ENABLE_PIN, OUTPUT);
pinMode(DIRECTION_PIN, OUTPUT);
pinMode(PWM_PIN, OUTPUT);
pinMode(FEEDBACK_PIN, INPUT);
//digitalWrite(ENABLE_PIN, 1);
digitalWrite(ENABLE_PIN, 1);
digitalWrite(DIRECTION_PIN, DIRECTION);
analogWrite(PWM_PIN, 0);

}

int PWM_VALUE;
int ENABLE;
int FEEDBACK;
int incomingByte;
int analog_val;

elapsedMillis SinceRead;
unsigned long Read_interval = 2;
elapsedMillis SinceReport;
unsigned long Report_interval = 200;
int MEASURED_GRAMS;
int STRAINGAUGE_VALUE;

void loop() {

if(SinceRead >= Read_interval) {
  myRA.addValue(analogRead(STRAINGAUGE_PIN));
  SinceRead = 0;
}

//if(SinceReport >= Report_interval) {
//  Serial.println(myRA.getAverage());
//  SinceReport = 0;
//}


while (Serial.available() > 0) {
  incomingByte = Serial.read();
  if(incomingByte == 'x') {
    ENABLE = 1;
    Serial.println("enabled");
  }
  if(incomingByte == 'o') {
    ENABLE = 0;
    Serial.println("disabled");
  }
  if(incomingByte == 'P') {
    Serial.println(analogRead(POT_PIN));
    } 
  if(incomingByte == 'f') {
    DIRECTION = 1;
    digitalWrite(DIRECTION_PIN, DIRECTION);
    Serial.println("forward");
  }
  if(incomingByte == 'z') {
    DIRECTION = 0;
    digitalWrite(DIRECTION_PIN, DIRECTION);
    Serial.println("reverse");
  }
  if(incomingByte == 'p') {
    PWM_VALUE = Serial.parseInt();
//    if(PWM_VALUE >= 0) {
//      DIRECTION = 1;
//    }
//    else {
//      DIRECTION = 0;
//    }
 //   digitalWrite(DIRECTION_PIN, DIRECTION);
    analogWrite(PWM_PIN, PWM_VALUE);
 //   digitalWrite(ENABLE_PIN, ENABLE);

    //Serial.print("PWM value: ");
    //Serial.println(PWM_VALUE);
  }

  if(incomingByte == 'g') {
    command_grams = Serial.parseInt();
    PWM_VALUE = PWM_COEF[0]*command_grams + PWM_COEF[1] + PWM_COEF[2];
    Serial.print("target grams: ");
    Serial.print(command_grams);
    Serial.print('\t');
    Serial.print("calculated PWM value: ");
    Serial.println(PWM_VALUE);

//    int last_dir = DIRECTION;
//    if(PWM_VALUE >= 0) {
//      DIRECTION = 1;
//    } else {
//      DIRECTION = 0;
//    }
//
//    if (last_dir != DIRECTION) {
//      digitalWrite(DIRECTION_PIN, DIRECTION);
//    }
  }

  if(incomingByte == 'u') {
    PWM_COEF[2] = Serial.parseFloat();
  }

  if(incomingByte == 's') {
      current_average = myRA.getAverage();
      Serial.print("strain gauge reading: ");
      Serial.print(current_average);
      Serial.print('\t');
      Serial.print("calculated PWM: ");
      calculated_pwm = Strain_COEF[0]*pow(current_average, 6) + Strain_COEF[1]*pow(current_average, 5) + Strain_COEF[2]*pow(current_average, 4) + Strain_COEF[3]*pow(current_average, 3) + Strain_COEF[4]*pow(current_average, 2) + Strain_COEF[5]*current_average + Strain_COEF[6];
      Serial.print(calculated_pwm);
      Serial.print('\t');
      Serial.print("acutal PWM: ");
      Serial.print(PWM_VALUE);
      Serial.print("P: ");
      Serial.println(analogRead(POT_PIN));
  }


  

if(incomingByte == 'a') {
  MEASURED_GRAMS = Serial.parseInt();
}

if(incomingByte == 'r') {
    STRAINGAUGE_VALUE = myRA.getAverage();
    
    Serial.print(PWM_VALUE);
    Serial.print('\t');
    Serial.print(STRAINGAUGE_VALUE);
    Serial.print('\t');
    Serial.println(MEASURED_GRAMS);
  
}

  if (Serial.read() == '\n') { 
//    digitalWrite(DIRECTION_PIN, DIRECTION);
    analogWrite(PWM_PIN, PWM_VALUE);
//    digitalWrite(ENABLE_PIN, ENABLE);
//    MEASURED_GRAMS = Serial.parseInt();
    //STRAINGAUGE_VALUE = myRA.getAverage();
//    
//    Serial.print(PWM_VALUE);
//    Serial.print('\t');
//    Serial.print(STRAINGAUGE_VALUE);
//    Serial.print('\t');
//    Serial.println(MEASURED_GRAMS);
    
    //Serial.print("strain gauge: ");
    //Serial.println(analogRead(STRAINGAUGE_PIN));
  }
}

}
