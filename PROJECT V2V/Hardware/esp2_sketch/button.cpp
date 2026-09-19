#include "button.h"
Button::Button(int gpio)
{
  pin = gpio;
  currentstate = HIGH;
  laststate = HIGH;
}
void Button::begin()
{
  pinMode(pin,INPUT_PULLUP);
}
void Button::update()
{
  laststate = currentstate;
  currentstate = digitalRead(pin); 
  if(laststate == HIGH && currentstate == LOW){
    lastpresstime = millis();
  }
  if(laststate == LOW && currentstate == HIGH){
    lastreleasetime = millis();
  }
  if(wasClicked()){
    clickcount++;
    lastclicktime = millis();
  }
  if(clickcount>0 && millis()-lastclicktime > 350 ){
    completedclicks = clickcount;
    clickcount = 0;
  }
}
bool Button::isPressed()
{
  return currentstate==LOW;
}
bool Button::wasPressed()
{
  return currentstate==LOW && laststate==HIGH;
}
bool Button::wasReleased()
{
  return currentstate==HIGH && laststate==LOW;
}
bool Button::wasClicked()
{
  return wasReleased() && (lastreleasetime - lastpresstime <= 100);
}
bool Button::wasLongPressed()
{
  return wasReleased() && (lastreleasetime - lastpresstime >= 1500) && (lastreleasetime - lastpresstime <= 2000);
}
int Button::getClickCount()
{
  int temp = completedclicks;
  completedclicks = 0;
  return temp;
}