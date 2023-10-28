| Supported Targets | ESP32 |

# Experimental AllFlex RFID reader.

AllFlex is a trademark


The goal of this project is to read ear tags used for cow identification.

Documentation and experiments show that the ear tag uses an RF frequency of 134.2 kHz for activation and FSK modulation (124.2 + 134.2 kHz) for response. (HDX protocol)

It relies on ISO 11784/5 documents

This project was tested with ear tags only. Only HDX is implemented.


### Hardware

 - One TX coil: 80 mm in diameter, approximately 80 turns, tuned to 134.2 Khz
 - One RX coil: 80 mm diameter, approximately 50 turns, set to 129.2 Khz
- TX part: a push-pull stage driven by an HC logic gate connected to the devkit esp32 module (output).
- RX part: a common emitter stage followed by an HC logic gates connected to the devkit esp32 module (input).
- An ESP devkit module to run software

**pinout:**
- GPIO19 : signal from tag (FSK_SIGNAL_INPUT_IO)
- GPIO21 : Output signal for debugging purpose (FSK_PROBE_OUTPUT_IO)
- GPIO25 : Ouput sigal for tag activation (CLOCKGEN_OUTPUT_IO)
- GPIO27 : Output signal useful to synchronize logic analyzer (CLOCKGEN_MOD_OUTPUT_IO)


### Output

The project uses ESP32 Classic Bluetooth Serial Port Profile (SPP)
Communication was tested with open source "Bluetooth Terminal" Android application.
Output could be simply sent to hardware UART (uart1)

### Configure the project

idf.py menuconfig

 **Important:**
 Disable Watch dog reset for CPU1 for FSK demodulation task.
WDT_CHECK_IDLE_TASK_CPU1 is not set
WDT_CHECK_IDLE_TASK_CPU1 is not set

- Keep default clock frequency
DEFAULT_CPU_FREQ_MHZ=160

### Build and Flash

* [ESP-IDF Getting Started Guide](https://idf.espressif.com/)
