# serial_comm.py
import serial
import time

arduino = None

def init_serial():
    global arduino
    try:
        arduino = serial.Serial("COM3", 9600, timeout=0.1)
        time.sleep(2)
        print("Arduino connected")
    except Exception as e:
        print("Arduino NOT connected:", e)
        arduino = None

def send(cmd):
    if arduino:
        arduino.write((cmd + "\n").encode())

def read_sensor():
    if arduino and arduino.in_waiting > 0:
        return arduino.readline().decode().strip()
    return "WAITING"
