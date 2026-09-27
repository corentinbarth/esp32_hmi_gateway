//local variable
#define VRY 34 

/*return the state of the joystick*/
int retour_joystick() {
  int y = analogRead(VRY);

  if (y < 100) {
    return DOWN;
  } 

  else if (y > 3900) {
    return UP;
  }
  else if (y >= 100 && y <= 3900) {
    return NEUTRAL;
  }

  return NEUTRAL; //security if none of the cases are verified
}

#endif