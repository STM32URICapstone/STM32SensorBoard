# Ensure serial and matplotlib are both installed for your Python environment
# python -m pip install pyserial matplotlib cstruct python-dotenv
import serial
import matplotlib.pyplot as plt
import cstruct
from dotenv import load_dotenv
import os
import math

load_dotenv(".env", override=True)

PORT = os.environ.get("JETSON_UART_PORT", "COM3")
BAUD = int(os.environ.get("JETSON_UART_BAUD", 115200))
MAX_POINTS = 200


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
    # Roll (x-axis rotation)
    sinr_cosp = 2 * (qr * qi + qj * qk)
    cosr_cosp = 1 - 2 * (qi * qi + qj * qj)
    roll = math.atan2(sinr_cosp, cosr_cosp)

    # Pitch (y-axis rotation)
    sinp = 2 * (qr * qj - qk * qi)
    if abs(sinp) >= 1:
        pitch = math.copysign(math.pi / 2, sinp)  # Use 90 degrees if out of range
    else:
        pitch = math.asin(sinp)

    # Yaw (z-axis rotation)
    siny_cosp = 2 * (qr * qk + qi * qj)
    cosy_cosp = 1 - 2 * (qj * qj + qk * qk)
    yaw = math.atan2(siny_cosp, cosy_cosp)

    return math.degrees(roll), math.degrees(pitch), math.degrees(yaw)


ser = serial.Serial(PORT, BAUD, timeout=0.1)

roll, pitch, yaw = [], [], []

plt.ion()
fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(10, 7), sharex=True)
(roll_line,) = ax1.plot([], [], "r-", label="roll")
(pitch_line,) = ax1.plot([], [], "g-", label="pitch")
(yaw_line,) = ax2.plot([], [], "b-", label="yaw")

ax1.set_ylim(-180, 180)
ax1.set_ylabel("angle (deg)")
ax1.legend(loc="upper left")
ax1.grid(True)

ax2.set_ylim(-180, 180)  # Adjusted to standard -180 to 180 yaw range from atan2
ax2.set_ylabel("heading (deg)")
ax2.set_xlabel("sample")
ax2.legend(loc="upper left")
ax2.grid(True)

raw_buffer = bytearray()

try:
    while plt.fignum_exists(fig.number):

        # 1) Slurp all waiting bytes into our sliding window buffer
        num_bytes = ser.in_waiting
        if num_bytes > 0:
            raw_buffer.extend(ser.read(num_bytes))

        # 2) Process all complete packets in the buffer
        while len(raw_buffer) >= PACKET_SIZE:
            sync_idx = raw_buffer.find(SYNC_BYTES)

            if sync_idx == -1:
                # Sync word not found. Toss the garbage, but keep the last 3 bytes
                # in case they are the beginning of a cut-off sync word.
                raw_buffer = raw_buffer[-3:]
                break

            if sync_idx > 0:
                # Trash the garbage bytes before the sync word
                raw_buffer = raw_buffer[sync_idx:]

            if len(raw_buffer) < PACKET_SIZE:
                # We have the sync word, but the packet hasn't finished arriving. Wait.
                break

            # 3) We have a perfectly aligned packet. Extract and advance buffer.
            packet_data = raw_buffer[:PACKET_SIZE]
            raw_buffer = raw_buffer[PACKET_SIZE:]

            header = Header()
            header.unpack(bytes(packet_data[: Header.__size__]))

            # IMU is Sensor ID 2
            if header.sensor_id != 1:
                print("only IMU data supported at this time")
                continue

            imu = IMUData()
            imu.unpack(bytes(packet_data[Header.__size__ : PACKET_SIZE]))

            # 4) Convert Quaternions to Euler for plotting
            r, p, y = quat_to_euler(imu.quat_real, imu.quat_i, imu.quat_j, imu.quat_k)

            roll.append(r)
            pitch.append(p)
            yaw.append(y)

            if len(roll) > MAX_POINTS:
                roll.pop(0)
                pitch.pop(0)
                yaw.pop(0)

        # 5) Render the updated data
        if roll:
            x = range(len(roll))
            roll_line.set_data(x, roll)
            pitch_line.set_data(x, pitch)
            yaw_line.set_data(x, yaw)

            # Shift x-axis to create a scrolling effect
            ax2.set_xlim(0, MAX_POINTS)

            fig.suptitle(
                f"Roll: {roll[-1]:.2f}°   Pitch: {pitch[-1]:.2f}°   Yaw: {yaw[-1]:.2f}°"
            )

        plt.pause(0.01)

except KeyboardInterrupt:
    pass
finally:
    ser.close()
