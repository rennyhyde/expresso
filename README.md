# expresso
Expresso is a pocket-sized MIDI/OSC controller for electroacoustic musicians. It consists of a joystick which straps easily to any instrument and transmits data via Bluetooth LE to the performer's laptop. This data can then be converted to MIDI CC or OSC for seamless integration with a DAW such as Ableton Live.

Expresso's intended use-case is as a virtual guitar pedal. The performer can control two parameters of an effect with the X and Y directions of the joystick, and can turn the effect on or off by pressing down.

## Hardware
Expresso is built on the Adafruit QT-PY ESP32-C3 microcontroller. It's important to note when using this microcontroller that you need to manually switch it in and out of boot mode whenever you flash new code. To enter boot mode: press 'reset', press 'boot', release 'reset', release 'boot'. To exit boot mode: press 'reset'.

Three of the QT-PY's analog input pins are connected to the three data pins of the HW-504 joystick (commonly found in Arduino sensor kits), which correspond to the X inclination, Y inclination, and the built-in button.

The 5V and GND pins from the QT PY and joystick should also be connected.

Version 1 of Expresso gets power from the 5V USB connection, so it should ideally be connected to a portable charger during use.

## expresso.ino
`expresso.ino` is the software running on the QT PY. As soon as it's connected to power, it starts advertising its Bluetooth connection. Once it connects to Bluetooth it starts sending the state of the joystick every 3ms.

The joystick isn't particularly accurate, so its directions are treated as ternary. In other words, in the X direction the joystick can either be in the center, fully right, or fully left, without any grey area in between.

The joystick's state is sent (updated in a custom BLE characteristic) as a string of 4 digits followed by a `~` stop character. From MSD to LSD, those digits represent:

- X location: 0 for left, 1 for center, 2 for right
- Y location: 0 for down, 1 for center, 2 for up
- Button Press: 1 in the moment when the button is pressed, 0 otherwise
- Button Hold: 1 *while* the button is being pressed, 0 otherwise

An example data string could look like `0101~` when the joystick is pointing to the left and actively being pressed, or `2210~` when the joystick is pointing up and to the right and has just been pressed.

## Receiving Expresso messages
Python is used on the client-side to receive messages from the Bluetooth connection to expresso and send them to the appropriate software. There are two ways you can do this, using MIDI CC or OSC. MIDI CC is recommended if you plan to use Expresso to control effects, as this will integrate seamlessly with a DAW. OSC is recommended for any other applications, such as playing an accompanying electronic instrument.

There's no need to connect to Expresso through your laptop's Bluetooth menu, as the python programs will take care of it for you. The programs will stop automatically (but not elegantly) when Expresso disconnects, or you can keyboard interrupt to stop execution early.

## MIDI CC
*Note: Be sure to use the WIN or MAC specific versions of this script, as Mac natively supports virtual MIDI ports while Windows does not. For Windows, I recommend using LoopMIDI to open a virtual port.*

Dependencies: [python-rtmidi](https://pypi.org/project/python-rtmidi/), [asyncio](https://pypi.org/project/asyncio/), [bleak](https://pypi.org/project/bleak/)

1. Install dependencies
2. (Windows only) Open LoopMIDI port, if not open already
3. Run `expresso_receive_midiCC_WIN.py` (Windows) or `expresso_receive_midiCC_MAC.py` (Mac)

Now expresso will be sending control changes to the laptop's internal MIDI loopback, so just open up your DAW, create some MIDI mappings, and you're good to go.

The 'Button' control is CC 0, and sends 127 when the pedal is 'on' and 0 when the pedal is 'off'. 

The 'X' parameter is CC 1, and will send some integer from 0 to 127 indicating the position of that control knob. Right now this is done as a linear ramp as long as the joystick is being held in a particular direction, but in the future this could be any function, such as exponential or logarithmic. The time it takes to go from the lowest to highest value is set to 2 seconds by default, but this can be changed in the python file by editing the `xGrowthRate` or `yGrowthRate` variables.

The 'Y' parameter is CC 2, and functions the same as the 'X' parameter above.

## OSC
Dependencies: [python-osc](https://pypi.org/project/python-osc/), [asyncio](https://pypi.org/project/asyncio/), [bleak](https://pypi.org/project/bleak/)

`expresso_receive_osc_impulse.py` can be used to receive Bluetooth messages from Expresso and convert them to OSC messages broadcast internally over UDP. Not much processing is done in the python file, allowing you the most data possible from the joystick.

Data from the joystick is transmitted as four separate OSC control variables: `/x_impuilse`, `/y_impuilse`, `/push`, `/hold`. It's important to note that the x and y impulses will either be -1, 0, or 1 to allow for easier post-processing of the OSC messages. The push and hold commands will be 0 if inactive and 1 if active.



