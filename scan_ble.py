import asyncio
from bleak import BleakScanner

async def main():
    print("Scanning... keep phone nearby and Bluetooth ON")

    devices = await BleakScanner.discover(timeout=8, return_adv=True)

    for device, adv in devices.values():
        name = device.name or "Unknown"
        print(f"{device.address}  {name}  RSSI={adv.rssi}")

asyncio.run(main())
