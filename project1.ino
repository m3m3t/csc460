/* 
 Controlling a servo position using a potentiometer (variable resistor) 
 by Michal Rinott <http://people.interaction-ivrea.it/m.rinott> 

 modified on 8 Nov 2013
 by Scott Fitzgerald
 http://arduino.cc/en/Tutorial/Knob
*/

#include <Servo.h> 
 
Servo myservo;  // create servo object to control a servo 
 
int potpin = 0;  // analog pin used to connect the potentiometer
int val;    // variable to read the value from the analog pin 
int button = 52

uint8_t irpin = 3;
 
int send = 7;
int[] bit = [0,0,0,1,1,1,1,1]
 
 
void pulse_bit(){
    if(send > 0){
        if(bit[send] == 1){
            digitalWrite(irpin, HIGH);
        }else{
          digitalWrite(irpin, LOW);
        }
        send --;
    }
}
void pulse_low(){
    digitalWrite(irpin, HIGH)
}
void send(uint8_t val){
    Scheduler_StartTask(0, 1000, pulse_bit);
    Scheduler_StartTask(500, 1000, pulse_low);
} 
 
void idle(uint32_t idle_period){
  val = analogRead(potpin);            // reads the value of the potentiometer (value between 0 and 1023) 
  val = map(val, 0, 1023, 0, 180);     // scale it to use it with the servo (value between 0 and 180) 
  //myservo.write(val);                  // sets the servo position according to the scaled value 
  buttonState = digitalRead(button);
  if(buttonState == HIGH){
      send(63);
  }
  delay(idle_period);   
}

void setup() 
{ 
  myservo.attach(9);  // attaches the servo on pin 9 to the servo object 
  
  pinMode(button, INPUT);
  pinMode(irpin, OUTPUT);
  
  Scheduler_Init();
} 
 
void loop() 
{ 
    uint32_t idle_period = Scheduler_Dispatch();
    if(idle_period){
        idle(idle_period);
    }
} 
