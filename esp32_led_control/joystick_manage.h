#ifndef JOYSTICK_MANAGER_H
#define JOYSTICK_MANAGER_H

#define VRY 34

// Codes de retour : position du joystick
#define HAUT   1   
#define BAS -1    
#define NEUTRE 0   

int retour_joystick() {
  int y = analogRead(VRY);

  if (y < 100) {
    return BAS;
  } 

  else if (y > 3900) {
    return HAUT;
  }
  else if (y >= 100 && y <= 3900) {
    return NEUTRE;
  }

  return NEUTRE; //sécurité si aucun des cas n'est atteint
}

#endif