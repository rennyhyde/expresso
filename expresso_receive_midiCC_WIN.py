'''
Receives Bluetooth signal from Expresso and sends Midi CC via LoopMidi on Windows
'''

import rtmidi
import asyncio
from bleak import BleakClient, BleakScanner
from bleak.backends.characteristic import BleakGATTCharacteristic

### Connect Bluetooth ###

SERVICE_UUID = "29b26bc8-182d-4998-9b85-c70217b582da"
CHARACTERISTIC_UUID =  "2f57bc23-d2e5-4e33-910f-cba6f96ed89b"


# MIDI Control Parameters
xpos = 0.0
ypos = 0.0
prevX = 0.0
prevY = 0.0
prevButton = 0
deviceOn = False

button_cc = 0
x_cc = 1
y_cc = 2

midi_status = 0xb0  #The least significant hex digit is the midi channel -- change this if desired

# Control rate and curve
xGrowthRate = 1000        # Time (ms) that the control should take to move from 0.0 to 1.0 or 0.0 to -1.0
yGrowthRate = 1000        # Time (ms) that the control should take to move from 0.0 to 1.0 or 0.0 to -1.0
notifyRate = 3      # Time (ms) between bluetooth notifications -> DO NOT CHANGE THIS WITHOUT CHANGING ARDUINO CODE

LINEAR = 0
# To-do: Implement nonlinear curves
# EXPONENTIAL = 1
# LOGARITHMIC = 2

curveX = LINEAR      # Select a curve shape for the X parameter
curveY = LINEAR      # Select a curve shape for the Y parameter


### Initialize MIDI Port ###
async def initMidi():
    midiout = rtmidi.MidiOut()
    available_ports = midiout.get_ports()

    #print("Available ports: " + str(available_ports))

    # To use LoopMidi you must open the LoopMidi port before running this program
    if available_ports and len(available_ports) >= 2:
        midiout.open_port(1)    # Port 0 is 'Microsoft GS Wavetable Synth'
    else:
        #midiout.open_virtual_port("My virtual output")     # Virtual ports are not supported on windows :(
        raise IOError('LoopMidi port not found. Virtual ports are not supported on Windows.')
    return midiout

async def send_midiCC(data, midiout):
    #print(data)
    #midiout = initMidi()

    bVal = int(data[3])
    xVal = float(data[1])
    yVal = float(data[0])

    global xpos
    global ypos
    global prevButton
    global deviceOn
    global prevX
    global prevY



    #print(bVal)

    try:
        buttonDiff = bVal - prevButton
    except:
        buttonDiff = 0
        prevButton = bVal

    

    with midiout:
        if buttonDiff > 0:  # Rising edge
            deviceOn = not deviceOn
            midiout.send_message([midi_status, button_cc, 127 if deviceOn else 0])
        prevButton = bVal

        if curveX == LINEAR:
            xpos += float((xVal-1) * (notifyRate / xGrowthRate))
        else:
            raise ValueError('Nonlinear curves are not yet supported.')
        
        if curveY == LINEAR:
            ypos += float((yVal-1) * (notifyRate / yGrowthRate))
        else:
            raise ValueError('Nonlinear curves are not yet supported.')
    
        if (xpos < -1.0):
            xpos = -1.0
        if (xpos > 1.0):
            xpos = 1.0
        if (ypos < -1.0):
            ypos = -1.0
        if (ypos > 1.0):
            ypos = 1.0


        if (xpos != prevX):
            midiout.send_message([midi_status, x_cc, int(127 * ((xpos + 1.0)/2))])
        if (ypos != prevY):
            midiout.send_message([midi_status, y_cc, int(127 * ((ypos + 1.0)/2))])

        prevX = xpos
        prevY = ypos

async def callback(sender: BleakGATTCharacteristic, data: bytearray):
    dataStr = str(data).strip("bytearray(b'").split("~")[0]
    midiout = await initMidi()
    await send_midiCC(dataStr, midiout)
    #print(dataStr)

async def main():
    print("Starting scan...")
    device = await BleakScanner.find_device_by_name("Expresso")
    print("Connecting to device...")
    async with BleakClient(device) as client:
        print("Connected")
        #while(client):
        await client.start_notify(CHARACTERISTIC_UUID, callback)
        while (client):
            await asyncio.sleep(1)
        await client.stop_notify(CHARACTERISTIC_UUID)
        del midiout


asyncio.run(main())

