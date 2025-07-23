# Light Phone
A LED display that moves a wave like pattern across a screen and displays notification updates from your iphone  

## How it works
ESP32 controller runs a RTOS with two main tasks:  
1. Display wave pattern on screen  
2. Read BLE notifications from a connected iphone

When a iphone notification of a predefined type is received, the RTOS task updates the LED display to alert the user.  

## Why I am building this
Me checking my phone often takes me out of the flow state when I am trying to be productive at my computer.  
I try to put my phone out of sight when I want to stay focused, but in the back of my mind I am concerned I might miss an urgent notification.  
I regularly find myself looking at my phone lock screen to check it for any important notifications.  
The Light Phone will act as a passive background display I can put in my FOV to put my mind at ease.  
It will only update its display when I get a certain type of notification, e.g. a text from a family member.   
In the future I can add some semantic parsing to see if a text message is actually urgent, but that is just a nice to have for now. 

## Steps:
1. Get hardware setup, verify basic WS2812b LED matrix commands work
2. Get LED wave display task working on 16x16 matrix
3. Create second task that updates display with simulated notifications
4. Ensure both tasks run smoothly on FreeRTOS with Arduino framework
5. Create BLE ANCS iphone driver that recevies notifications from phone
6. Update 16x16 matrix when BLE notifications are received
7. Integrate into RTOS and verify everything runs smoothly
8. 3D print diffusion screen that goes across the LED matrix and makes it look prettier
9. Add button functionality to turn on or off notification update task for really focused work
10. Add finesses touches to make the display and notification updates really pretty, BLE connection indicator
