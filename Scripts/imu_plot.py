import serial
import matplotlib.pyplot as plt
import cstruct
from dotenv import load_dotenv
import os
import math
import time
from collections import deque

load_dotenv(".env", override=True)

PORT = os.environ.get("JETSON_UART_PORT", "COM3")
BAUD = int(os.environ.get("JETSON_UART_BAUD", 115200))
MAX_POINTS = 200
RENDER_FPS = 30
RENDER_INTERVAL = 1.0 / RENDER_FPS


class Header(cstruct.MemCStruct):
    __byte_order__ = cstruct.LITTLE_ENDIAN
    __def__ = """
        struct {
            uint32_t sync_word;
            uint8_t sensor_id;
            uint8_t sequence_num;
            uint16_t data_length;
            uint32_t crc;
        };
    """


class IMUData(cstruct.MemCStruct):
    __byte_order__ = cstruct.LITTLE_ENDIAN
    __def__ = """
        struct {
            float acc_x;
            float acc_y;
            float acc_z;
            float quat_i;
            float quat_j;
            float quat_k;
            float quat_real;
            float mag_x;
            float mag_y;
            float mag_z;
        };
    """


PACKET_SIZE = Header.__size__ + IMUData.__size__
SYNC_BYTES = (0x55AA55AA).to_bytes(4, byteorder="little")


def quat_to_euler(qr, qi, qj, qk):
    """Converts a quaternion to Euler angles (Roll, Pitch, Yaw) in degrees."""
    sinr_cosp = 2 * (qr * qi + qj * qk)
    cosr_cosp = 1 - 2 * (qi * qi + qj * qj)
    roll = math.atan2(sinr_cosp, cosr_cosp)

    sinp = 2 * (qr * qj - qk * qi)
    if abs(sinp) >= 1:
        pitch = math.copysign(math.pi / 2, sinp)
    else:
        pitch = math.asin(sinp)

    siny_cosp = 2 * (qr * qk + qi * qj)
    cosy_cosp = 1 - 2 * (qj * qj + qk * qk)
    yaw = math.atan2(siny_cosp, cosy_cosp)

    return math.degrees(roll), math.degrees(pitch), math.degrees(yaw)


ser = serial.Serial(PORT, BAUD, timeout=0)
ser.reset_input_buffer()

# deque automatically drops old items when maxlen is exceeded (O(1) sliding window)
roll = deque(maxlen=MAX_POINTS)
pitch = deque(maxlen=MAX_POINTS)
yaw = deque(maxlen=MAX_POINTS)
acc_x = deque(maxlen=MAX_POINTS)
acc_y = deque(maxlen=MAX_POINTS)
acc_z = deque(maxlen=MAX_POINTS)

plt.ion()
fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(10, 7), sharex=True)
(roll_line,) = ax1.plot([], [], "r-", label="roll")
(pitch_line,) = ax1.plot([], [], "g-", label="pitch")
(yaw_line,) = ax1.plot([], [], "b-", label="yaw")

(acc_x_line,) = ax2.plot([], [], "r-", label="acc_x")
(acc_y_line,) = ax2.plot([], [], "g-", label="acc_y")
(acc_z_line,) = ax2.plot([], [], "b-", label="acc_z")

ax1.set_ylim(-180, 180)
ax1.set_ylabel("angle (deg)")
ax1.legend(loc="upper left")
ax1.grid(True)

ax2.set_ylim(-15, 15)
ax2.set_ylabel("acceleration (m/s^2)")
ax2.set_xlabel("sample")
ax2.legend(loc="upper left")
ax2.grid(True)

plt.show(block=False)

raw_buffer = bytearray()
last_render_time = time.time()

try:
    while plt.fignum_exists(fig.number):
        num_bytes = ser.in_waiting
        if num_bytes > 0:
            raw_buffer.extend(ser.read(num_bytes))

        # Process all available packets in the buffer instantly
        while len(raw_buffer) >= PACKET_SIZE:
            sync_idx = raw_buffer.find(SYNC_BYTES)

            if sync_idx == -1:
                raw_buffer = raw_buffer[-3:]
                break

            if sync_idx > 0:
                raw_buffer = raw_buffer[sync_idx:]

            if len(raw_buffer) < PACKET_SIZE:
                break

            packet_data = raw_buffer[:PACKET_SIZE]
            raw_buffer = raw_buffer[PACKET_SIZE:]

            header = Header()
            header.unpack(bytes(packet_data[: Header.__size__]))

            if header.sensor_id != 1:
                continue

            imu = IMUData()
            imu.unpack(bytes(packet_data[Header.__size__ : PACKET_SIZE]))

            r, p, y = quat_to_euler(imu.quat_real, imu.quat_i, imu.quat_j, imu.quat_k)

            roll.append(r)
            pitch.append(p)
            yaw.append(y)
            acc_x.append(imu.acc_x)
            acc_y.append(imu.acc_y)
            acc_z.append(imu.acc_z)

        # Rendering throttle: Only update the GUI if enough time has passed
        current_time = time.time()
        if roll and (current_time - last_render_time) >= RENDER_INTERVAL:
            last_render_time = current_time

            # Convert deques to lists for Matplotlib
            x = range(len(roll))
            roll_line.set_data(x, list(roll))
            pitch_line.set_data(x, list(pitch))
            yaw_line.set_data(x, list(yaw))
            acc_x_line.set_data(x, list(acc_x))
            acc_y_line.set_data(x, list(acc_y))
            acc_z_line.set_data(x, list(acc_z))

            ax2.set_xlim(0, MAX_POINTS)

            fig.suptitle(
                f"Roll: {roll[-1]:.2f}°   Pitch: {pitch[-1]:.2f}°   Yaw: {yaw[-1]:.2f}°"
            )

            # Flush GUI events without blocking the parsing thread
            fig.canvas.draw_idle()
        fig.canvas.flush_events()

        # Yield a tiny amount to prevent 100% CPU pegging if buffer is completely empty
        if ser.in_waiting == 0:
            time.sleep(0.001)

except KeyboardInterrupt:
    pass
finally:
    ser.close()
