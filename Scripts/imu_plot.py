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
MAX_POINTS = 200  # firmware sends at ~100 Hz, so this is a ~2 s window
RENDER_FPS = 30
RENDER_INTERVAL = 1.0 / RENDER_FPS
IMU_SENSOR_ID = 1


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
            float acc_x;      /* m/s^2, includes gravity */
            float acc_y;
            float acc_z;
            float yaw_vel;
            float pitch_vel;
            float roll_vel;
            float temp_c;
        };
    """


PACKET_SIZE = Header.__size__ + IMUData.__size__
SYNC_BYTES = (0x55AA55AA).to_bytes(4, byteorder="little")


# def quat_to_euler(qr, qi, qj, qk):
#     """Converts a quaternion to Euler angles (Roll, Pitch, Yaw) in degrees."""
#     sinr_cosp = 2 * (qr * qi + qj * qk)
#     cosr_cosp = 1 - 2 * (qi * qi + qj * qj)
#     roll = math.atan2(sinr_cosp, cosr_cosp)

#     sinp = 2 * (qr * qj - qk * qi)
#     if abs(sinp) >= 1:
#         pitch = math.copysign(math.pi / 2, sinp)
#     else:
#         pitch = math.asin(sinp)

#     siny_cosp = 2 * (qr * qk + qi * qj)
#     cosy_cosp = 1 - 2 * (qj * qj + qk * qk)
#     yaw = math.atan2(siny_cosp, cosy_cosp)

#     return math.degrees(roll), math.degrees(pitch), math.degrees(yaw)


ser = serial.Serial(PORT, BAUD, timeout=0)
ser.reset_input_buffer()

# deque automatically drops old items when maxlen is exceeded (O(1) sliding window)
acc_x = deque(maxlen=MAX_POINTS)
acc_y = deque(maxlen=MAX_POINTS)
acc_z = deque(maxlen=MAX_POINTS)

roll = deque(maxlen=MAX_POINTS)
pitch = deque(maxlen=MAX_POINTS)
yaw = deque(maxlen=MAX_POINTS)

temp_c = deque(maxlen=MAX_POINTS)
# mag_x = deque(maxlen=MAX_POINTS)
# mag_y = deque(maxlen=MAX_POINTS)
# mag_z = deque(maxlen=MAX_POINTS)

plt.ion()
fig, (ax1, ax2, ax3) = plt.subplots(3, 1, figsize=(10, 9), sharex=True)
(roll_line,) = ax1.plot([], [], "r-", label="roll")
(pitch_line,) = ax1.plot([], [], "g-", label="pitch")
(yaw_line,) = ax1.plot([], [], "b-", label="yaw")

(acc_x_line,) = ax2.plot([], [], "r-", label="acc_x")
(acc_y_line,) = ax2.plot([], [], "g-", label="acc_y")
(acc_z_line,) = ax2.plot([], [], "b-", label="acc_z")

(temp_c_line,) = ax3.plot([], [], "r-", label="temp_c")

# (mag_x_line,) = ax3.plot([], [], "r-", label="mag_x")
# (mag_y_line,) = ax3.plot([], [], "g-", label="mag_y")
# (mag_z_line,) = ax3.plot([], [], "b-", label="mag_z")

ax1.set_ylim(-500, 500)
ax1.set_ylabel("angular vel. (deg/s)")
ax1.legend(loc="upper left")
ax1.grid(True)

ax2.set_ylim(-3, 3)
ax2.set_ylabel("acceleration (g)")
ax2.legend(loc="upper left")
ax2.grid(True)

ax3.set_ylim(0, 30)
ax3.set_ylabel("temperature (C)")
ax3.legend(loc="upper left")
ax3.grid(True)

# ax3.set_ylim(-100, 100)  # Earth's field is ~25-65 uT
# ax3.set_ylabel("magnetic field (uT)")
# ax3.set_xlabel("sample")
# ax3.legend(loc="upper left")
# ax3.grid(True)

plt.show(block=False)

raw_buffer = bytearray()
last_render_time = time.time()

# Packet statistics
last_seq = None
dropped_packets = 0
packet_count = 0
rate_window_start = time.time()
packet_rate = 0.0

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

            header = Header()
            header.unpack(bytes(raw_buffer[: Header.__size__]))

            # A false sync match inside payload data won't have a sane header: skip one byte
            # and search again instead of throwing away a whole packet's worth of data.
            if header.sensor_id != IMU_SENSOR_ID or header.data_length != IMUData.__size__:
                raw_buffer = raw_buffer[1:]
                continue

            packet_data = raw_buffer[:PACKET_SIZE]
            raw_buffer = raw_buffer[PACKET_SIZE:]

            if last_seq is not None:
                dropped_packets += (header.sequence_num - last_seq - 1) % 256
            last_seq = header.sequence_num
            packet_count += 1

            imu = IMUData()
            imu.unpack(bytes(packet_data[Header.__size__ : PACKET_SIZE]))

            # r, p, y = quat_to_euler(imu.quat_real, imu.quat_i, imu.quat_j, imu.quat_k)

            roll.append(imu.roll_vel)
            pitch.append(imu.pitch_vel)
            yaw.append(imu.yaw_vel)
            acc_x.append(imu.acc_x)
            acc_y.append(imu.acc_y)
            acc_z.append(imu.acc_z)
            temp_c.append(imu.temp_c)
            # mag_x.append(imu.mag_x)
            # mag_y.append(imu.mag_y)
            # mag_z.append(imu.mag_z)

        # Rendering throttle: Only update the GUI if enough time has passed
        current_time = time.time()
        if current_time - rate_window_start >= 1.0:
            packet_rate = packet_count / (current_time - rate_window_start)
            packet_count = 0
            rate_window_start = current_time

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
            temp_c_line.set_data(x, list(temp_c))
            # mag_x_line.set_data(x, list(mag_x))
            # mag_y_line.set_data(x, list(mag_y))
            # mag_z_line.set_data(x, list(mag_z))

            ax3.set_xlim(0, MAX_POINTS)

            # mag_norm = math.sqrt(mag_x[-1] ** 2 + mag_y[-1] ** 2 + mag_z[-1] ** 2)
            fig.suptitle(
                f"Roll_vel: {roll[-1]:.2f}°   Pitch_vel: {pitch[-1]:.2f}°   Yaw_vel: {yaw[-1]:.2f}°\n"
                # f"|B|: {mag_norm:.1f} uT   {packet_rate:.0f} pkt/s   dropped: {dropped_packets}"
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