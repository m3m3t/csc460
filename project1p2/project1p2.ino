#include "Servo.h" 
#include "scheduler.h"
#include "packet.h"
#include "radio.h"

//transmission protocol
#define SENDING_BITS 8

//servo/joystick constants
#define MAX_JOY_STICK_VAL 1023
#define MIN_JOY_STICK_VAL 0
#define SERVO_MIDDLE 90 // middle of range for servo

#define MAX_ROOMBA_SPEED 500
#define MAX_ROOMBA_TURN 2000
//pins
#define SERVO_PIN 9 //pin for servo
#define IR_PIN 24 // pin for ir transmitter
#define LED_PIN 24 // pin for radio led
#define JOY_STICK_PIN_X 0 //pin for joystick read
#define JOY_STICK_PIN_Y 1//pin for joystick read
#define BUTTON_PIN 2

volatile uint8_t rxflag = 0; //radio flag

uint8_t station_addr[5] = { 
  0xE4, 0xE4, 0xE4, 0xE4, 0xE4 }; // Receiver address
uint8_t my_addr[5] = { 
  0x98, 0x76, 0x54, 0x32, 0x11 }; // Transmitter address

radiopacket_t packet_move;//radio packet
radiopacket_t packet_ir;//

int button_state; //button on joystick

boolean sending = false;


//on idle that data from servo and joystick
void idle(uint32_t idle_period){
  delay(idle_period);
}

//read joystick, servo is smooth and stops when joystick released
void read_joy_stick(){
  static int16_t velocity = 0; //how much to move forward/back
  static int16_t radius = 0; //how much to turn
  
  int valy = analogRead(JOY_STICK_PIN_Y);            // reads the value of the potentiometer (value between 0 and 1023) 
  int valx = analogRead(JOY_STICK_PIN_X);            // reads the value of the potentiometer (value between 0 and 1023) 

  //move joystick
  if(valx < (int)(MAX_JOY_STICK_VAL * 0.25f)){
     velocity += (velocity < (-1)*MAX_ROOMBA_SPEED/2) ? -2 : 0;
  }
  else if(valx < (int)(MAX_JOY_STICK_VAL *0.45f)){
    velocity += (velocity < (-1)*MAX_ROOMBA_SPEED/5) ? -1 : 0;
  }
  else if(valx >= (int)(MAX_JOY_STICK_VAL * 0.45f) && valx <= (int)(MAX_JOY_STICK_VAL * 0.55f)){
    velocity = 0;
  }
  else if(valx < (int)(MAX_JOY_STICK_VAL *0.75f)){
    velocity += (velocity < MAX_ROOMBA_SPEED/5) ? 1 : 0;
  }
  else{
    velocity += (velocity < MAX_ROOMBA_SPEED/2) ? 2 : 0;
  }
  
  //move joystick
  if(valy < (int)(MAX_JOY_STICK_VAL * 0.25f)){
     radius += (radius < (-1)*MAX_ROOMBA_TURN/2) ? -2 : 0;
  }
  else if(valy < (int)(MAX_JOY_STICK_VAL *0.45f)){
    radius += (radius < (-1)*MAX_ROOMBA_TURN/5) ? -1 : 0;
  }
  else if(valy >= (int)(MAX_JOY_STICK_VAL * 0.45f) && valy <= (int)(MAX_JOY_STICK_VAL * 0.55f)){
    radius = 0;
  }
  else if(valy < (int)(MAX_JOY_STICK_VAL *0.75f)){
    radius += (radius < MAX_ROOMBA_TURN/5) ? 1 : 0;
  }
  else{
    radius += (radius < MAX_ROOMBA_TURN/2) ? 2 : 0;
  }

  radio_movement(velocity, radius);
  
}


void read_button(){
  button_state = analogRead(BUTTON_PIN);
  if(button_state < 5){
    sending = true;
  }
}

void radio_movement(int16_t velocity, int16_t radius){
  char string[20];
  sprintf(string, "Sending v: %d r:%d",velocity, radius);
  Serial.println(string);
  packet_move.type = COMMAND;
  memcpy(packet_move.payload.command.sender_address, my_addr, RADIO_ADDRESS_LENGTH);
  packet_move.payload.command.command = 137;
  packet_move.payload.command.num_arg_bytes = 4;
  //convert velocity to two 8 bit numbers
  packet_move.payload.command.arguments[0] =  (uint8_t)((velocity << 8)& 0xff);
  packet_move.payload.command.arguments[1] =  (uint8_t)(velocity & 0xff);
  //convert radius to two 8 bit numbers
  packet_move.payload.command.arguments[2] =  (uint8_t)((radius << 8)& 0xff);
  packet_move.payload.command.arguments[3] =  (uint8_t)(radius & 0xff);

  if (Radio_Transmit(&packet_move, RADIO_WAIT_FOR_TX) == RADIO_TX_MAX_RT) // Transmitt packet.
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
    if (Radio_Receive(&packet_move) != RADIO_RX_MORE_PACKETS) // Receive packet.
    {
      // if there are no more packets on the radio, clear the receive flag;
      // otherwise, we want to handle the next packet on the next loop iteration.
      rxflag = 0;
    }
  }
}

void signal_ir(){
}

//setup all the pins
void setup() 
{ 
  Serial.begin(57600);
  //initialize all pins
  pinMode(IR_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT);

  scheduler_setup();
  radio_setup();

  Scheduler_StartTask(0, 125, read_joy_stick);
  Scheduler_StartTask(0, 200, read_button);
  Scheduler_StartTask(0, 100, signal_ir);
  Serial.println("Starting");
  digitalWrite(IR_PIN, HIGH);
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






