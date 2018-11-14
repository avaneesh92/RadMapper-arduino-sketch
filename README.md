# Arduino serial motor driver
Arduino based serial motor controller. Accepts commands from serial port and drive two motors . Rover must be built in differential drive config (eg Tank type). 
## Features
## Hardware Connections
 *   Arduino Pin 2 connected to RX via 330 ohm resister to enable wakeup on receive 
 *   Left motor on Arduino pin 4 and 5
 *   Right motor on Arduino pin 6 and 7
## API
### Command Format 
* Command format "#>CMD Speed Timeout<"
* "#" has to be added before start of each command to wake up arduino from sleep
* ">" marks start of command and "<" marks end of command
* Range for speed is 0 to 255
* Range for timeout is 1 to 10000 milli seconds (0 is treated as no timeout, keep motors running)   
* Return value "OK" indicates that command is received and will be executed immediately
* Return value "ERROR number" indicates error number 
   
### COMMANDS
* => ">FW speed timeout<" # Move forward
* => ">BW speed timeout<" # Move backward
* => ">RT speed timeout<" # Turn right with given speed
* => ">LT speed timeout<" # Turn left with given speed
* => ">STOP 0 0<"         # Stop moving

### ERROR Numbers
* 1 => "Invalid Command"
* 2 => "End of command not received within 100 ms"
* 3 => "Invalid Speed value"
* 4 => "Invalid Timeout value"
