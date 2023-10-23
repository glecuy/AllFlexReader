| Supported Targets | ESP32 | ESP32-C2 | ESP32-C3 | ESP32-C6 | ESP32-H2 | ESP32-S2 | ESP32-S3 |
| ----------------- | ----- | -------- | -------- | -------- | -------- | -------- | -------- |

# Experimental AllFlex RFID reader.

AllFlex is a trademark


The goal of this project is to read ear tags used for cow identification.

Documentation and experiments show that the ear tag uses an RF frequency of 134.2 kHz for activation and FSK modulation (124.2 + 134.2 kHz) for response. (HDX protocol)

It relies on ISO 11784/5 documents

This project was tested with ear tags only. Only HDX is implemented.


### Hardware

One TX coil: 80 mm in diameter, approximately 80 turns, tuned to 134.2 Khz
One RX coil: 80 mm diameter, approximately 50 turns, set to 129.2 Khz

TX part: a push-pull stage driven by an hc logic gate connected to the devkit esp32 module (output).

RX part: a common emmetter stage followed by an hc logic gates connected to the devkit esp32 module (input).

An ESP devkit module to run software


### Configure the project

idf.py menuconfig

Important: Disable Watch dog reset for CPU1 for FSK demodulation task.


### Build and Flash

* [ESP-IDF Getting Started Guide](https://idf.espressif.com/)


