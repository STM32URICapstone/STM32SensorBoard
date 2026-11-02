# Ensure serial and matplotlib are both installed for your Python environment
# python -m pip install pyserial matplotlib
import serial
import matplotlib.pyplot as plt

# On my system, TTL-USB converter is on COM3, you may need to adjust for your system accordingly
PORT = "COM3"
BAUD = 115200
MAX_POINTS = 200     # how many samples to keep on screen (200 = 20 s at 10 Hz)

ser = serial.Serial(PORT, BAUD, timeout=0.1)

roll, pitch, yaw = [], [], []
accuracy = 0

# Setup plot
plt.ion()
fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(10, 7), sharex=True)
roll_line, = ax1.plot([], [], "r-", label="roll")
pitch_line, = ax1.plot([], [], "g-", label="pitch")
yaw_line, = ax2.plot([], [], "b-", label="yaw")

# Typical Roll & Pitch range: -180 to 180 deg
ax1.set_ylim(-180, 180)
ax1.set_ylabel("angle (deg)")
ax1.legend(loc="upper left")
ax1.grid(True)

# Typical Yaw range: 0 to 360 deg
ax2.set_ylim(0, 360)
ax2.set_ylabel("heading (deg)")
ax2.set_xlabel("sample")
ax2.legend(loc="upper left")
ax2.grid(True)

try:
    while plt.fignum_exists(fig.number):  # close the window to stop the script, (ctrl+c doesn't work, will add this as an option later)

        # 1) Read every line available from serial port
        while True:
            raw = ser.readline()
            if not raw:
                break  # timed out
            line = raw.decode("ascii", errors="ignore").strip()

            # 2) Parse line if the formatting fits. If not, skip.
            # This will need to be updated if the formatting defined in appmain.cpp changes.

            # current formatting (without accelerometer or magnetometer)
            # printf("%.2f\t%.2f\t%.2f\tacc=%u\r\n", a.roll_deg, a.pitch_deg, a.yaw_deg, a.accuracy);

            parts = line.split("\t")
            if len(parts) != 4 or not parts[3].startswith("acc="):
                continue
            try:
                r, p, y = float(parts[0]), float(parts[1]), float(parts[2])
                accuracy = int(parts[3][4:])
            except ValueError:
                continue

            # 3) Add to the arrays, cut off old samples to prevent clutter.
            # Modify MAX_POINTS or remove this entirely to see the entire plot
            roll.append(r)
            pitch.append(p)
            yaw.append(y)
            if len(roll) > MAX_POINTS:
                roll.pop(0)
                pitch.pop(0)
                yaw.pop(0)

            if ser.in_waiting == 0:
                break  # caught up, go draw

        # 4) Update and render
        x = range(len(roll))
        roll_line.set_data(x, roll)
        pitch_line.set_data(x, pitch)
        yaw_line.set_data(x, yaw)
        ax2.set_xlim(0, MAX_POINTS)
        if roll:
            fig.suptitle(f"roll {roll[-1]:.2f}   pitch {pitch[-1]:.2f}   "
                         f"yaw {yaw[-1]:.2f}   acc={accuracy}")
        plt.pause(0.05)  # draws the plot and handles window events

except KeyboardInterrupt:
    pass
finally:
    ser.close()