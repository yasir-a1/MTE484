#Echoes everythign from the serial logger on the arduino side, 

import serial
import threading
import csv
from datetime import datetime

PORT = "COM4"
BAUD = 115200

filename = datetime.now().strftime("motor_log_%Y-%m-%d_%H-%M-%S.csv")

ser = serial.Serial(PORT, BAUD, timeout=1)

csv_file = open(filename, "w", newline="")
writer = csv.writer(csv_file)

print(f"\nConnected to {PORT}")
print(f"Logging data to: {filename}")
print("Enter target angle and press Enter.")
print("Type q to stop.\n")


def read_serial():
    while True:
        try:
            line = ser.readline().decode("utf-8", errors="ignore").strip()

            if line:
                print(line)

                # Only log CSV-formatted lines
                if "," in line:
                    writer.writerow(line.split(","))
                    csv_file.flush()

        except:
            break


# Serial reader runs in background
thread = threading.Thread(target=read_serial, daemon=True)
thread.start()


# Main thread handles user input
try:

    while True:

        command = input()

        if command.lower() == "q":
            break

        # Send target angle to Arduino
        ser.write((command + "\n").encode("utf-8"))

finally:

    csv_file.close()
    ser.close()

    print(f"\nLogging stopped.")
    print(f"Saved to: {filename}")