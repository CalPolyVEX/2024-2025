//Tone library uses TC5


int num_bytes_received;
int num_error_counter = 0;

// the setup function runs once when you press reset or power the board
void setup() {
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(13, OUTPUT);
  pinMode(12, OUTPUT);
  pinMode(5, OUTPUT);
  Serial.begin(115200);
  init_vex_brain_serial();
  led_setup();
  //init_buzzer();
  //delay(3000);
  //play_mario_theme();
  init_encoders();
  init_led_timer();

  //set PA19 to input with pullup
  PORT->Group[0].PINCFG[19].reg |= PORT_PINCFG_INEN | PORT_PINCFG_PULLEN;
  REG_PORT_OUTSET0 = PORT_PA19;
  delay(1);

  if ((REG_PORT_IN0 & (1 << 19)) == 0) {
    play_mario_theme2();
  }

  PORT->Group[1].DIRSET.reg = PORT_PB01; //set PB01 (LEDSTRIP3) to output
  led_setup();
}

// the loop function runs over and over again forever
void loop() {  
  unsigned char buf[5];
  int toggle = 0;
  int toggle2 = 1, toggle3 = 1;
  
  while (1) {
    num_bytes_received = serial_read_from_brain_delay(buf, 20000, 1); //wait up to 20ms for first byte

    if (num_bytes_received == 1) { //if first byte received
      if (buf[0] == 100) { //if it is the header byte
        num_bytes_received += serial_read_from_brain_delay(buf, 10000, 1); //read the next 1 byte
        
        if (num_bytes_received == 2) { //if 2 total bytes were received from the brain
          read_4_encoder(); //read 4 encoder values and transmit the data to the brain
          buf[0] = 0;
        }
      } else if (buf[0] == 101) { //LED 1 toggle command
        num_bytes_received += serial_read_from_brain_delay(buf, 10000, 1); //read the next 1 byte
        
        if (toggle2) {
          led_on(1);
          toggle2 = 0;
        } else { 
          led_off(1);
          toggle2 = 1;
        }
      } else if (buf[0] == 102) { //LED 2 toggle command
        num_bytes_received += serial_read_from_brain_delay(buf, 10000, 1); //read the next 1 byte
        
        if (toggle3) {
          led_on(2);
          beep();
          toggle3 = 0;
        } else { 
          led_off(2);
          toggle3 = 1;
        }
      } else { //the header byte was incorrect
        num_error_counter++;
      }
    } else { //there was a timeout
      num_error_counter++;
    }
  }

}
