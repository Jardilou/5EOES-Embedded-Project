# 5EOES-Embedded-Project
A project to assess the course of embedded security describing how to operate attacks to crack a password on an embedded target. The target in our case is an arduino uno platform flashed with an unidentified firmware.

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
![Serial Monitor](https://github.com/Jardilou/5EOES-Embedded-Project/blob/main/Attack_Identification/welcome_to_the_vault.png)
This means there is a hidden channel to try our password.

To detect a hidden channel, we connect a USB to TTL UART Uploader Module CH340G HW-193
![CH340G HW-193](https://github.com/Jardilou/5EOES-Embedded-Project/blob/main/Attack_Identification/CH340G_HW-193.jpg)
The next step for us is to remove the atmega328p microcontroller and to cable it like on the following schematic. 


