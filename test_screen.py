import ctypes
import time

HWND_BROADCAST = 0xFFFF
WM_SYSCOMMAND = 0x0112
SC_MONITORPOWER = 0xF170

def screen_off():
    ctypes.windll.user32.SendMessageW(
        HWND_BROADCAST,
        WM_SYSCOMMAND,
        SC_MONITORPOWER,
        2  # power off
    )
    print("🔴 screen_off() called")

def screen_on():
    ctypes.windll.user32.SendMessageW(
        HWND_BROADCAST,
        WM_SYSCOMMAND,
        SC_MONITORPOWER,
        -1  # power on
    )
    print("🟢 screen_on() called")

print("WAIT 5 seconds…")
time.sleep(5)

print("EXIT → screen OFF")
screen_off()

print("WAIT 10 seconds…")
time.sleep(10)

print("ENTER → screen ON")
screen_on()
