import asyncio
import time
from bleak import BleakScanner

PHONE_ADDRESS = "30:BB:7D:4C:3E:5C"   # <-- your phone
RSSI_TRIGGER = -70                    # closer = -60, farther = -80
SEEN_TIMEOUT = 5                      # seconds since last seen
SHUTDOWN_CONFIRM = 5                  # must be weak for 5sec before OFF

smoothRSSI = None
lastSeen = 0
ledState = False
shutdownStart = None


async def presence_loop():
    global smoothRSSI, lastSeen, ledState, shutdownStart

    while True:
        # discover + get advertisement data
        devices = await BleakScanner.discover(timeout=1, return_adv=True)

        now = time.time()
        rssi = None

        # find our phone and read RSSI
        for (device, adv) in devices.values():
            if device.address.upper() == PHONE_ADDRESS.upper():
                rssi = adv.rssi
                lastSeen = now
                break

        # smooth the RSSI
        if rssi is not None:
            if smoothRSSI is None:
                smoothRSSI = rssi
            else:
                smoothRSSI = smoothRSSI * 0.7 + rssi * 0.3

        # --------- DECISION LOGIC ---------

        # TURN ON
        if (
            (now - lastSeen) < SEEN_TIMEOUT
            and smoothRSSI is not None
            and smoothRSSI > RSSI_TRIGGER
        ):
            if not ledState:
                ledState = True
                shutdownStart = None
                on_action(smoothRSSI)

        # TURN OFF (with confirmation delay)
        else:
            if ledState:
                if shutdownStart is None:
                    shutdownStart = now
                elif (now - shutdownStart) > SHUTDOWN_CONFIRM:
                    ledState = False
                    shutdownStart = None
                    off_action()

        await asyncio.sleep(0.2)


def on_action(rssi):
    print(f"🟢 ENTER detected (RSSI={int(rssi)}) → AMBIENCE ON")


def off_action():
    print("🔴 LEFT detected → AMBIENCE OFF")


asyncio.run(presence_loop())
