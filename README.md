# Eye for Phelma Float – Persistence of Vision (POV) Display

## Introduction
This repository contains the IoT and Human-Machine Interface (HMI) codebase for an academic group project conducted at **Grenoble INP - Phelma**. 

As part of the **Ol’INP 2027** competition gathering all engineering schools from Grenoble INP, our 7-person team is building electronic "eyes" for the future competition float utilizing Persistence of Vision (POV) display technology. 

**My Role:** I was in charge of building the IoT architecture and the centralized HMI to activate all actuators on the float and control the POV display.

## Embedded System Architecture
The main goal of this project is to prototype an IoT architecture and a centralized HMI capable of interacting in real-time with the float's operators and users. 

To meet our flexibility and robustness requirements, the system is designed around four core functions:
*   **Centralized Actuator Control:** Driving all hardware peripherals (e.g., horn relays, high-power LED strips) via local physical inputs (joystick, push-buttons).
*   **Local Mass Storage:** Buffering and ensuring the persistence of visual configuration files on an SD card prior to display processing.
*   **Embedded Data Processing:** Dynamically formatting and adapting image matrices to the dimensional and timing constraints of the POV system.
*   **Wireless Remote Control:** A remote user interface (via smartphone) enabling wireless OTA (Over-The-Air) uploading, updating, and selection of images. *(Currently under development).*

<img width="811" height="412" alt="global_schematic" src="https://github.com/user-attachments/assets/4e6a242c-0b79-40b4-aedb-4b683bf74bf2" />

## Hardware Selection & Local Interface

The local interface allows users to browse a list of files stored on the SD card via an OLED screen and a joystick, selecting the target file to send to the STM32 for rendering. Independent push-buttons handle relay switching for the float's accessories.

### Why ESP32?
I chose the **ESP32** over alternatives like the Raspberry Pi for several critical technical reasons:
1.  **Power Consumption:** Operating outdoors on battery power, energy is scarce (the POV display itself draws significant current). The ESP32 consumes only 50-240mA, whereas a Raspberry Pi can draw up to 3A and requires active cooling.
2.  **Simplified Interfacing:** The ESP32 features native analog inputs (ADCs) essential for our joystick. A Raspberry Pi would have required external ADC components.
3.  **Cost-Effectiveness:** At roughly €2, the ESP32 fits our budget perfectly compared to a €50 Raspberry Pi, ensuring easy reproducibility.

*Note: While the ESP32 has graphical and processing limits compared to a heavier SBC, it is strictly the best option for our specific specifications.*

### Display: OLED over LCD
For outdoor operation, OLED technology offers superior brightness and readability without relying on a power-hungry backlight. It is easily driven via the ESP32’s I2C bus.

## Communication Protocol (ESP32 ➔ STM32)

To ensure the STM32 receives the correct data without dropped packets (which would cause severe visual desynchronization), we implemented a highly robust transfer protocol.

### 1. Image Pre-Processing (Python)
Before transmission, images are processed via a custom Python script:
*   **RGBA ➔ RGB:** Removing the alpha channel; transparent pixels are turned off (R=G=B=0).
*   **Square Cropping:** Ensuring perfect symmetry for circular mapping.
*   **Polar Coordinate Mapping:** Converting standard coordinates to angular positions (degrees).
*   **POV Resolution Scaling:** Adapting the matrix to the physical LED count (width) and angular resolution per revolution (height).
*   **Serialization:** Flattening the data into a `.bin` file for the ESP32.

<img width="1122" height="427" alt="serialization_process" src="https://github.com/user-attachments/assets/79ea3b0b-e721-4fda-8a19-7882b3e3ddae" />

### 2. RS-485 Data Transfer
We utilize an **RS-485 transceiver** for reliable, long-distance differential signaling, ensuring high noise immunity in the float's electrically noisy environment.

**Custom Frame Format:**
*   `SOF` (Start of Frame): `0xAA`
*   `CMD`: `0x01` (Start), `0x02` (Data), `0x03` (End), `0x04` (ACK)
*   `SEQ`: Frame sequence number
*   `LEN`: Payload length
*   `Payload`: Serialized image data
*   `CRC`: Cyclic Redundancy Check for data integrity

<img width="811" height="147" alt="frame" src="https://github.com/user-attachments/assets/8d49126f-e95a-414d-ba87-f318a1b82177" />

To guarantee transmission robustness, the STM32 recalculates the CRC on reception. We use a **closed-loop exchange with Acknowledgments (ACKs)** for every frame sent.

<img width="253" height="379" alt="frame_exchange" src="https://github.com/user-attachments/assets/457fba60-e1a6-4ecd-9eea-3ecc9ca2e60b" />

### 3. Pipeline Validation: Python POV Simulator
To validate the entire data pipeline prior to physical hardware deployment, the final transmitted frames are reconstructed and visualized using a custom Python-based POV simulator. This digital twin approach allows us to verify data integrity, test rotational speeds, and ensure pixel-perfect synchronization without risking hardware damage.

<img width="528" height="517" alt="POV_simulation" src="https://github.com/user-attachments/assets/4b314e88-52da-467b-adc4-21d4d6a5a523" />



