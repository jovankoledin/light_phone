# Light Phone
A LED display that moves a wave like pattern across a screen and displays notification updates from your iphone  

## How it works
ESP32 controller runs a RTOS with two main tasks:  
1. Display wave pattern on screen  
2. Read BLE notifications from a connected iphone  
When a iphone notification of a predefined type is received, the RTOS task updates the LED display to alert the user.  

## Why I am building this
Staying focused and in a flow state is hard, and my phone is usually the most distracting thing I have.  
I try to put my phone out of sight when I want to stay focused, but in the back of my mind I am concerned I might miss an urgent notification.  
I regularly find myself looking at my phone lock screen to check it for any important notifications. 
The Light Phone will act as a passive background display I can put in my FOV to put my mind at ease. 
It will only update its display when I get a notification from certain sources, e.g. a text from a family member.  
In the future I can add some semantic parsing to see if a text message is actually urgent, but that is just a nice to have for now. 
