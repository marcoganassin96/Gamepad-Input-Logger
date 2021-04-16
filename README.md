# Using-Raw-Input-API-to-Process-Joystick-Input
Original source from: https://www.codeproject.com/Articles/185522/Using-the-Raw-Input-API-to-Process-Joystick-Input
Original author: Alexander Böcken

I recommend going to the URL for a walkthrough of how to access joystick devices via Raw Input.

Here's what's changed from the previous source:
* Allows user to record input from xBox type gamepads
* Recorded inputs are printed on files in folder "$Raw Input folder/recording"
* Press "Toggle view" button to start/stop recording
* Press "Map" button to quit the application safely 

Previously changed by Samuel Mörling:
* Get input from xbox type gamepads as well.
* Listen for messages even if the application is in the background.
