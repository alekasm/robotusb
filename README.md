# RobotUSB

RobotUSB is a Windows automation program that communicates to a compatible USB mouse/keyboard device desired output. The custom scripting language is simple but flexible to automate any number of tasks. Currently RobotUSB supports Arduino ATmega32U4 based boards such as the Leonardo.

## Setup
The following Windows settings are required to ensure the correct function of RobotUSB:
1. Display: Scale and layout is set to "100%"
2. Mouse Properties: Pointer speed is exactly in the "middle"
3. Mouse Properties: "Enhanced pointer precision" (mouse acceleration) is disabled

### Arduino Setup
Download and Install the Arduino IDE and hook up your ATmega32U4 based Arduino (such as the Leonardo). Ensure your board is not set on the Arduino Uno which is the default. You will also need to select the port (Tools -> Port) which your device is connected to. Take note of the "COM" number, such as "COM10". Open "robotusb.ino" in the Arduino IDE and then "Upload" it to your device. Once uploaded, you can start RobotUSB by selecting your desired script and COM number. Ensure Serial Monitor is NOT running, this will interfere with RobotUSB's ability to connect to your Arduino.

## Scripting Language
**WINDOW_ORIGIN,&lt;window name&gt;**  
*Sets the screen origin to the position of a window with the exact name*  
   
**OFFSET_ORIGIN,&lt;x&gt;,&lt;y&gt;**  
*Offsets the screen origin by the positive (or zero) x,y values*  

**SCREEN_SIZE,&lt;width&gt;,&lt;height&gt;**  
*Sets the dimensions of the screen relative to the origin. Improves color scan efficiency.*

**LOOP_COUNT,&lt;amount&gt;**  
*The amount of times to run the set of automation actions*

**MOUSE_COLOR,&lt;r&gt;,&lt;g&gt;,&lt;b&gt;,&lt;LEFT_CLICK|RIGHT_CLICK|NO_CLICK&gt;**  
*Moves the mouse to the first scanned RGB value, then performs a mouse action*  
*RGB values must be between 0 and 255*

**MOUSE,&lt;r&gt;,&lt;g&gt;,&lt;b&gt;,&lt;LEFT_CLICK|RIGHT_CLICK|NO_CLICK&gt;**  
*Moves the mouse to the first scanned RGB value, then performs a mouse action*  

**DELAY,&lt;milliseconds&gt;**  
*Sleeps for the amount of time specified before performing the next action*

## Scripting Description
Entering mouse commands are absolute to your screen origin, meaning that a mouse command at say 0,0 would move your mouse to the top-left part of your screen. However, if you specify a WINDOW_ORIGIN then a mouse command at 0,0 would move the mouse to the top-left part of that window. An OFFSET_ORIGIN command works with or without specifying a WINDOW_ORIGIN, it simply moves the origin some relative x,y amount. All the main input actions (mouse, delay) are processed in-order, and this entire set is processed the amount of times specified by LOOP_COUNT.

## Scripting Example
WINDOW_ORIGIN,RuneLite  
OFFSET_ORIGIN,4,27  
SCREEN_SIZE,765,503  
LOOP_COUNT,300  
MOUSE_COLOR,27,3,2,LEFT_CLICK  
DELAY,10000  

