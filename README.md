# 5EOES-Embedded-Project

**The goal of this project is to assess the course of Embedded Security describing how to operate attacks to crack a password on an embedded target using a chipwhisperer Nano. The target in our case is an Arduino Uno platform flashed with an unidentified firmware.**

---

## Table of Contents

* [Preliminary Work](#0-preliminary-work)
* [Attack Identification](#1-attack-identification)
* [Power Analysis](#2-power-analysis)
* [Firmware Analysis](#3-firmware-analysis)
* [Discussion of Countermeasures](#4-discussion-of-countermeasures)
* [Final Attack Tree](#5-final-attack-tree)
* [Vulnerability Assessment Score](#6-vulnerability-assessment-score)
* [Sources](#7-sources)

---

## 0. Preliminary Work

The very first step is to flash the unidentified firmware (see `Preliminary_Work/secure_sketch_v20251015.1.elf`) on the arduino uno.
This is done using the following command:

```sh
.\avrdude.exe -v -patmega328p -carduino -P COMXX -b115200 -Uflash:w:secure_sketch_v20251015.1.elf
```
`COMXX` must be replaced by the right COM port and the command must be executed in the same folder as the firmware. **"Avrdude done. Thank you"** should appear once the flashing is finished.

The second step is to download the chipwhisperer toolchain following this guide:

[https://chipwhisperer.readthedocs.io/en/latest/](https://chipwhisperer.readthedocs.io/en/latest/)

Once the toolchain is setup, before entering the attack identification phase, in order to avoid having the STM32F0 GPIOs driven by the CW-Nano when interacting with our external target, we should set them as high impedance. This is done by:

1. Creating a new folder in our chipwhisperer path named `firmware/mcu/stm32-gpio-tristate`.
2. Putting the `gpio_tristate.c` and `Makefile.txt` (see the `Preliminary_Work` folder) files in this new folder.
3. Running the following code:

```bash
%%bash
cd ../../firmware/mcu/stm32-gpio-tristate
make PLATFORM='CWNANO' CRYPTO_TARGET=NONE -j
```

```python
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
```

Once this is finished, below is what should show up on the Serial Monitor.

---

## 1. Attack Identification

When connecting the Arduino Uno in a classic setup, the following message would show up. <br/>

![Serial Monitor](https://github.com/Jardilou/5EOES-Embedded-Project/blob/main/Attack_Identification/welcome_to_the_vault.png) <br/>

We can therefore conclude the probable presence of a hidden channel, likely UART on the pins of the ATMEGA328p.

To detect a hidden channel, a USB to TTL UART Uploader Module is needed. In this case the CH340G HW-193 was used.

![CH340G HW-193](https://github.com/Jardilou/5EOES-Embedded-Project/blob/main/Attack_Identification/CH340G_HW-193.jpg) <br/>

The next step is to remove the ATMEGA328p microcontroller and cable it in the same way as the following schematic. The value of the resistors is 100 Ohms and the value of the capacitors must be between 100 and 300 µF.

The schematic will be detailed in the Power Analysis section. <br/>
![ATMEGA\_Breadboard](https://github.com/Jardilou/5EOES-Embedded-Project/blob/main/Attack_Identification/ATMEGA_Breadboard_Circuit.png) <br/>

After a series of tries and errors in order to find the secret UART channel, the correct connection between the USB-UART HW-193 module and the ATMEGA on the breadboard was discovered :

* HW-193 5V to VCC Rail
* HW-193 GND to GND Rail
* HW-193 RV to ATMEGA pin 16 (see the `Attack_Identification/ATMEGA_Pinout.png` file)
* HW-193 5V to ATMEGA pin 17

This discovery was made thanks to the proximity of the TX and RX pins. Should these pins be far away from each other, the time needed for the discovery would have been far greater.

The Arduino Uno board should be disconnected in this configuration since the input voltage will come from the HW-193 module.

Up next is the message appearing when serial connection is established as well as the message received when typing a wrong password.

![Password\_Request](https://github.com/Jardilou/5EOES-Embedded-Project/blob/main/Attack_Identification/Enter_Password_Request.png)

We have thus found the main entry point!

---

## 2. Power Analysis

### 2.1 Description

A power analysis attack on an embedded platform measures tiny variations of power consumption during the operations of the platform. The goal to extract secret data, a password in our case. It exploits physical leakage which will be measured by our chipwhisperer nano in order to analyse the traces.

### 2.2 Setup of the chipwhisperer

1. Remove the USB-UART connection cable.
2. Since the Chipwhisperer Nano is powered in 3.3V and the Arduino Uno is powered in 5V, both will need to be connected to the computer.
3. In order to connect the pins of the chipwhisperer to the breadboard, 20-pins headers need to be soldered on the board as well as 3-pins headers on the measure ports.
4. The pins of the chipwhisperer need to be connected as such:

> (Make sure to verify the pins of the chipwhisperer on the back of the board; the pin 2, 4, 6 etc are the closest to the edge)

| CWNano             | ATMEGA/Breadboard      | Arduino Uno |
| ------------------ | ---------------------- | ----------- |
| Pin 2 (GND)        | GND Rail               | GND Pin     |
| Pin 5 (Reset)      | Pin 1 (Reset)          | Reset Pin   |
| Pin 8 (VRef)       | VCC Rail               | 5V Pin      |
| Pin 10 (UART RX)   | Pin 17 (UART TX)       | /           |
| Pin 12 (UART TX)   | Pin 16 (UART RX)       | /           |
| Pin 16 (Trigger input) | Pin 19             | Pin 13      |
| Left Pin MEASURE   | GND Rail               | /           |
| Middle Pin MEASURE | Pin 7 (ATMEGA VCC pin) | /           |

Below is the CWNano up close.

![CWNano\_up\_close](https://github.com/Jardilou/5EOES-Embedded-Project/blob/main/Power_Analysis_Attack/Images/CWNano_Up_Close.png) 
<br/>

Additionnally, below is the ATMEGA pinout.

![ATMEGA\_Pinout](https://github.com/Jardilou/5EOES-Embedded-Project/blob/main/Attack_Identification/ATMEGA_Pinout.jpg) 
<br/>

### 2.3 Power Traces Analysis
The only hint in our possession regarding the password is the first letter, "f". Initially, the approach was to observe is there were any peaks of consumption when the right letter was sent. Unfortunately, this line of inquiry did not lead to conclusive results because of the therefore another approach was considered. This will be detailed hereunder.

In any case, the logical first step is to plot the power trace of "f" and compare it to a wrong input. This is done when executing the following code.
```python
import chipwhisperer as cw
import matplotlib.pyplot as plt
import numpy as np
import time

# Setup of scope and target
scope = cw.scope()
target = cw.target(scope, cw.targets.SimpleSerial2)
target.baud = 9600
scope.default_setup()
scope.adc.samples = 10000

############################################################################################################

def capture(guess):
    print(f"Arm scope for {guess}...")
    scope.arm()
    time.sleep(0.01)
    print(f"Sending '{guess}'...")
    target.ser.write(guess.encode() + b"\n")
    ret = scope.capture()
    if ret:
        print(f"Timeout during capture of '{guess}'")
        return None
    trace = scope.get_last_trace()
    print(f"Trace '{guess}' captured: {len(guess)} samples")
    time.sleep(1)
    return trace


############################################################################################################
trace_a = capture("a")
trace_f = capture("f")
trace_abc = capture("abc")
trace_abcdef = capture("abcdef")

#To be executed after retrieval of password for delay comparison
# trace_f7_dash = capture("f7-")
# trace_f7_dash_at_Jp = capture("f7-@Jp")
# trace_right_pw = capture("f7-@Jp0w")

############################################################################################################

#To be executed after retrieval of password for delay comparison
# if trace_f is not None and trace_a is not None and trace_abcdef is not None and trace_abc is not None and trace_f7_dash is not None and  trace_f7_dash_at_Jp is not None and trace_right_pw is not None:

if trace_f is not None and trace_a is not None and trace_abcdef is not None and trace_abc is not None:
    plt.figure(figsize=(15, 8))

    plt.subplot(7, 1, 1); plt.plot(trace_a, 'r-', linewidth=0.5); plt.title("Power trace for 'a'"); plt.ylabel("Amplitude"); plt.grid(True, alpha=0.3)
    plt.subplot(7, 1, 2); plt.plot(trace_abc, 'r-', linewidth=0.5); plt.title("Power trace for abc"); plt.ylabel("Amplitude"); plt.grid(True, alpha=0.3)
    plt.subplot(7, 1, 3); plt.plot(trace_abcdef, 'r-', linewidth=0.5); plt.title("Power trace for abcdef"); plt.ylabel("Amplitude"); plt.grid(True, alpha=0.3)
    plt.subplot(7, 1, 4); plt.plot(trace_f, 'b-', linewidth=0.5); plt.title("Power trace for 'f'"); plt.ylabel("Amplitude"); plt.grid(True, alpha=0.3)
    # plt.subplot(7, 1, 5); plt.plot(trace_f7_dash, 'b-', linewidth=0.5); plt.title("Power trace for f7-"); plt.ylabel("Amplitude"); plt.grid(True, alpha=0.3)
    # plt.subplot(7, 1, 6); plt.plot(trace_f7_dash_at_Jp, 'b-', linewidth=0.5); plt.title("Power trace for f7-@J"); plt.ylabel("Amplitude"); plt.grid(True, alpha=0.3)
    # plt.subplot(7, 1, 7); plt.plot(trace_right_pw, 'g-', linewidth=0.5); plt.title("Power trace for f7-@Jp0"); plt.ylabel("Amplitude"); plt.grid(True, alpha=0.3)

    plt.tight_layout(); plt.show()


else:
    print("Error : Incomplete capture of traces")
```
The next picture represents the traces of "a", "abc" and "abcdef" compared to the trace of "f".

![First_power_Traces](https://github.com/Jardilou/5EOES-Embedded-Project/blob/main/Power_Analysis_Attack/Images/f_a_abc_abcdef_Delay_Comparison.png
) 

It is clear that the input of a correct character extends the computing time. There is most likely a loop in the source code where each character is checked one after another and the code doesn't leave the loop until it a wrong character is detected in the string. Therefore, greater shift of the peaks of this power trace indicates if the character is correct or not.

This analysis method is therefore not a power analysis per se, but in reality more of a timing analysis on the power traces. The next step is thus to define a method to compute the shift of the trace.

```python
import numpy as np
from scipy.signal import correlate


def estimate_sample_shift(trace, reference):
    """
    Returns the best estimated horizontal shift (in samples)
    needed to align `trace` to `reference`.
    Positive shift => trace is delayed (shifted right).
    """
    corr = correlate(trace, reference, mode='full')
    lag = np.argmax(corr) - (len(reference) - 1)
    return lag

```

The only step left is to automate a password attack. It is the purpose of the code below 


```python
def capture_trace(password_attempt):
    """Capture power trace for a given password attempt"""
    scope.arm()
    time.sleep(0.01)
    
    # Send password attempt
    target.ser.write(password_attempt + b"\n")
    
    ret = scope.capture()
    if ret:
        print(f"  ⚠ Timeout for '{password_attempt}'")
        return None
    
    trace = scope.get_last_trace()
    time.sleep(0.5)  # Small delay between captures
    return trace

guessed_pw = b""                   
for _ in range(0, 10):

    biggest_shift = 0
    biggest_char = b"a"           

    ref_trace = capture_trace(b"a\n")  
    # List of ASCII characters 33 to 127 except for \ as it resulted in errors
    for c in b"!#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~":
        # Computes the traces and offset with the trace of letter "a" for each character
        trace_c = capture_trace(guessed_pw + bytes([c]) )
        shift = estimate_sample_shift(trace_c, ref_trace)
        # If the shift between the trace of c and of "a" is greater than that of any other character, it is added to the password string
        if shift > biggest_shift:
            biggest_shift = shift
            biggest_char = bytes([c])


    
    guessed_pw += biggest_char
    print(guessed_pw)

```

The output of this code is shown on the image here under.


### 2.4 Results of the attack



![Password_retrieval](https://github.com/Jardilou/5EOES-Embedded-Project/blob/main/Power_Analysis_Attack/Images/Password_retrieval_script_output.png)


It is important to note that since the length of the password is unknown, each of these attempts must be tested by connecting the serial connection. For the salt and hash retrieval, see section * [Firmware Analysis](#3-firmware-analysis).

The next figure shows the complete comparison of delays for a variety of guesses. Each time correct characters are added to the password, the shift is greater. To get the complete figure, simply decomment the corresponding line on the code previously discussed.


![Complete_delay_comparison](https://github.com/Jardilou/5EOES-Embedded-Project/blob/main/Power_Analysis_Attack/Images/Complete_Delay_Comparison.png)

---

## 3. Firmware Analysis

The first option in order to retrieve the cyphered text contained on the platform is typing the following command. It extracts the human-readable text from our binary file. `-n 4` means only strings with a length above 4 characters should be retrieved since I assumed the password wouldn't be this short.

```sh
strings -n 4 secure_sketch_v20251015.1.elf > extracted_strings.txt
```

This extracted file contained 705 lines in which lied the password (see `Firmware_Attack/extracted_strings.txt`).

Nevertheless, this next option yields more relevant results.

```sh
avr-objdump -s -j .data secure_sketch_v20251015.1.elf | less
```

`avr-objdump` is a tool that lets you inspect the internal contents of an AVR firmware ELF file. The option `-s` tells it to dump the raw bytes.
The option `-j XXX` restricts the dump to a specific memory section `XXX`. I tried the `.rodata` as well as the `.text` sections but unfortunately they yielded no results. The `.data` section contains global or static variables that have an initial value and will be copied into volatile SRAM at runtime. It is a key element of most embedded firmwares.
The `| less` at the end allows scrolling through the output.
The output I got from this command is detailed here below.


![Im\_in](https://github.com/Jardilou/5EOES-Embedded-Project/blob/main/Firmware_Attack/.data_section_content.png) <br/>

Among this file was a succession of random characters closely resembling to a password. Hmm, I wonder if this could be it...

I reconnected my serial connection using the HW-193. What a surprise: a salt and hash appeared just before my eyes. Moreover, the previous image showing the content of the `.data` section exposed the very well kept secret **"Je suis une petite tortue"**, confirming indeed that my professor is, in fact, a small turtle.
This attack is thus more damaging than the aforementioned Power Trace Analysis Attack since it exposed not only the password to the vault but the secrets as well. This secret was used only to generate random salt and hashes but in concept, this attack is able to retrieve any sensitive information written in plaintext in the file.

![Im\_in2](https://github.com/Jardilou/5EOES-Embedded-Project/blob/main/Firmware_Attack/salt_and_hash.png) <br/>

## 4. Discussion of countermeasures

### 4.1 Password stored in hash and salt

The password should therefore not be stored in plaintext inside the `.ino` file. The method to implement this countermeasure is to locally compute the hash of the password (Note : it remains unchanged  between the initial version and the version with countermeasures implementation). Once it is computed, the hash and salt are hard-coded as byte arrays.   

### 4.2 Constant-timing password comparison

In order to counter the power trace timing analysis attack, it is needed to implement constant-time password comparison of the password sent by UART with the hash and salt of the password stored in the file mentioned here above. 

### 4.3 Implementation

The way to implement these countermeasures is the following :

```cpp

const uint8_t SAVED_SALT[] PROGMEM = {
  0x12, 0x34, 0x56, 0x78
};

//Hash of my password
const uint8_t SAVED_HASH[] PROGMEM = {
  0x81, 0xB4, 0x4C, 0xAE, 0x27, 0x0C, 0xB3, 0x31,
  0x27, 0xAD, 0xD7, 0x8C, 0x8C, 0x5A, 0xA7, 0xF5,
  0x95, 0x26, 0xF5, 0xE4, 0xBA, 0xF2, 0x8B, 0x20,
  0x5D, 0x1D, 0x9B, 0x0F, 0xA7, 0xE9, 0x40, 0x35
};

```

The hash is a 32-byte value and the salt is a 4-byte value. They are stored on the program memory. On Arduino platforms, PROGMEM tells the compiler to keep the bytes in flash instead of RAM.

```cpp

// Compute SHA3-256(salt || passwordAttempt) into outDigest
void hashAttemptWithSalt(const char *attempt, const uint8_t *salt, size_t saltLen, uint8_t *outDigest) {
  SHA3_256 sha3;
  sha3.reset();
  sha3.update(salt, saltLen);
  sha3.update((const uint8_t *)attempt, strlen(attempt));
  sha3.finalize(outDigest, 32);
}
```

This function computes concatenated salt and attempt and writes the 32-byte result into outDigest. This result will be compared with the salt and hash stored in the cell above.

```cpp
// Constant-time compare: returns true iff a == b (both len bytes)
bool consttime_eq(const uint8_t *a, const uint8_t *b, size_t len) {
  uint8_t diff = 0;
  for (size_t i = 0; i < len; ++i) {
    diff |= a[i] ^ b[i];
  }
  // diff == 0 => equal
  // We return (diff == 0) without any branching that depends on contents
  return diff == 0;
}
```

This function compares two byte arrays to check for equality but the time required for computation has to remain constant, regardless of how many bytes match. This means that no early exit is implemented to get out of the loop. 

The operation `a[i]^b[i]` is a `XOR` operation : each byte of a is compared with the corresponding byte of b. If the bytes are equal, XOR gives 0. If they are different, `XOR` gives some non-zero value. 

`diff` will store whether there is any difference between the two arrays. It is initialized to 0 meaning "no difference yet". 
`diff |= ...` is a `bitwise OR` operation :
If `diff` equals 0 and bytes differ, `diff` becomes non-zero. If `diff` is already non-zero, it stays non-zero.
By the end of the loop, `diff` equals 0 if and only if all bytes matched. In opposition to a `return false`, this implementation ensures that the execution time doesn't depend on the first mismatch, thus rendering the timing analysis impossible.

```cpp
bool check_password_attempt(const char *attempt) {
  // load salt from PROGMEM
  uint8_t salt[4];
  for (size_t i = 0; i < sizeof(salt); ++i) salt[i] = pgm_read_byte_near(SAVED_SALT + i);

  // compute hash of salt || attempt
  hashAttemptWithSalt(attempt, salt, sizeof(salt), digest);

  // load stored hash from PROGMEM
  uint8_t stored[32];
  for (size_t i = 0; i < sizeof(stored); ++i) stored[i] = pgm_read_byte_near(SAVED_HASH + i);

  // constant-time compare
  return consttime_eq(digest, stored, 32);
}

```
This function :
1. Reads the salt from flash into a RAM buffer salt[4] using pgm_read_byte_near (this function comes from the AVR API). Each of the 4 bytes are copied from SAVED_SALT into RAM.
2. Computes hash: call the aforementioned hashAttemptWithSalt(attempt, salt, sizeof(salt), digest) function to compute the digest SHA3-256(salt || attempt).
3. Reads previously stored hash from flash into stored[32].
4. Compares computed digest with stored using the constant-time consttime_eq. Return true if equal (password correct), else false.
It will be called in the `void loop()` function in order to check the input password.

The complete code can be found in the folder `Countermeasures/Countermeasures.ino`, as well as the `.elf` file.

### 4.4 Results

The next figure shows the content of the `.data` secion retrieved using the method previously mentioned.

![data_cm](https://github.com/Jardilou/5EOES-Embedded-Project/blob/main/Countermeasures/images/.data_section_countermeasures.png)

Although the secret text is still present (it is necessary to generate random hash and salt), the plaintext password disappeared completely from the section. The method using strings yielded the same results (see `Countermeasures/extracted_strings_after_countermeasures.txt`).

Regarding the power traces, the constant-time implementation makes it obsolete since no shift can be observed. Therefore, the passwords retrieved will be random.

![power_traces_cm](https://github.com/Jardilou/5EOES-Embedded-Project/blob/main/Countermeasures/images/power_traces_offset_countermeasures.png
)

![passwd_guess_cm](https://github.com/Jardilou/5EOES-Embedded-Project/blob/main/Countermeasures/images/password_guess_countermeasures.png)


## 5. Final Attack Tree

Here are the vulnerabilities identified:

* Access to the .hex firmware flashed on the platform containing unencrypted data
* Access to a hidden UART channel as well as a trigger set high during communication. This UART channel gave information when the wrong password is input.

Exploits set up:

* Firmware analysis
* Side-channel power traces analysis

![Attack\_tree](https://github.com/Jardilou/5EOES-Embedded-Project/blob/main/Additional_Content/ES_Project_Attack_Tree.png)

---

## 6. Vulnerability Assessment Score

### 6.1 Power trace side-channel analysis

This table captures CVSS 4.0 metrics for threat modeling purposes.

#### 1. Base Metrics

| Metric                       | Value    | Justification                                                                          |
| ---------------------------- | -------- | -------------------------------------------------------------------------------------- |
| **AV — Attack Vector**       | Physical | Attack needs physical access to the embedded platform                                  |
| **AC — Attack Complexity**   | Low      | Sending a simple binary password yields a response. Once setup, pretty straightforward |
| **AT — Attack Requirements** | None     | No victim-controlled prerequisites needed                                              |
| **PR — Privileges Required** | None     | No authentication or access needed before exploit                                      |
| **UI — User Interaction**    | None     | No interaction from the victim for the attack to succeed                               |

**Vulnerable System Impact Metrics**

| Metric                   | Value | Justification                         |
| ------------------------ | ----- | ------------------------------------- |
| **VC — Confidentiality** | Low   | Only the hash and salt is retrieved   |
| **VI — Integrity**       | None  | No data or code modification possible |
| **VA — Availability**    | None  | No system service disruption possible |

Subsequent System Impact Metrics : No subsequent systems

---

#### 2. Supplemental Metrics

| Metric                                 | Value       | Justification                                                                     |
| -------------------------------------- | ----------- | --------------------------------------------------------------------------------- |
| **S — Safety**                         | Negligible  | No real potential physical injury or safety impact                                |
| **AU — Automatable**                   | Yes         | Exploit can be automated at scale                                                 |
| **R — Recovery**                       | User        | The system requires the user to flash a new firmware in case of successful attack |
| **V — Value Density**                  | Diffuse     | A single platform is hacked in comparison to a central system for example         |
| **RE — Vulnerability Response Effort** | High        | Platforms need to be recalled and reflashed                                       |
| **U — Provider Urgency**               | Not defined | Note priority for vendor to remediate                                             |

3. Environmental Metrics - Security Requirements : no additional environment

---

#### 4. Threat Metrics (Temporal)

| Metric                   | Value | Justification                                  |
| ------------------------ | ----- | ---------------------------------------------- |
| **E — Exploit Maturity** | POC   | The exploit works but is not deployed at scale |

All these parameters sum up to a CVSS score of **0.3**, which is very low.

---

### 6.2 Firmware analysis

Since the target as well as its secrets remains the same, most of these parameters will remain the same.

#### 1. Base Metrics

| Metric                       | Value    | Justification                                                                  |
| ---------------------------- | -------- | ------------------------------------------------------------------------------ |
| **AV — Attack Vector**       | Physical | Attacker exploits the vulnerability by acdessing target through terminal       |
| **AC — Attack Complexity**   | Low      | Once environment setup with the tools and firmware, 1 line of code is required |
| **AT — Attack Requirements** | Present  | Victim-controlled prerequisites needed : firmware needs to be available        |
| **PR — Privileges Required** | None     | No authentication or access needed before exploit                              |
| **UI — User Interaction**    | None     | No interaction from the victim for the attack to succeed                       |

**Vulnerable System Impact Metrics**

| Metric                   | Value | Justification                                                                 |
| ------------------------ | ----- | ----------------------------------------------------------------------------- |
| **VC — Confidentiality** | High  | Hash and salt are retrieved as well as the secret "Je suis une petite tortue" |
| **VI — Integrity**       | None  | No data or code modification possible                                         |
| **VA — Availability**    | None  | No system service disruption possible                                         |

Subsequent System Impact Metrics : No subsequent systems

---

#### 2. Supplemental Metrics

| Metric                                 | Value       | Justification                                                                     |
| -------------------------------------- | ----------- | --------------------------------------------------------------------------------- |
| **S — Safety**                         | Negligible  | No real potential physical injury or safety impact                                |
| **AU — Automatable**                   | Yes         | Exploit can be automated at scale                                                 |
| **R — Recovery**                       | User        | The system requires the user to flash a new firmware in case of successful attack |
| **V — Value Density**                  | Diffuse     | A single platform is hacked in comparison to a central system for example         |
| **RE — Vulnerability Response Effort** | High        | Platforms need to be recalled and reflashed                                       |
| **U — Provider Urgency**               | Not defined | Note priority for vendor to remediate                                             |

3. Environmental Metrics - Security Requirements : no additional environment

---

#### 4. Threat Metrics (Temporal)

| Metric                   | Value | Justification                                  |
| ------------------------ | ----- | ---------------------------------------------- |
| **E — Exploit Maturity** | POC   | The exploit works but is not deployed at scale |

All these metrics sum to a CVSS score of **5**, which is medium.

---

## 7. Sources
- NewAE Technology Inc., “CW1101 ChipWhisperer-Nano,” ChipWhisperer Documentation, 2015-2025. [Online]. Available: https://rtfm.newae.com/Capture/ChipWhisperer-Nano/
- E. Odunlade, “Perform Power Analysis Side-Channel Attacks with the ChipWhisperer-Nano,” Electronics-Lab.com, 18 Nov. 2019. [Online]. Available: https://www.electronics-lab.com/perform-power-analysis-side-channel-attacks-chipwhisperer-nano/
- NewAE Technology Inc., “20-Pin Connector — ChipWhisperer Documentation,” ChipWhisperer ReadTheDocs, Copyright 2023–2025. [Online]. Available: https://chipwhisperer.readthedocs.io/en/v6.0.0b/Capture/20-pin-connector.html#id15
- National Vulnerability Database, “CVSS v4.0 Calculator,” NVD – Vulnerability Metrics, U.S. NIST. [Online]. Available: https://nvd.nist.gov/vuln-metrics/cvss/v4-calculator

