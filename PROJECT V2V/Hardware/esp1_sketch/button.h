#ifndef BUTTON_H
#define BUTTON_H
#include <Arduino.h>

class Button
{
  private:
    int pin;
    bool currentstate;
    bool laststate;
    unsigned long lastpresstime;
    unsigned long lastreleasetime;
    int clickcount = 0;   
    int completedclicks = 0;
    unsigned long lastclicktime = 0;

  public:
    Button(int gpio);
    void begin();
    void update();

    bool isPressed();

    bool wasPressed();
    bool wasReleased();

    bool wasClicked();

    bool wasLongPressed();

    int getClickCount();
};

#endif
