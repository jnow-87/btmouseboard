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



#include <firmware/blemouse.h>

void BleMouse::click(uint8_t b)
{
  _buttons = b;
  move(0,0,0,0);
  _buttons = 0;
  move(0,0,0,0);
}

void BleMouse::move(signed char x, signed char y, signed char wheel, signed char hWheel)
{
  if (_keyboard->isConnected())
  {
    uint8_t m[5];
    m[0] = _buttons;
    m[1] = x;
    m[2] = y;
    m[3] = wheel;
    m[4] = hWheel;
    _keyboard->inputMouse->setValue(m, 5);
    _keyboard->inputMouse->notify();
  }
}

void BleMouse::buttons(uint8_t b)
{
  if (b != _buttons)
  {
    _buttons = b;
    move(0,0,0,0);
  }
}

void BleMouse::press(uint8_t b)
{
  buttons(_buttons | b);
}

void BleMouse::release(uint8_t b)
{
  buttons(_buttons & ~b);
}

bool BleMouse::isPressed(uint8_t b)
{
  if ((b & _buttons) > 0)
    return true;
  return false;
}
