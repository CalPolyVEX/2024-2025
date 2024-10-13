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
  
  while (1) {
    num_bytes_received = serial_read_from_brain_delay(buf, 20000, 1); //wait up to 20ms for first byte

    if (num_bytes_received == 1) { //if first byte received
      if (buf[0] == 100) { //if it is the header byte
        num_bytes_received += serial_read_from_brain_delay(buf, 10000, 3); //read the next 3 bytes
        
        if (num_bytes_received == 4) { //if 4 total bytes were received from the brain
          read_4_encoder(); //read 4 encoder values and transmit the data to the brain
          buf[0] = 0;
        }
        
        if (toggle) {
          digitalWrite(13, HIGH);    // turn the LED2 on 
          toggle = 0;
        } else { 
          digitalWrite(13, LOW);
          toggle = 1;
        }
      } else { //the header byte was incorrect
        num_error_counter++;
      }
    } else { //there was a timeout
      num_error_counter++;
    }
  }

}
