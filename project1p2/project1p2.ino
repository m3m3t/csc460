

#include "scheduler.h"
#include "packet.h"
#include "radio.h"
#include "cops_and_robbers.h"

#define MAX_JOY_STICK_VAL 1023
#define MIN_JOY_STICK_VAL 0

#define MAX_ROOMBA_SPEED 500.0f
#define MAX_ROOMBA_TURN 2000.0f

#define IR_PIN 13 // pin for ir transmitter
#define BUTTON_PIN 2

#define JOY_STICK_PIN_X 0 //pin for joystick read
#define JOY_STICK_PIN_Y 1//pin for joystick read

volatile uint8_t rxflag = 0; //radio flag

uint8_t station_addr[5] = { 
  0xE4, 0xE4, 0xE4, 0xE4, 0xE4 }; // Receiver address
uint8_t my_addr[5] = { 
  0x98, 0x76, 0x54, 0x32, 0x11 }; // Transmitter address

radiopacket_t packet_move;//radio packet
radiopacket_t packet_ir;//packet for ir command
radiopacket_t packet_start_sci;

//read joystick, calculate driving parameters
void read_joy_stick(){
  static int32_t velocity = 0; //how much to move forward/back
  static int32_t radius = 0; //how much to turn

  int valy = analogRead(JOY_STICK_PIN_Y);            // reads the value of the potentiometer (value between 0 and 1023) 
  int valx = analogRead(JOY_STICK_PIN_X);            // reads the value of the potentiometer (value between 0 and 1023) 

  //velocity
  if(valx < (int)(MAX_JOY_STICK_VAL *0.40f)){
    velocity = -(int)(0.50f * MAX_ROOMBA_SPEED);
  }else if(valx >= (int)(MAX_JOY_STICK_VAL * 0.40f) && valx <= (int)(MAX_JOY_STICK_VAL * 0.60f)){
    velocity = 0;
  }else if (valx > (int)(MAX_JOY_STICK_VAL * 0.60f)){
     velocity = (int)(0.50f * MAX_ROOMBA_SPEED);
  }

  //turn radius
  if(velocity != 0){
    if(valy < (int)(MAX_JOY_STICK_VAL *0.40f)){
      radius = (int)(0.50f * MAX_ROOMBA_TURN);
    }
    else if(valy >= (int)(MAX_JOY_STICK_VAL * 0.40f) && valy <= (int)(MAX_JOY_STICK_VAL * 0.60f)){
      //specail case for straight
      radius = 32768;
    }
    else if(valy > (int)(MAX_JOY_STICK_VAL *0.60f)){
      radius = (int)(-0.50f * MAX_ROOMBA_TURN);
    }

  }
  else{//turning on spot
    if(valy < (int)(MAX_JOY_STICK_VAL *0.40f)){
      radius = 1;
    }
    else if(valy >= (int)(MAX_JOY_STICK_VAL * 0.40f) && valy <= (int)(MAX_JOY_STICK_VAL * 0.60f)){
      radius = 32768;
    }
    else if(valy > (int)(MAX_JOY_STICK_VAL *0.60f)){
      radius = -1;
    }
  }

  radio_movement(velocity, radius);
}


//set up the packet to tell rommba to drive
void radio_movement(int16_t velocity, int16_t radius){
  static int val = 0;

  if(val == 0){
    digitalWrite(IR_PIN, LOW);
  }else if(val == 5){
    digitalWrite(IR_PIN, HIGH);
    val = -1;
  }
   val ++;

  char string[20];
  //sprintf(string, "Sending v: %d r:%d",velocity, radius);

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

  //Serial.println(string);
  if (Radio_Transmit(&packet_move, RADIO_RETURN_ON_TX) == RADIO_TX_MAX_RT) // Transmitt packet.
  {
    Serial.println("Data not trasmitted. Max retry.");
  }
  else // Transmitted succesfully.
  {
    Serial.println("Data trasmitted.");
  }

}

void radio_start_roomba_sci(){
  digitalWrite(IR_PIN, LOW);
  //send start packet
  packet_start_sci.type = COMMAND;
  memcpy(packet_start_sci.payload.command.sender_address, my_addr, RADIO_ADDRESS_LENGTH);
  packet_start_sci.payload.command.command = 128;
  packet_start_sci.payload.command.num_arg_bytes = 0;
  
  if (Radio_Transmit(&packet_start_sci, RADIO_RETURN_ON_TX) == RADIO_TX_MAX_RT) // Transmitt packet.
  {
    Serial.println("Data not trasmitted. Max retry.");
  }
  else // Transmitted succesfully.
  {
    Serial.println("Data trasmitted.");
  }

  //send control command
  memcpy(packet_start_sci.payload.command.sender_address, my_addr, RADIO_ADDRESS_LENGTH);
  packet_start_sci.payload.command.command = 130;
  packet_start_sci.payload.command.num_arg_bytes = 0;;
  
  if (Radio_Transmit(&packet_start_sci, RADIO_RETURN_ON_TX) == RADIO_TX_MAX_RT) // Transmitt packet.
  {
    Serial.println("Data not trasmitted. Max retry.");
  }
  else // Transmitted succesfully.
  {
    Serial.println("Data trasmitted.");
  }
  digitalWrite(IR_PIN, HIGH);
}

//initailize the radio
void radio_setup(){
  //pinMode(LED_PIN, OUTPUT);
  Radio_Init();

  // configure the receive settings for radio pipe 0
  Radio_Configure_Rx(RADIO_PIPE_0, my_addr, ENABLE);
  // configure radio transceiver settings.
  Radio_Configure(RADIO_2MBPS, RADIO_HIGHEST_POWER);
}

void radio_rxhandler(uint8_t pipe_number)
{
  // This function is called when the radio receives a packet.
  // It is called in the radio's ISR, so it must be kept short.
  // The function may be left empty if the application doesn't need to respond immediately to the interrupt.
}

//read button to check if fire ir
void read_button(){
  static int is_pressed = 0;
  int button_val = analogRead(BUTTON_PIN);
  if(button_val < 10 && is_pressed == 0){
    is_pressed = 1;
    fire_ir();
  }else if (button_val >= 100){
    is_pressed = 0;
  }
}

//send packet to fire ir
void fire_ir(){
  digitalWrite(IR_PIN, HIGH);
  packet_ir.type = IR_COMMAND;
  memcpy(packet_ir.payload.ir_command.sender_address, my_addr, RADIO_ADDRESS_LENGTH);
  packet_ir.payload.ir_command.ir_command = SEND_BYTE;
  packet_ir.payload.ir_command.ir_data = 0x41; //"A"
  packet_ir.payload.ir_command.servo_angle = 90;
  
    if (Radio_Transmit(&packet_ir, RADIO_RETURN_ON_TX) == RADIO_TX_MAX_RT) // Transmitt packet.
  {
    Serial.println("IR Data not trasmitted. Max retry.");
  }
  else // Transmitted succesfully.
  {
    Serial.println("IR Data trasmitted.");
  }
  digitalWrite(IR_PIN, LOW);
}

//setup all the pins
void setup() 
{ 
  Serial.begin(57600);
  Serial.println("begin");
  //initialize all pins
  pinMode(IR_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT);

  Scheduler_Init();
  radio_setup();

  //inialize the roomba for sci
  radio_start_roomba_sci();

  Scheduler_StartTask(1950, 200, read_joy_stick);
  Scheduler_StartTask(2000, 100, read_button);
} 


void idle(uint32_t idle){

}

void loop()
{

  uint32_t idle_period = Scheduler_Dispatch();
  if (idle_period)
  {
    idle(idle_period);
  }
}




