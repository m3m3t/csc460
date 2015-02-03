#include "Servo.h" 
#include "scheduler.h"
#include "packet.h"
#include "radio.h"

//transmission protocol
#define SENDING_BITS 8

//servo/joystick constants
#define MAX_SERVO_VAL 170
#define MIN_SERVO_VAL 10
#define MAX_JOY_STICK_VAL 1023
#define MIN_JOY_STICK_VAL 0
#define SERVO_MIDDLE 90 // middle of range for servo

//pins
#define SERVO_PIN 9 //pin for servo
#define IR_PIN 24 // pin for ir transmitter
#define LED_PIN 13 // pin for radio led
#define JOY_STICK_PIN 0 //pin for joystick read
#define BUTTON_PIN 1

volatile uint8_t rxflag = 0; //radio flag

uint8_t station_addr[5] = { 
  0xE4, 0xE4, 0xE4, 0xE4, 0xE4 }; // Receiver address
uint8_t my_addr[5] = { 
  0x98, 0x76, 0x54, 0x32, 0x11 }; // Transmitter address

radiopacket_t packet;//radio packet

int SENDING_CODE[] = {
  0,1,0,0,1,0,0,1}; //8 bit number

Servo myservo;  // create servo object to control a servo 
int current_servo_period = 0;

int servo_val;    // variable to read the value from the analog pin 
int button_state; //button on joystick

int sending_bit = SENDING_BITS;
boolean sending = false;

void send_one(int bit_half){
  if(bit_half == 0){
    digitalWrite(IR_PIN, HIGH);
  }
  else{
    digitalWrite(IR_PIN, LOW);
  }
}

void send_zero(int bit_half){
  if(bit_half == 0){
    digitalWrite(IR_PIN, LOW);
  }
  else{
    digitalWrite(IR_PIN, HIGH);
  }
}

void reset_send(){
  sending_bit = SENDING_BITS;
  sending = false;
  digitalWrite(IR_PIN, LOW);
}

void signal_ir(){
  static int bit_half = 0; // 0 for first half, 1 for second half

  //check if in sending state
  if(sending == true){
    //check if activation bit needs to be sent
    if(sending_bit == SENDING_BITS){
      send_one(bit_half);

    }
    else{
      //which bit and half is to be sent
      if(SENDING_CODE[sending_bit] == 1){
        send_one(bit_half);
      }
      else{
        send_zero(bit_half);
      }
    }
    //update to half of bit to be sent
    if(bit_half == 0){
      bit_half = 1;
    }
    else{
      //if second half was sent, reset bit_half and decrement sending_bit for next bit
      bit_half = 0;
      sending_bit --;

      //all bits been sent
      if (sending_bit < 0){
        reset_send();
      }
    }
  }
}

//on idle that data from servo and joystick
void idle(uint32_t idle_period){
  myservo.write(servo_val);
  delay(idle_period);
}

//read joystick, servo is smooth and stops when joystick released
void read_joy_stick(){
  int delta = 0; //how much to move servo
  int val = analogRead(JOY_STICK_PIN);            // reads the value of the potentiometer (value between 0 and 1023) 

  //move joystick
  if(val < (int)(MAX_JOY_STICK_VAL * 0.25f)){
    delta = -2;
  }
  else if(val < (int)(MAX_JOY_STICK_VAL *0.45f)){
    delta = -1;
  }
  else if(val >= (int)(MAX_JOY_STICK_VAL * 0.45f) && val <= (int)(MAX_JOY_STICK_VAL * 0.55f)){
    delta = 0;
  }
  else if(val < (int)(MAX_JOY_STICK_VAL *0.75f)){
    delta = 1;
  }
  else{
    delta = 2;
  }

  servo_val += delta;

  //make sure servo within bounds
  if(servo_val < MIN_SERVO_VAL){
    servo_val = MIN_SERVO_VAL;
  }
  else if (servo_val > MAX_SERVO_VAL){
    servo_val = MAX_SERVO_VAL; 
  }
}


void read_button(){
  button_state = analogRead(BUTTON_PIN);
  if(button_state < 5){
    sending = true;
  }
}

void radio_movement(){
  packet.type = MESSAGE;
  memcpy(packet.payload.message.address, my_addr, RADIO_ADDRESS_LENGTH);
  packet.payload.message.messageid = 55;
  snprintf((char*)packet.payload.message.messagecontent, sizeof(packet.payload.message.messagecontent), "Test message.");

  if (Radio_Transmit(&packet, RADIO_WAIT_FOR_TX) == RADIO_TX_MAX_RT) // Transmitt packet.
  {
    Serial.println("Data not trasmitted. Max retry.");
  }
  else // Transmitted succesfully.
  {
    Serial.println("Data trasmitted.");
  }
  // The rxflag is set by radio_rxhandler function below indicating that a
  // new packet is ready to be read.
  if (rxflag)
  {
    if (Radio_Receive(&packet) != RADIO_RX_MORE_PACKETS) // Receive packet.
    {
      // if there are no more packets on the radio, clear the receive flag;
      // otherwise, we want to handle the next packet on the next loop iteration.
      rxflag = 0;
    }
    if (packet.type == ACK)
    {
      //digitalWrite(LED_PIN, LOW); // Turn off the led.
    }
  }
}

//setup all the pins
void setup() 
{ 
  //initialize all pins
  pinMode(IR_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT);

  myservo.attach(SERVO_PIN);  // attaches the servo on pin 9 to the servo object ;
  servo_val = SERVO_MIDDLE; //start servo in middle

  scheduler_setup();
  radio_setup();

  Scheduler_StartTask(0, 15, read_joy_stick);
  Scheduler_StartTask(0, 200, read_button);
  Scheduler_StartTask(0, 1, signal_ir);
  Scheduler_StartTask(0, 500, radio_movement);
} 

void scheduler_setup(){
  Scheduler_Init();
}

void radio_setup(){
  //pinMode(LED_PIN, OUTPUT);
  Radio_Init();

  // configure the receive settings for radio pipe 0
  Radio_Configure_Rx(RADIO_PIPE_0, my_addr, ENABLE);
  // configure radio transceiver settings.
  Radio_Configure(RADIO_2MBPS, RADIO_HIGHEST_POWER);
}

void loop()
{
  uint32_t idle_period = Scheduler_Dispatch();
  if (idle_period)
  {
    idle(idle_period);
  }
}





