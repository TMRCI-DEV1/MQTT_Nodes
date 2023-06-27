The calibration process in this sketch is designed to allow you to set and save the head and tail positions for each track on 
the turntable. 

Here's a step-by-step explanation of the calibration process and the relevant functionality in the sketch:

The sketch starts by initializing the system, connecting to the WiFi network, and setting up the MQTT client.

After the WiFi and MQTT connections are established, the sketch enters the calibration mode. The LCD display shows a "Calibration Mode" 
message.

To calibrate a track, you need to enter a 1 or 2-digit track number on the keypad. The track number represents the track on the turntable that 
you want to calibrate.

Once you enter the track number, you can press the '*' key on the keypad to indicate that you want to set the head position of the track 
or the '#' key to set the tail position.

After pressing '*' or '#', the LCD display prompts you to confirm whether you want to save the current position as the head or tail position. 
Pressing '1' will save the position, while pressing '3' will cancel the calibration for that track.

If you press '1' to save the position, the sketch will update the head or tail position for the specified track in the trackHeads or trackTails 
array respectively.

The head and tail positions are also stored in the EEPROM memory using the EEPROM library. The EEPROM address for each track is calculated 
based on the track number.

The track number is cleared, and the LCD display is cleared to prepare for the calibration of the next track.

This process can be repeated for each track on the turntable, allowing you to calibrate the head and tail positions for all tracks.

The saved head and tail positions will be used later when the turntable is controlled via MQTT messages. When a track number and 
end (head or tail) are received in an MQTT message, the sketch will calculate the target position based on the track number and end 
number and move the stepper motor to that position.

The sketch also includes manual adjustment functionality. Pressing '4' on the keypad moves the turntable clockwise by small increments, 
and pressing '5' moves it counterclockwise by small increments. This can be used for fine-tuning the calibration if needed.

The current position of the turntable is continuously stored in the currentPosition variable, and it is also saved to the EEPROM memory 
to retain the last known position even after power cycling the system.
