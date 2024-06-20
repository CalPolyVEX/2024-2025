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
  int encoder_reading;
  
  digitalWrite(13, HIGH);   // turn the LED on (HIGH is the voltage level)
  digitalWrite(12, LOW);    // turn the LED on (HIGH is the voltage level)
  digitalWrite(5, HIGH);    // turn the LED on (HIGH is the voltage level)
  //delay(1000);              // wait for a second
  digitalWrite(13, LOW);    // turn the LED off by making the voltage LOW
  digitalWrite(12, HIGH);   // turn the LED off by making the voltage LOW
  digitalWrite(5, LOW);     // turn the LED off by making the voltage LOW
  //delay(1000);              // wait for a second
  //test_encoder_chips();
  
  //Serial.write(test_encoder_chips() + 97); //write 'a'
  //Serial.write(97);
  //serial_write_to_brain(97);
  //led_show();
  read_4_encoder();
  num_bytes_received = serial_read_from_brain_delay(buf, 2000, 4); 
  if ((num_bytes_received != 4)) // && (num_bytes_received != 0))
    num_error_counter++;
    
  delay(1);
}
