import asyncio
from bleak import BleakClient, BleakScanner
from bleak.backends.characteristic import BleakGATTCharacteristic
from pythonosc import udp_client

SERVICE_UUID = "29b26bc8-182d-4998-9b85-c70217b582da"
CHARACTERISTIC_UUID =  "2f57bc23-d2e5-4e33-910f-cba6f96ed89b"

ip = "127.0.0.1"
portOut = 5005
UDPclient = udp_client.SimpleUDPClient(ip, portOut)

def send_to_pd(data):
    prefix = "/x_impulse"
    val = int(data[0]) - 1
    UDPclient.send_message(prefix, val)
    prefix = "/y_impulse"
    val = int(data[1]) - 1
    UDPclient.send_message(prefix, val)
    prefix = "/push"
    val = int(data[2])
    UDPclient.send_message(prefix, val)
    prefix = "/hold"
    val = int(data[3])
    UDPclient.send_message(prefix, val)

def callback(sender: BleakGATTCharacteristic, data: bytearray):
    dataStr = str(data).strip("bytearray(b'").split("~")[0]
    send_to_pd(dataStr)
    #print(dataStr)

async def main():
    print("Starting scan...")
    device = await BleakScanner.find_device_by_name("Expresso")
    print("Connecting to device...")
    async with BleakClient(device) as client:
        print("Connected")
        while(client):
            await client.start_notify(CHARACTERISTIC_UUID, callback)

asyncio.run(main())
