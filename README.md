================================================================================
  WATER LEVEL SENSOR — ESP32 + AJ-SR04M WATERPROOF ULTRASONIC SENSOR
================================================================================

OVERVIEW
--------
This project uses an ESP32 microcontroller paired with an AJ-SR04M waterproof
ultrasonic distance sensor to measure water levels. The sensor emits ultrasonic
pulses and measures the time taken for the echo to return from the water surface,
calculating the distance and therefore the water level.

The AJ-SR04M is fully waterproof and weatherproof, making it suitable for use
in tanks, wells, reservoirs, or any outdoor water monitoring application.


--------------------------------------------------------------------------------
COMPONENTS
----------
  - ESP32 development board (e.g. ESP32-DevKitC or similar)
  - AJ-SR04M waterproof ultrasonic sensor (4-wire cable)
  - 1x 1kΩ resistor
  - 1x 2kΩ resistor
  - Breadboard and jumper wires
  - USB cable for programming and power


--------------------------------------------------------------------------------
WIRING
------

  AJ-SR04M Pin    |  ESP32 Pin       |  Notes
  ----------------|------------------|----------------------------------------
  VCC (Red)       |  VIN or 5V       |  Use 5V for best range performance
  GND (Black)     |  GND             |  Common ground
  TRIG (Yellow)   |  GPIO 5          |  Direct connection, no resistor needed
  ECHO (White)    |  GPIO 18 *       |  Via voltage divider (see below)

  * IMPORTANT: The ECHO pin outputs 5V logic. The ESP32 GPIO pins are only
    3.3V tolerant. A voltage divider MUST be used to protect the ESP32.


VOLTAGE DIVIDER (for ECHO pin)
-------------------------------
The voltage divider scales the 5V ECHO signal down to ~3.3V safe for the ESP32.

  Wiring order on breadboard:

    ECHO (5V) ──── 1kΩ ──── [midpoint row] ──── GPIO 18
                                  |
                                 2kΩ
                                  |
                                 GND

  Breadboard layout:
    - One end of the 1kΩ resistor connects to the ECHO wire from the sensor
    - The other end of the 1kΩ resistor goes to a shared midpoint row
    - One end of the 2kΩ resistor connects to that same midpoint row
    - The other end of the 2kΩ resistor connects to GND
    - A wire from the midpoint row goes to GPIO 18 on the ESP32

  Calculation:
    Vout = Vin x (R2 / R1 + R2)
    Vout = 5V x (2000 / 3000) = 3.33V  ✓ Safe for ESP32


  ⚠ Common mistake — do NOT wire it this way:
    ECHO ──── 2kΩ ──── [midpoint] ──── GPIO 18
                            |
                           1kΩ
                            |
                           GND
  This gives only 1.67V which is too low for the ESP32 to read as HIGH.


--------------------------------------------------------------------------------
SENSOR SPECIFICATIONS (AJ-SR04M)
---------------------------------
  Operating Voltage    :  3.3V – 5V DC
  Operating Current    :  ~8 mA
  Measuring Range      :  20 cm – 450 cm
  Accuracy             :  ±1 cm
  Trigger Pulse Width  :  10 µs
  Interface            :  TRIG / ECHO (standard HC-SR04 compatible)
  Waterproofing        :  IP67 rated transducer head
  Cable Length         :  ~2.5 m (varies by supplier)


--------------------------------------------------------------------------------
ARDUINO / PLATFORMIO CODE
--------------------------

  File: src/main.cpp

  -----------------------------------------------------------------------
  #include <Arduino.h>

  #define TRIG_PIN  5
  #define ECHO_PIN  18

  void setup() {
    Serial.begin(115200);
    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);
    Serial.println("AJ-SR04M Ready");
  }

  float getDistanceCm() {
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);

    long duration = pulseIn(ECHO_PIN, HIGH, 60000);

    if (duration == 0) {
      return -1;  // No echo received
    }

    return (duration * 0.0343) / 2.0;
  }

  void loop() {
    float distance = getDistanceCm();

    if (distance < 0) {
      Serial.println("Out of range or no echo");
    } else {
      Serial.print("Distance: ");
      Serial.print(distance, 1);
      Serial.println(" cm");
    }

    delay(200);
  }
  -----------------------------------------------------------------------

  platformio.ini:

  -----------------------------------------------------------------------
  [env:esp32dev]
  platform = espressif32
  board = esp32dev
  framework = arduino
  monitor_speed = 115200
  -----------------------------------------------------------------------

  Note: The #include <Arduino.h> line is required in PlatformIO.
  Arduino IDE adds this automatically, but PlatformIO does not.


--------------------------------------------------------------------------------
HOW IT WORKS
------------
1. The ESP32 sends a 10 microsecond HIGH pulse on the TRIG pin.
2. The AJ-SR04M emits a burst of 8 ultrasonic pulses at 40 kHz.
3. The pulses travel through air, hit the water surface, and reflect back.
4. The sensor raises the ECHO pin HIGH for the duration of the round trip.
5. The ESP32 measures that duration using pulseIn().
6. Distance is calculated: distance (cm) = duration (µs) x 0.0343 / 2

   The division by 2 accounts for the pulse travelling to the target AND back.
   0.0343 cm/µs is the speed of sound at approximately 20°C.


--------------------------------------------------------------------------------
TROUBLESHOOTING
---------------

  Problem: "Out of range or no echo" printed constantly
  Solutions:
    1. Check VCC is connected to 5V (VIN), not 3.3V — use 5V for full range
    2. Verify voltage divider resistors are the correct way round (1kΩ on ECHO
       side, 2kΩ on GND side — not reversed)
    3. Confirm TRIG and ECHO wires are not swapped
    4. Point the sensor at a flat solid surface 30–100 cm away for testing
    5. Increase pulseIn timeout: pulseIn(ECHO_PIN, HIGH, 60000)

  Problem: Unstable or jumping readings
  Solutions:
    1. Add averaging over multiple readings
    2. Ensure solid GND connection between ESP32 and sensor
    3. Keep sensor cable away from power wires to reduce interference

  Problem: Readings always too short / too long
  Solutions:
    1. Check the 0.0343 constant — adjust for temperature if needed
       (speed of sound = 331.3 + 0.606 x Temp°C) cm/ms


--------------------------------------------------------------------------------
NOTES
-----
  - The sensor requires a minimum distance of 20 cm to function correctly.
    Objects closer than 20 cm will produce unreliable or no readings.
  - For water tank level monitoring, mount the sensor at the top of the tank
    pointing downward. Subtract the reading from the total tank height to get
    the water depth.
  - The waterproof rating applies to the sensor head only. Keep the PCB and
    cable connector dry.
  - For Home Assistant integration, consider flashing the ESP32 with ESPHome
    instead of custom Arduino code.


================================================================================
  END OF DOCUMENT
================================================================================
