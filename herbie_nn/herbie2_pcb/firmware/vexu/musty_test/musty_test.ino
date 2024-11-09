
//Arduino Tone library uses TC5
//Encoder SPI uses SERCOM4
//VEX Brain communication uses SERCOM5
//LED pattern delays use TC3

int num_bytes_received;  //number of bytes received from the VEX brain so far
int num_error_counter = 0;  //number of data reception errors

// the setup function runs once when you press reset or power the board
void setup() {
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(13, OUTPUT); //set LED2 pin to output
  pinMode(12, OUTPUT); //set LED1 pin to output
  pinMode(5, OUTPUT);  //set SERCOM1.0 to output
  Serial.begin(115200);
  init_vex_brain_serial();
  led_setup();
  init_encoders();
  init_led_timer();
  //setup_otos();
  //initialize_lox();
  setup_VL53L0X();
  //set PA19 (SERCOM1.3) to input with pullup
  PORT->Group[0].PINCFG[19].reg |= PORT_PINCFG_INEN | PORT_PINCFG_PULLEN;
  REG_PORT_OUTSET0 = PORT_PA19;
  delay(1);

  if ((REG_PORT_IN0 & (1 << 19)) == 0) {  //play Mario theme if pins shorted on boot
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

    if (get_encoder_reading(3) % 2 == 1){
      led_off(1);
    } else if (get_encoder_reading(3) % 2 == 0) {
      led_on(1);
    }
    /*
    if (get_value_of_VL53L0X(1) >= 50 && get_value_of_VL53L0X(1) <= 2000){
      led_on(2);
    } else {
      led_off(2);
    }
    */
    

    //Serial.println(get_encoder_reading(3));
    Serial.print("Distance:");
    Serial.println(get_value_of_VL53L0X(1));

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
      } else if (buf[0] == 200){ // command to get L0X data
        num_bytes_received += serial_read_from_brain_delay(buf, 10000, 1);

        if (num_bytes_received == 2) { //if 2 total bytes were received from the brain
          read_VL53L0X_to_brain(); 
          buf[0] = 0;
        }


      } else if (buf[0] == 300){ //command to get otos data
        if (num_bytes_received == 2) { //if 2 total bytes were received from the brain
          send_otos_data(); 
          buf[0] = 0;
        }

      } else { //the header byte was incorrect
        num_error_counter++;
      }
    } else { //there was a timeout
      num_error_counter++;
    }
  }

}
