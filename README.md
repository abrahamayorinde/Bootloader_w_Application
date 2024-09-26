# Bootloader_w_Application
A bootloader application that allows the application (also an application) to be updated via UART.

The flash memory, 1024KB, is split into two to accomodate the two binaries, the bootloader binary and the application binary.  The bootloader is allotted bootloader and the remaining is left for the application.

The boot requires a communication channel to receive the software update.  USART2 is used for this purpose. 

The initial intention was to poll the USART for the data, however receiving one byte at a time proved problematic because the target and PC would often lose synchronization causing the PC to abort the upload and the target to fail the download.
The alternative was to use the interrupt method.  The STM32 interrupt requires knowing how much data will be received prior to updating setting the interrupt.  Fortunately the protocol sends along with it the expected number of bytes prior to the download.  This required adapting the routine so that the 

Start/Initial Frame
| Start of Frame (SOF) | Packet Type | Data Length | Command | CRC | End of Frame (EOF)
| -------- | ------- |------- |------- |------- |------- |
| 1 byte | 1 byte | 2 bytes | 1 byte | 4 bytes | 1 byte |
| 0xAA | 0x00 | 0x01 | 0x00 | 0x00 | 0xBB |


Header Frame
| Start of Frame (SOF) | Packet Type | Data Length | Meta Data | CRC | End of Frame (EOF)
| -------- | ------- |------- |------- |------- |------- |
| 1 byte | 1 byte | 2 bytes | 16 bytes | 4 bytes | 1 byte |
| 0xAA | 0x02 | 2 bytes | (Package size, package crc, reserved1, reserved 2)| 0x00 | 0xBB|


Data Frame
| Start of Frame (SOF) | Packet Type | Data Length | Data | CRC | End of Frame (EOF)
| -------- | ------- |------- |------- |------- |------- |
| 1 byte | 1 byte | 2 bytes | n bytes | 4 bytes | 1 byte |
| 0xAA | 0x01 | (length of data) | (Data length bytes) | 0x00 | 0xBB |

End Frame
| Start of Frame (SOF) | Packet Type | Data Length | Command | CRC | End of Frame (EOF)
| -------- | ------- |------- |------- |------- |------- |
| 1 byte | 1 byte | 2 bytes | 1 byte | 4 bytes | 1 byte |
| 0xAA | 0x00 | 0x01 | 0x01 | 0x00 | 0xBB |

Target Response/Acknowledge Frame
| Start of Frame (SOF) | Packet Type | Data Length | Status | CRC | End of Frame (EOF)
| -------- | ------- |------- |------- |------- |------- |
| 0xAA | 0x03 | 0x01 | (0x00 or 0x01) | 0x00 | 0xBB |
