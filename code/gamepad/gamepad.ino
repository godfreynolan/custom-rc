#include <avr/power.h>
#include <EEPROM.h>

#include "Mouse.h"
#include "Keyboard.h"

#include "Joystick.h" // https://github.com/MHeironimus/ArduinoJoystickLibrary

// Pin mappings
#define JOYSTICK_LEFT_X_PIN A7
#define JOYSTICK_LEFT_Y_PIN A6
#define JOYSTICK_LEFT_BUTTON_PIN 7

#define JOYSTICK_RIGHT_X_PIN A0
#define JOYSTICK_RIGHT_Y_PIN A2
#define JOYSTICK_RIGHT_BUTTON_PIN 2

#define BUTTON_BOTTOM_PIN 14
#define BUTTON_RIGHT_PIN 10
#define BUTTON_TOP_PIN 16
#define BUTTON_LEFT_PIN 15

#define SWITCH_PIN 9

// HID button indices
#define BUTTON_BOTTOM_INDEX 0
#define BUTTON_RIGHT_INDEX 1
#define BUTTON_TOP_INDEX 3
#define BUTTON_LEFT_INDEX 4

// Input settings
#define JOYSTICK_RANGE 127
#define JOYSTICK_CENTER_ANALOG_READING 512
#define DEADZONE 12
#define MOUSE_SENSITIVITY 12
#define SCROLL_SENSITIVITY 3
#define SCROLL_DELAY 5

// EEPROM memory address for calibration data
#define CALIBRATION_LEFT_X_EEPROM_ADDR 0
#define CALIBRATION_LEFT_Y_EEPROM_ADDR sizeof(JoystickCalibration)
#define CALIBRATION_RIGHT_X_EEPROM_ADDR (2 * sizeof(JoystickCalibration))
#define CALIBRATION_RIGHT_Y_EEPROM_ADDR (3 * sizeof(JoystickCalibration))

typedef struct {
    int min = 0;
    int center = 512;
    int max = 1023;
} JoystickCalibration;

JoystickCalibration calibration_left_x = {};
JoystickCalibration calibration_left_y = {};
JoystickCalibration calibration_right_x = {};
JoystickCalibration calibration_right_y = {};

Joystick_ Joystick(JOYSTICK_DEFAULT_REPORT_ID,JOYSTICK_TYPE_GAMEPAD,
    5, 0,                  // Button Count, Hat Switch Count
    true, true, false,     // X and Y, but no Z Axis
    true, true, false,     // Rx, Ry, no Rz
    false, false,          // No rudder or throttle
    false, false, false    // No accelerator, brake, or steering
); 

unsigned int scroll_delay_counter = SCROLL_DELAY;

/*
 * Like Arduino's map function but ensures values below center are mapped below
 * center and values above center are mapped above center.
 */
int centered_map(int input, int input_min, int input_center, int input_max, int output_min, int output_center, int output_max) {
    if (input < input_center) {
        return map(input, input_min, input_center, output_min, output_center);
    } else if (input > input_center) {
        return map(input, input_center, input_max, output_center, output_max);
    } else {
        return output_center;
    }
}

/* Convert an analog reading from a joystick to a HID joystick input.
*/
int analog_joystick_to_joystick(int analog_reading, JoystickCalibration calibration) {
    if (abs(analog_reading - calibration.center) < DEADZONE) {
        return 0;
    }
    return centered_map(analog_reading, calibration.min, calibration.center, calibration.max, -JOYSTICK_RANGE, 0, JOYSTICK_RANGE);
}

/* Convert an analog reading from a joystick to a HID mouse input.
*/
int analog_joystick_to_mouse(int analog_reading, JoystickCalibration calibration) {
    if (abs(analog_reading - calibration.center) < DEADZONE) {
        return 0;
    }
    return centered_map(analog_reading, calibration.min, calibration.center, calibration.max, -MOUSE_SENSITIVITY, 0, MOUSE_SENSITIVITY);
}

/* Convert an analog reading from a joystick to a HID mouse wheel input.
*/
int analog_joystick_to_scrollwheel(int analog_reading, JoystickCalibration calibration) {
    if (abs(analog_reading - calibration.center) < DEADZONE) {
        return 0;
    }
    return centered_map(analog_reading, calibration.min, calibration.center, calibration.max, -SCROLL_SENSITIVITY, 0, SCROLL_SENSITIVITY);
}

void wait_until_released(uint8_t pin) {
    while(!digitalRead(pin));
}

void wait_until_pressed(uint8_t pin) {
    while(digitalRead(pin));
}

void calibration_debounce() {
    delay(250);
}

/* Calibration sequence for left joystick
 */
void calibrate_left(JoystickCalibration* calibration_x, JoystickCalibration* calibration_y) {
    Serial.println("Left calibration started");
    // center
    calibration_x->center = analogRead(JOYSTICK_LEFT_X_PIN);;
    calibration_y->center = analogRead(JOYSTICK_LEFT_Y_PIN);;
    wait_until_released(JOYSTICK_RIGHT_BUTTON_PIN);
    calibration_debounce();

    // y max
    Serial.println("Move left stick to top and press right stick");
    wait_until_pressed(JOYSTICK_RIGHT_BUTTON_PIN);
    calibration_y->max = analogRead(JOYSTICK_LEFT_Y_PIN);
    wait_until_released(JOYSTICK_RIGHT_BUTTON_PIN);
    calibration_debounce();

    // y min
    Serial.println("Move left stick to bottom and press right stick");
    wait_until_pressed(JOYSTICK_RIGHT_BUTTON_PIN);
    calibration_y->min = analogRead(JOYSTICK_LEFT_Y_PIN);
    wait_until_released(JOYSTICK_RIGHT_BUTTON_PIN);
    calibration_debounce();

    // x min
    Serial.println("Move left stick to left and press right stick");
    wait_until_pressed(JOYSTICK_RIGHT_BUTTON_PIN);
    calibration_x->min = analogRead(JOYSTICK_LEFT_X_PIN);
    wait_until_released(JOYSTICK_RIGHT_BUTTON_PIN);
    calibration_debounce();

    // x max
    Serial.println("Move left stick to right and press right stick");
    wait_until_pressed(JOYSTICK_RIGHT_BUTTON_PIN);
    calibration_x->max = analogRead(JOYSTICK_LEFT_X_PIN);
    wait_until_released(JOYSTICK_RIGHT_BUTTON_PIN);
    calibration_debounce();

    EEPROM.put(CALIBRATION_LEFT_X_EEPROM_ADDR, *calibration_x);
    EEPROM.put(CALIBRATION_LEFT_Y_EEPROM_ADDR, *calibration_y);
}

void calibrate_right(JoystickCalibration* calibration_x, JoystickCalibration* calibration_y) {
    // center
    Serial.println("Right calibration started");
    calibration_x->center = analogRead(JOYSTICK_RIGHT_X_PIN);;
    calibration_y->center = analogRead(JOYSTICK_RIGHT_Y_PIN);;
    wait_until_released(JOYSTICK_LEFT_BUTTON_PIN);
    calibration_debounce();

    // y max
    Serial.println("Move right stick to top and press left stick");
    wait_until_pressed(JOYSTICK_LEFT_BUTTON_PIN);
    calibration_y->max = analogRead(JOYSTICK_RIGHT_Y_PIN);
    wait_until_released(JOYSTICK_LEFT_BUTTON_PIN);
    calibration_debounce();

    // y min
    Serial.println("Move right stick to bottom and press left stick");
    wait_until_pressed(JOYSTICK_LEFT_BUTTON_PIN);
    calibration_y->min = analogRead(JOYSTICK_RIGHT_Y_PIN);
    wait_until_released(JOYSTICK_LEFT_BUTTON_PIN);
    calibration_debounce();

    // x min
    Serial.println("Move right stick to left and press left stick");
    wait_until_pressed(JOYSTICK_LEFT_BUTTON_PIN);
    calibration_x->min = analogRead(JOYSTICK_RIGHT_X_PIN);
    wait_until_released(JOYSTICK_LEFT_BUTTON_PIN);
    calibration_debounce();

    // x max
    Serial.println("Move right stick to right and press left stick");
    wait_until_pressed(JOYSTICK_LEFT_BUTTON_PIN);
    calibration_x->max = analogRead(JOYSTICK_RIGHT_X_PIN);
    wait_until_released(JOYSTICK_LEFT_BUTTON_PIN);
    calibration_debounce();

    EEPROM.put(CALIBRATION_RIGHT_X_EEPROM_ADDR, *calibration_x);
    EEPROM.put(CALIBRATION_RIGHT_Y_EEPROM_ADDR, *calibration_y);
}

void setup() {
    // clock_prescale_set(clock_div_2);

    Serial.begin(9600);

    // Initialize pins
    pinMode(JOYSTICK_RIGHT_X_PIN, INPUT);
    pinMode(JOYSTICK_RIGHT_Y_PIN, INPUT);
    pinMode(JOYSTICK_RIGHT_BUTTON_PIN, INPUT_PULLUP);

    pinMode(JOYSTICK_LEFT_X_PIN, INPUT);
    pinMode(JOYSTICK_LEFT_Y_PIN, INPUT);
    pinMode(JOYSTICK_LEFT_BUTTON_PIN, INPUT_PULLUP);

    pinMode(BUTTON_BOTTOM_PIN, INPUT_PULLUP);
    pinMode(BUTTON_RIGHT_PIN, INPUT_PULLUP);
    pinMode(BUTTON_TOP_PIN, INPUT_PULLUP);
    pinMode(BUTTON_LEFT_PIN, INPUT_PULLUP);

    pinMode(SWITCH_PIN, INPUT_PULLUP);

    // Set intended output range for sticks
    Joystick.setXAxisRange(-127, 127);
    Joystick.setYAxisRange(-127, 127);
    Joystick.setRxAxisRange(-127, 127);
    Joystick.setRyAxisRange(-127, 127);

    // Read calibration data. If this is the first time using this code, this 
    // data will be garbage. The sticks won't work properly until they are
    // calibrated.
    EEPROM.get(CALIBRATION_LEFT_X_EEPROM_ADDR, calibration_left_x);
    EEPROM.get(CALIBRATION_LEFT_Y_EEPROM_ADDR, calibration_left_y);
    EEPROM.get(CALIBRATION_RIGHT_X_EEPROM_ADDR, calibration_right_x);
    EEPROM.get(CALIBRATION_RIGHT_Y_EEPROM_ADDR, calibration_right_y);

    Joystick.begin(false);
    Mouse.begin();
    Keyboard.begin();
}

void loop() {
    // Enter calibration sequence if either of the stick buttons are pushed.
    if (!digitalRead(JOYSTICK_RIGHT_BUTTON_PIN)) {
        calibrate_left(&calibration_left_x, &calibration_left_y);
    } else if (!digitalRead(JOYSTICK_LEFT_BUTTON_PIN)) {
        calibrate_right(&calibration_right_x, &calibration_right_y);
    }

    const bool mouse_mode = digitalRead(SWITCH_PIN);
    if (mouse_mode) {
        const int left_x = analog_joystick_to_mouse(analogRead(JOYSTICK_LEFT_X_PIN), calibration_left_x);
        const int left_y = -analog_joystick_to_mouse(analogRead(JOYSTICK_LEFT_Y_PIN), calibration_left_y);
        
        // Things will scroll too fast even if right_y == 1 if we send this 
        // every update. Instead, we'll only send scroll values once every
        // SCROLL_DELAY updates.
        int right_y; 
        if (scroll_delay_counter >= SCROLL_DELAY) {
            right_y = -analog_joystick_to_scrollwheel(analogRead(JOYSTICK_RIGHT_Y_PIN), calibration_right_y);
            scroll_delay_counter = 0;
        } else {
            right_y = 0;
        }
        if (scroll_delay_counter < SCROLL_DELAY) {
            scroll_delay_counter++;
        }

        Mouse.move(left_x, left_y, -right_y);

        const bool button_bottom = !digitalRead(BUTTON_BOTTOM_PIN);
        const bool button_right = !digitalRead(BUTTON_RIGHT_PIN);
        //const bool button_top = !digitalRead(BUTTON_TOP_PIN);
        //const bool button_left = !digitalRead(BUTTON_LEFT_PIN);

        if (button_bottom) {
            Mouse.press(MOUSE_LEFT);
        } else {
            Mouse.release(MOUSE_LEFT);
        }
        if (button_right) {
            Mouse.press(MOUSE_RIGHT);
        } else {
            Mouse.release(MOUSE_RIGHT);
        }

    } else {
        const int left_x = analog_joystick_to_joystick(analogRead(JOYSTICK_LEFT_X_PIN), calibration_left_x);
        const int left_y = -analog_joystick_to_joystick(analogRead(JOYSTICK_LEFT_Y_PIN), calibration_left_y);
        Joystick.setXAxis(left_x);
        Joystick.setYAxis(left_y);

        const int right_x = analog_joystick_to_joystick(analogRead(JOYSTICK_RIGHT_X_PIN), calibration_right_x);
        const int right_y = -analog_joystick_to_joystick(analogRead(JOYSTICK_RIGHT_Y_PIN), calibration_right_y);
        Joystick.setRxAxis(right_x);
        Joystick.setRyAxis(right_y);

        const bool button_bottom = !digitalRead(BUTTON_BOTTOM_PIN);
        const bool button_right = !digitalRead(BUTTON_RIGHT_PIN);
        const bool button_top = !digitalRead(BUTTON_TOP_PIN);
        const bool button_left = !digitalRead(BUTTON_LEFT_PIN);
        Joystick.setButton(BUTTON_BOTTOM_INDEX, button_bottom);
        Joystick.setButton(BUTTON_RIGHT_INDEX, button_right);
        Joystick.setButton(BUTTON_TOP_INDEX, button_top);
        Joystick.setButton(BUTTON_LEFT_INDEX, button_left);

        Joystick.sendState();
    }
    delay(10);
}
