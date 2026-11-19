# Message Format
# Overview
Data from the sensor board will be delivered over UART in frames/packets, and will originate from the various sensors. All packets will include: syncword, originating sensor, data length, data, and a CRC. 

This setup was chosen to allow the Jetson (or other robot system) to ignore outputs from unneeded sensors.

Exact structure specified below.

# Protocol

## Parameters
| Name            | Value                  |
| :-------------- | :--------------------- |
| UART Baud Rate  | 115200 (default)       |
| Max Packet Size | 2048 bytes             |
| Max Data Size   | 2036 bytes             |
| Sync Word       | 0x55AA55AA             |
| Endianness      | Little-Endian          |

## Structure

<table style="border-collapse:collapse;border-spacing:0;border:1px solid currentColor;table-layout:fixed;width:100%" class="tg">
  <thead>
    <tr>
      <th style="border:1px solid currentColor;font-family:Arial, sans-serif;font-size:14px;font-weight:bold;overflow:hidden;padding:10px 5px;text-align:center;vertical-align:top;word-break:normal;width:20%">Word #</th>
      <th style="border:1px solid currentColor;font-family:Arial, sans-serif;font-size:14px;font-weight:bold;overflow:hidden;padding:10px 5px;text-align:center;vertical-align:top;word-break:normal;width:20%">Byte 0</th>
      <th style="border:1px solid currentColor;font-family:Arial, sans-serif;font-size:14px;font-weight:bold;overflow:hidden;padding:10px 5px;text-align:center;vertical-align:top;word-break:normal;width:20%">Byte 1</th>
      <th style="border:1px solid currentColor;font-family:Arial, sans-serif;font-size:14px;font-weight:bold;overflow:hidden;padding:10px 5px;text-align:center;vertical-align:top;word-break:normal;width:20%">Byte 2</th>
      <th style="border:1px solid currentColor;font-family:Arial, sans-serif;font-size:14px;font-weight:bold;overflow:hidden;padding:10px 5px;text-align:center;vertical-align:top;word-break:normal;width:20%">Byte 3</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td style="border:1px solid currentColor;font-family:Arial, sans-serif;font-size:14px;padding:10px 6px;text-align:center;vertical-align:middle">0</td>
      <td style="border:1px solid currentColor;font-family:Arial, sans-serif;font-size:14px;padding:10px 6px;text-align:center;vertical-align:middle" colspan="4">Sync Word</td>
    </tr>
    <tr>
      <td style="border:1px solid currentColor;font-family:Arial, sans-serif;font-size:14px;padding:10px 6px;text-align:center;vertical-align:middle">1</td>
      <td style="border:1px solid currentColor;font-family:Arial, sans-serif;font-size:14px;padding:10px 6px;text-align:center;vertical-align:middle">Sensor ID</td>
      <td style="border:1px solid currentColor;font-family:Arial, sans-serif;font-size:14px;padding:10px 6px;text-align:center;vertical-align:middle">Sequence #</td>
      <td style="border:1px solid currentColor;font-family:Arial, sans-serif;font-size:14px;padding:10px 6px;text-align:center;vertical-align:middle" colspan="2">Data Length</td>
    </tr>
    <tr>
      <td style="border:1px solid currentColor;font-family:Arial, sans-serif;font-size:14px;padding:10px 6px;text-align:center;vertical-align:middle">2</td>
      <td style="border:1px solid currentColor;font-family:Arial, sans-serif;font-size:14px;padding:10px 6px;text-align:center;vertical-align:middle" colspan="4">UTC (ms since midnight)</td>
    </tr>
    <tr>
      <td style="border:1px solid currentColor;font-family:Arial, sans-serif;font-size:14px;padding:10px 6px;text-align:center;vertical-align:middle">3</td>
      <td style="border:1px solid currentColor;font-family:Arial, sans-serif;font-size:14px;padding:10px 6px;text-align:center;vertical-align:middle">Month</td>
      <td style="border:1px solid currentColor;font-family:Arial, sans-serif;font-size:14px;padding:10px 6px;text-align:center;vertical-align:middle">Day</td>
      <td style="border:1px solid currentColor;font-family:Arial, sans-serif;font-size:14px;padding:10px 6px;text-align:center;vertical-align:middle" colspan="2">Year</td>
    </tr>
    <tr>
      <td style="border:1px solid currentColor;font-family:Arial, sans-serif;font-size:14px;padding:10px 6px;text-align:center;vertical-align:middle">4...N-1</td>
      <td style="border:1px solid currentColor;font-family:Arial, sans-serif;font-size:14px;padding:10px 6px;text-align:center;vertical-align:middle" colspan="4">Data Bytes</td>
    </tr>
    <tr>
      <td style="border:1px solid currentColor;font-family:Arial, sans-serif;font-size:14px;padding:10px 6px;text-align:center;vertical-align:middle">N</td>
      <td style="border:1px solid currentColor;font-family:Arial, sans-serif;font-size:14px;padding:10px 6px;text-align:center;vertical-align:middle" colspan="4">32-Bit CRC</td>
    </tr>
  </tbody>
</table>