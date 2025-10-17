# 5EOES-Embedded-Project
A project to assess the course of embedded security describing how to operate attacks to crack a password on an embedded target. The target in our case is an arduino uno platform flashed with an unidentified firmware.

# 0.Preliminary Work
The very first step is to flash the unidentified firmware (see Preliminary_Work/secure_sketch_v20251015.1.elf) on the arduino uno. 
This is done using the command :
<pre> ```
.\avrdude.exe -v -patmega328p -carduino -P COMXX -b115200 -Uflash:w:secure_sketch_v20251015.1.elf 
``` </pre>
