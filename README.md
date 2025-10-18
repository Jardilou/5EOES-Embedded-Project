# 5EOES-Embedded-Project
The goal of this project is to assess the course of Embedded Security describing how to operate attacks to crack a password on an embedded target using a chipwhisperer Nano. The target in our case is an Arduino Uno platform flashed with an unidentified firmware.

## 0. Preliminary Work
The very first step is to flash the unidentified firmware (see Preliminary_Work/secure_sketch_v20251015.1.elf) on the arduino uno. 
This is done using the following command :
<pre>
.\avrdude.exe -v -patmega328p -carduino -P COMXX -b115200 -Uflash:w:secure_sketch_v20251015.1.elf 
</pre>
using the right COM port as COMXX and entering the command in the same folder as the firmware. You should see "Avrdude done. Thank you" once the flashing is finished.

Before entering the attack identification phase, in order to avoid having the STM32F0 GPIOs driven by the CW-Nano when interacting with our external target, we should set them as high impedance. This is done by :
1. Creating a new folder in our chipwhisperer path named firmware/mcu/stm32-gpio-tristate.
2. Putting the gpio_tristate.c and Makefile.txt (see the Preliminary_Work folder) files in this new folder.
3. Running the following code :
   
<pre>
%%bash
cd ../../firmware/mcu/stm32-gpio-tristate
make PLATFORM='CWNANO' CRYPTO_TARGET=NONE -j
</pre>

<pre>
import chipwhisperer as cw
try:
    if not scope.connectStatus:
        scope.con()
except NameError:
    scope = cw.scope()
try:
    target = cw.target(scope)
except IOError:
    print("INFO: Caught exception on reconnecting to target - attempting to reconnect to scope first.")
    print("INFO: This is a work-around when USB has died without Python knowing. Ignore errors above this line.")
    scope = cw.scope()
    target = cw.target(scope)
cw.program_target(scope, cw.programmers.STM32FProgrammer, "../../firmware/mcu/stm32-gpio-tristate/gpio-tristate-{}.hex".format(PLATFORM))
</pre>

Once this is done, let us inspect what shows up on the Serial Monitor.

## 1. Attack Identification
When connecting the Arduino Uno in a classic setup, the following message would show up.
<br/>
![Serial Monitor](https://github.com/Jardilou/5EOES-Embedded-Project/blob/main/Attack_Identification/welcome_to_the_vault.png)
<br/>
We conclude that there is a hidden channel, probably UART On the pins of the ATMEGA328p.

<br/>
To detect a hidden channel, we connect a USB to TTL UART Uploader Module CH340G HW-193.

![CH340G HW-193](https://github.com/Jardilou/5EOES-Embedded-Project/blob/main/Attack_Identification/CH340G_HW-193.jpg)
<br/>

The next step for us is to remove the ATMEGA328p microcontroller and to cable it like on the following schematic. The values of the resistors is 100 Ohms and the values of the capacitances need to be between 100 and 300 µF.
<br/>

The schematic will be detailed in the Power Analysis section.
<br/>
![ATMEGA_Breadboard](https://github.com/Jardilou/5EOES-Embedded-Project/blob/main/Attack_Identification/ATMEGA_Breadboard_Circuit.png)
<br/>
After a series of tries and errors in order to find the secret UART channel, we discovered that the USB-UART HW-193 module should be connected as such :
<br/>
- HW-193 5V to VCC Rail
- HW-193 GND to GND Rail
- HW-193 RV to ATMEGA pin 16 (see the Attack_Identification/ATMEGA_Pinout.png file)
- HW-193 5V to ATMEGA pin 17
<br/>
This discovery was made thanks to the proximity of the TX and RX pins. Should these pins be far away from each other, the time needed for the discovery would have been greater.
<br/>

The Arduino Uno board should be disconnected in this configuration since the input voltage will come from the HW-193 module.
<br/>

Here is the message that shows up when connected to the Serial Monitor and the message received when typing a wrong password.  

![Password_Request](https://raw.githubusercontent.com/Jardilou/5EOES-Embedded-Project/main/Attack_Identification/Enter_Password_Request.png)

We have thus found the main entry point !  

## 2. Power Analysis
2.1 Description
<br/>

A power analysis attack on an embedded platform measures tiny variations of power consumption during operations to extract secret data like a password in our case. It exploits physical leakage which will be measured by our chipwhisperer nano in order to analyse the traces.
<br/>

2.2 Setup of the chipwhisperer : 
<br/>

1. Remove the USB-UART connection cable
2. Since the Chipwhisperer Nano is powered in 3.3V and the Arduino Uno is powered in 5V, both will need to be connected to the computer
3. In order to connect the pins of the chipwhisperer to the breadboard, 20-pins headers need to be soldered on the board as well as 3-pins headers on the measure ports. 
4. The pins of the chipwhisperer need to be connected as such :
<br/>
(Make sure to verify the pins of the chipwhisperer on the back of the board; the pin 2, 4, 6 etc are the closest to the edge)

| CWNano        | ATMEGA/Breadboard | Arduino Uno |
|---------------|-----------------|-------------|
| Pin 2 (GND)   | GND Rail         | GND Pin     |
| Pin 5 (Reset) | Pin 1 (Reset)    | Reset Pin   |
| Pin 8 (VRef)  | VCC Rail         | 5V Pin      |
| Pin 10 (UART RX) | Pin 17 (UART TX) | /         |
| Pin 12 (UART TX) | Pin 16 (UART RX) | /         |
| Pin 16 (UART TX) | Pin 19 (UART RX) | Pin 13   |
| Left Pin MEASURE | GND Rail         | /         |
| Middle Pin MEASURE | Pin 7(ATMEGA VCC pin) | /         |
<br/>

Here is the CWNano up close.
![CWNano_up_close](https://github.com/Jardilou/5EOES-Embedded-Project/blob/main/Power_Analysis_Attack/Images/CWNano_Up_Close.png)
<br/>
Here is the ATMEGA pinout in case you need help.

![ATMEGA_Pinout](https://github.com/Jardilou/5EOES-Embedded-Project/blob/main/Attack_Identification/ATMEGA_Pinout.jpg)
<br/>

2.3 Power Traces Analysis
<br/>

2.4 Results of the attack
<br/>

2.5 Discussion of countermeasures
<br/>

## 3. Firmware Analysis
3.1 Retrieval of the strings present in the firmware
<br/>
At first, I tried to retrieve the passwords using the following command. It extracts the human-readable text from our binary file. "-n 4" means all strings with a length above 4 characters since I assumed the password wouldn't be this short.
<pre>
strings -n 4 secure_sketch_v20251015.1.elf > extracted_strings.txt
</pre>
This extracted file contained 705 lines in which lied the password (see Firmware_Attack/extracted_strings.txt). But before trying to test them all or write a script to brute-force the complete file, I tried this following command.
<pre>
avr-objdump -s -j .data secure_sketch_v20251015.1.elf | less
</pre>
avr-objdump is a tool that lets you inspect the internal contents of an AVR firmware ELF file. The option -s tells it to dump the raw bytes.
The option -j XXX restricts the dump to a specific memory section XXX. I tried the .rodata as well as the .text sections but unfortunately they yielded no results. The .data section contains global/static variables that have an initial value and will be copied into SRAM at runtime.
It is a key element of most embedded firmwares.
The | less at the end just allows scrolling conveniently through the output.
So each command is inspecting a different memory region of the same firmware.
<br/>
Here is the output I got from this command.


3.2 Analysis of the output
![Im_in](https://github.com/Jardilou/5EOES-Embedded-Project/blob/main/Firmware_Attack/.data_section_content.png)
<br/>
Among this file was a succession of random characters closely resembling to a password. Hmm, I wonder if this could be it. It certainly couldn't be that easy, after all the work I did trying to debug the power analysis...

<br/>
I reconnected my serial connection using the HW-193 and surprise surprise, a salt and hash appeared just before my eyes. 

<br/>
![Im_in2](https://github.com/Jardilou/5EOES-Embedded-Project/blob/main/Firmware_Attack/salt_and_hash.png)

3.3 Countermeasures to implement
<br/>

## 4. Final Attack Tree
Justification of the attacks regarding the vulnerabilities
## 5. Vulnerability Assessment Score
