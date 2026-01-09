import asyncio
import time
from bleak import BleakScanner

PHONE_ADDRESS = "30:BB:7D:4C:3E:5C"

# -------- FINAL PARAMETERS --------
ENTER_RSSI = -85          # ENTER when close
EXIT_RSSI = -88           # EXIT when clearly far
EXIT_TIME = 10            # seconds (RSSI OR disappearance)
SEEN_TIMEOUT = 2          # scan jitter tolerance
SMOOTH_ALPHA = 0.3
# ---------------------------------

smoothRSSI = None
lastSeen = None
present = False
exitTimerRSSI = None


async def presence_loop():
    global smoothRSSI, lastSeen, present, exitTimerRSSI

    while True:
        devices = await BleakScanner.discover(timeout=1, return_adv=True)
        now = time.time()

        rssi = None

        for device, adv in devices.values():
            if device.address.upper() == PHONE_ADDRESS.upper():
                rssi = adv.rssi
                lastSeen = now
                break

        # Smooth RSSI only when beacon is seen
        if rssi is not None:
            if smoothRSSI is None:
                smoothRSSI = rssi
            else:
                smoothRSSI = smoothRSSI * (1 - SMOOTH_ALPHA) + rssi * SMOOTH_ALPHA

        seen_age = float("inf") if lastSeen is None else (now - lastSeen)

        # ---------------- ENTER ----------------
        if not present:
            if (
                seen_age < SEEN_TIMEOUT
                and smoothRSSI is not None
                and smoothRSSI > ENTER_RSSI
            ):
                present = True
                exitTimerRSSI = None
                on_action(smoothRSSI)

        # ---------------- EXIT -----------------
        else:
            # Condition A: beacon disappeared
            if seen_age > EXIT_TIME:
                present = False
                exitTimerRSSI = None
                off_action()

            # Condition B: RSSI very weak for long enough
            elif smoothRSSI is not None and smoothRSSI < EXIT_RSSI:
                if exitTimerRSSI is None:
                    exitTimerRSSI = now
                elif (now - exitTimerRSSI) > EXIT_TIME:
                    present = False
                    exitTimerRSSI = None
                    off_action()
            else:
                exitTimerRSSI = None  # signal recovered

        # -------- DEBUG --------
        seen_txt = "∞" if lastSeen is None else int(seen_age)
        rssi_txt = "None" if smoothRSSI is None else int(smoothRSSI)
        print(f"present={present}  seen={seen_txt}s  rssi={rssi_txt}")

        await asyncio.sleep(0.2)


def on_action(rssi):
    print(f"🟢 ENTER → ambience ON (RSSI={int(rssi)})")


def off_action():
    print("🔴 LEFT → ambience OFF")


asyncio.run(presence_loop())
