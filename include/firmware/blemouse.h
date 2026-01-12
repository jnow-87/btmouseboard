/*
 * Credits
 *
 * This file is based on the works of:
 *  - https://github.com/T-vK/ESP32-BLE-Keyboard, commit b7aaf9b
 *  - https://github.com/blackketter/ESP32-BLE-Combo, commit b25c1cb
 *
 * While ESP32-BLE-Combo itself is a fork of ESP32-BLE-Keyboard, it never got
 * updated to more recent versions of it. In particular the support for the
 * NimBLE library was never added.
 *
 * This is an attempt to merge both, even though some details might be missed,
 * since rather then following the exact change histories of both repositories,
 * the commits, listed above, were taken, compared and combined.
 */



#ifndef ESP32_BLE_COMBO_MOUSE_H
#define ESP32_BLE_COMBO_MOUSE_H
#include <firmware/blekeyboard.h>

#define MOUSE_LEFT 1
#define MOUSE_RIGHT 2
#define MOUSE_MIDDLE 4
#define MOUSE_BACK 8
#define MOUSE_FORWARD 16
#define MOUSE_ALL (MOUSE_LEFT | MOUSE_RIGHT | MOUSE_MIDDLE) # For compatibility with the Mouse library

class BleMouse {
private:
  BleKeyboard* _keyboard;
  uint8_t _buttons;
  void buttons(uint8_t b);
public:
  BleMouse(BleKeyboard* keyboard) { _keyboard = keyboard; };
  void begin(void) {};
  void end(void) {};
  void click(uint8_t b = MOUSE_LEFT);
  void move(signed char x, signed char y, signed char wheel = 0, signed char hWheel = 0);
  void press(uint8_t b = MOUSE_LEFT);   // press LEFT by default
  void release(uint8_t b = MOUSE_LEFT); // release LEFT by default
  bool isPressed(uint8_t b = MOUSE_LEFT); // check LEFT by default
};

#endif // ESP32_BLE_COMBO_MOUSE_H
