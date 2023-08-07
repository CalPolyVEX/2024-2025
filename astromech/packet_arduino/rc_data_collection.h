#ifndef RC_DATA_COLLECTION_H
#define RC_DATA_COLLECTION_H

#include "i2c_pin_led.h"
#include "logic_engine_control.h"
#include "motor_servo_control.h"
#include "packet_arduino.h"
#include "pc_decoder.h"
#include "reon_hp_i2c.h"

// Queue class
// Implemented using a circular array
#define QUEUE_BUFFER_SIZE 250 // should not be bigger than a uint8_t (255)
class Queue {
public:
    // Initialize Queue Object
    Queue() {
        // Set Head and Tail Initial Values
        head = 0;
        tail = 0;
        data_size = 0;

        dummy_var = 0;
    }

    // Enqueue data
    void enqueue(uint8_t new_data) {
        // If buffer is full, error
        if (data_size == QUEUE_BUFFER_SIZE) {
            return;
        }

        // Insert Data and Increment Head
        buffer[head] = new_data;
        data_size++;
        head++;

        // If head is outside buffer size, go back to start of array
        if (head > QUEUE_BUFFER_SIZE)
            head = 0;
    }

    // Dequeue Data
    uint8_t dequeue() {
        // If buffer is Empty, error
        if (!data_size) {
            return 0xf;
        }

        // Read Data and Increment Tail
        dummy_var = buffer[tail];
        data_size--;
        tail++;

        // If tail is outside buffer size, go back to start of array
        if (tail > QUEUE_BUFFER_SIZE)
            tail = 0;

        // Return data
        return dummy_var;
    }

    // Dequeues Data into Array
    uint8_t dequeue_array(unsigned int size, uint8_t *array) {
        // Test if there is sufficient data
        if (data_size < size) {
            return 0xf;
        }

        for (unsigned int i = 0; i < size; i++) {
            // Read Data and Increment Tail
            array[i] = buffer[tail];
            buffer[tail] = 0;
            data_size--;
            tail++;

            // If tail is outside buffer size, go back to start of array
            if (tail > QUEUE_BUFFER_SIZE)
                tail = 0;
        }
    }

    // Reset the queue
    void reset() {
        // Set Head and Tail Initial Values
        head = 0;
        tail = 0;
        data_size = 0;
    }

    uint8_t get_data_size() { return data_size; }

private:
    // Circular Array
    uint8_t buffer[QUEUE_BUFFER_SIZE + 1];

    // Head and Tail Markers
    uint8_t head;
    uint8_t tail;

    // Number of Data in Buffer
    uint8_t data_size;

    uint8_t dummy_var;
};

//class Queue;

void receiver_setup();
bool receiver_loop();
void decodeData();
void playAudio();
void stopTracks();

#endif