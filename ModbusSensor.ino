#include <SimpleModbusSlave.h>

#define  ledPin  13 // onboard led 
#define  sensorPin  3 // RCWL-0516 Sensor

#define slaveID 2 // ID des Modbus-Slaves festlegen
#define TxEnablePin 2 // TxEnablePin festlegen. Damit wird die Kommunikationsrichtung am MAX485 gesteuert.

/* This example code has 9 holding registers. 6 analogue inputs, 1 button, 1 digital output
   and 1 register to indicate errors encountered since started.
   Function 5 (write single coil) is not implemented so I'm using a whole register
   and function 16 to set the onboard Led on the Atmega328P.

   The modbus_update() method updates the holdingRegs register array and checks communication.

   Note:
   The Arduino serial ring buffer is 128 bytes or 64 registers.
   Most of the time you will connect the arduino to a master via serial
   using a MAX485 or similar.

   In a function 3 request the master will attempt to read from your
   slave and since 5 bytes is already used for ID, FUNCTION, NO OF BYTES
   and two BYTES CRC the master can only request 122 bytes or 61 registers.

   In a function 16 request the master will attempt to write to your
   slave and since a 9 bytes is already used for ID, FUNCTION, ADDRESS,
   NO OF REGISTERS, NO OF BYTES and two BYTES CRC the master can only write
   118 bytes or 59 registers.

   Using the FTDI USB to Serial converter the maximum bytes you can send is limited
   to its internal buffer which is 60 bytes or 30 unsigned int registers.

   Thus:

   In a function 3 request the master will attempt to read from your
   slave and since 5 bytes is already used for ID, FUNCTION, NO OF BYTES
   and two BYTES CRC the master can only request 54 bytes or 27 registers.

   In a function 16 request the master will attempt to write to your
   slave and since a 9 bytes is already used for ID, FUNCTION, ADDRESS,
   NO OF REGISTERS, NO OF BYTES and two BYTES CRC the master can only write
   50 bytes or 25 registers.

   Since it is assumed that you will mostly use the Arduino to connect to a
   master without using a USB to Serial converter the internal buffer is set
   the same as the Arduino Serial ring buffer which is 128 bytes.
*/


// Using the enum instruction allows for an easy method for adding and
// removing registers. Doing it this way saves you #defining the size
// of your slaves register array each time you want to add more registers
// and at a glimpse informs you of your slaves register layout.

//////////////// registers of your slave ///////////////////
enum {
    // just add or remove registers and you're good to go...
    // The first register starts at address 0
    ADC0,
    ADC1,
    ADC2,
    ADC3,
    ADC4,
    ADC5,
    SENSOR_STATE,
    SINCE_MOVE,
    LED_STATE,
    BUTTON_STATE,
    TOTAL_ERRORS,
    // leave this one
    TOTAL_REGS_SIZE
    // total number of registers for function 3 and 16 share the same register array
};

unsigned int holdingRegs[TOTAL_REGS_SIZE]; // function 3 and 16 register array
unsigned long prevMillis;   // Zeitpunkt der letzten Bewegung
unsigned int timeSinceMotion;     // Vergangene Zeit (Sekunden) seit letzter Bewegung
bool prevstate;                 // Variable zur Erkennung des Statuswechsels
////////////////////////////////////////////////////////////

void setup()
{
    /* parameters(long baudrate,
                  unsigned char ID,
                  unsigned char transmit enable pin,
                  unsigned int holding registers size,
                  unsigned char low latency)

       The transmit enable pin is used in half duplex communication to activate a MAX485 or similar
       to deactivate this mode use any value < 2 because 0 & 1 is reserved for Rx & Tx.
       Low latency delays makes the implementation non-standard
       but practically it works with all major modbus master implementations.
    */

    modbus_configure(19200, slaveID, TxEnablePin, TOTAL_REGS_SIZE, 0);
    pinMode(ledPin, OUTPUT);
    pinMode(sensorPin, INPUT);

    delay(4000);  // Eine Verzögerung beim Hochfahren verhindert Fehlalarme in
                  // den ersten 3 Sekunden nach Anlegen einer Spannung am Sensor
}

void loop()
{
    // modbus_update() is the only method used in loop(). It returns the total error
    // count since the slave started. You don't have to use it but it's useful
    // for fault finding by the modbus master.
    holdingRegs[TOTAL_ERRORS] = modbus_update(holdingRegs);
    for (byte i = 0; i < 6; i++) {
        holdingRegs[i] = analogRead(i);
        delayMicroseconds(250);
    }

    byte sensorState = digitalRead(sensorPin); // read button states

    // assign the sensorState value to the holding register
    holdingRegs[SENSOR_STATE] = sensorState;

    // read the LED_STATE register value and set the onboard LED high or low with function 16
    byte ledState = holdingRegs[LED_STATE];

    if (ledState) {// set led
        digitalWrite(ledPin, HIGH);
    }
    else { // reset led
        digitalWrite(ledPin, LOW);
    }

    // Zeit seit letzter Bewegung zählen
    if (!sensorState) {           // Keine Bewegung erkannt
      if (prevstate) {            // Wenn die aktuelle nicht-Bewegung erstmals erkannt wurde...
        prevMillis = millis();    // ...aktuellen Zeitpunkt merken und...
        prevstate = 0;            // ...Hilfsvariable setzen und...
        timeSinceMotion = 0;      // ...Zeit seit letzter Bewegung auf 0 setzen
        }
      if (millis() - prevMillis > 1000) {         // Wenn seit Erkennung der aktuellen Bewegung mehr als 1 Sekunde vergangen ist, ...
        timeSinceMotion = timeSinceMotion + 1;    // ...Zeit seit letzter Bewegung um 1 (Sekunde) erhöhen und...
        prevMillis = prevMillis + 1000;           // ...gemerkten Zeitpunkt um 1000 (Millisekunden) erhöhen
      }
    }
    else {                        // Wenn eine Bewegung erkannt wurde...
      if (!prevstate) {           // ...erstmals nach der vorherigen nicht-Bewegung, ...
        prevMillis = millis();    // ...aktuellen Zeitpunkt merken und...
        prevstate = 1;            // ...Hilfsvariable auf 1 (Bewegung) setzen und...
        timeSinceMotion = 0;      // ...Zeit seit letzter Bewegung auf 0 setzen.
        }
    }

    //Zeit seit letzter Bewegung in Register schreiben
    holdingRegs[SINCE_MOVE] = timeSinceMotion;
}
