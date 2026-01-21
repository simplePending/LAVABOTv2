# serial_comm.py
import serial
import time

arduino = serial.Serial('COM3', 9600, timeout=1)
time.sleep(2)

def send(cmd):
    """Send command to Arduino"""
    arduino.write((cmd + "\n").encode())
    time.sleep(0.1)  # Small delay for Arduino processing

def read_sensor():
    """Read photoelectric sensor state from Arduino"""
    if arduino.in_waiting > 0:
        response = arduino.readline().decode().strip()
        return response
    return "WAITING"