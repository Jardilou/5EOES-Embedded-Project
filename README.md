# 5EOES-Embedded-Project
A project to assess the course of embedded security describing how to operate attacks to crack a password on an embedded target using a chipwhisperer Nano. The target in our case is an Arduino Uno platform flashed with an unidentified firmware.

## 0.Preliminary Work
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
This means there is a hidden channel to try our password.
<br/>
To detect a hidden channel, we connect a USB to TTL UART Uploader Module CH340G HW-193.
<br/>
![CH340G HW-193](https://github.com/Jardilou/5EOES-Embedded-Project/blob/main/Attack_Identification/CH340G_HW-193.jpg)
<br/>
The next step for us is to remove the atmega328p microcontroller and to cable it like on the following schematic. The values of the resistors is 100 Ohms and the values of the capacitances need to be between 100 and 300 µF.
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
The Arduino Uno board should be disconnected in this configuration since the input voltage will come from the HW-193 module.
<br/>
Here is the message that shows up when connected to the Serial Monitor and the message received when typing a wrong password.  

![Password_Request](https://raw.githubusercontent.com/Jardilou/5EOES-Embedded-Project/main/Attack_Identification/Enter_Password_Request.png)

We have thus found the main entry point !  

## 2 Power Analysis
A power analysis attack on an embedded platform measures tiny variations of power consumption during operations to extract secret data like a password in our case. It exploits physical leakage which will be measured by our chipwhisperer nano in order to analyse the traces.
<br/>
Setup of the chipwhisperer : 
1. Remove the USB-UART connection cable
2. Since the Chipwhisperer Nano is powered in 3.3V and the Arduino Uno is powered in 5V, both will need to be connected to the computer
3. In order to connect the pins of the chipwhisperer to the breadboard, 20-pins headers need to be soldered on the board as well as 3-pins headers on the measure ports. 
![CWNano_up_close](https://github.com/Jardilou/5EOES-Embedded-Project/blob/main/Power_Analysis_Attack/Images/CWNano_Up_Close.png)
<br/>
5. The pins of the chipwhisperer need to be connected as such /(Make sure to verify the pins of the chipwhisperer on the back of the board; the pin 2, 4, 6 etc are the closest to the edge):

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


Here is the ATMEGA pinout in case you need help.
<br/>
![ATMEGA_Pinout](https://github.com/Jardilou/5EOES-Embedded-Project/main/Attack_Identification/ATMEGA_Pinout.jpg)
