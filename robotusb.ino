#include <Mouse.h>
#define SERIAL_MESSAGE_SIZE 16
#define MAX_TOKEN_LENGTH 16
#define TOKEN0_MOUSE 0
#define TOKEN0_KEYBOARD 1
#define MOUSE_NOCLICK 0
#define MOUSE_LEFTCLICK 1
#define MOUSE_RIGHTCLICK 2
#define MOUSE_FREQUENCY 2

void setup() 
{
  Serial.begin(115200);
  Serial.setTimeout(10);
  Mouse.begin();
}

void MouseClick(int x, int y, int mclick)
{  
  unsigned int xiter = abs(x)/MOUSE_FREQUENCY;
  unsigned int xremainder = abs(x) % xiter;
  unsigned int yiter = abs(y)/MOUSE_FREQUENCY;
  unsigned int yremainder = abs(y) % yiter;
  int xdir = x < 0 ? -1 : 1;
  int ydir = y < 0 ? -1 : 1;

  int dx =  MOUSE_FREQUENCY * xdir;
  int dy =  MOUSE_FREQUENCY * ydir;
  for(unsigned int xi = 0; xi < xiter; xi++)
  {
    Mouse.move(dx, 0, 0);   
  }

  for(unsigned int yi = 0; yi < yiter; yi++)
  {
    Mouse.move(0, dy, 0);   
  }
  
  Mouse.move(xremainder * xdir, yremainder * ydir, 0);  

  if(mclick == MOUSE_LEFTCLICK)
  {
    Mouse.click(MOUSE_LEFT);
  }
  else if(mclick == MOUSE_RIGHTCLICK)
  {
    Mouse.click(MOUSE_RIGHT);
  }
  Serial.flush();
}

void KeyboardClick(char key)
{
  
}

void loop() 
{
  if(Serial.available() >= SERIAL_MESSAGE_SIZE)
  {  
    char data[16];
    Serial.readBytes(data, SERIAL_MESSAGE_SIZE);
    String serial_string(data);
    char* saveptr = (char*)serial_string.c_str();
    char* token;
    int tokens[MAX_TOKEN_LENGTH];
    unsigned int pos = 0;
    while((token = strtok_r(saveptr, ",", &saveptr)))
    {
      if(pos < MAX_TOKEN_LENGTH)
      {
        tokens[pos++] = atoi(token);
      }
    }
    if(tokens[0] == TOKEN0_MOUSE)
    {
      MouseClick(tokens[1], tokens[2], tokens[3]);
    }
    else if (tokens[0] == TOKEN0_KEYBOARD)
    {
      KeyboardClick((char)tokens[1]);
    }
  }
  delay(10);
}
