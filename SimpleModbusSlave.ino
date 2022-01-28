#include <SimpleModbusSlave.h>

#define  ledPin  13 // integrierte led 

/* Dieses Beispielprogramm enthält 8 Holding Register. Davon 6 analoge Eingänge, ein digitaler Ausgang und ein Register für die aufgetretenen Fehler seit dem Start.
   Funktion 5 (write single coil) für Digitalausgänge wurde nicht implementiert, deshalb wird ein komplettes 16 Bit Register für den Digitalausgang verwendet. Dieses wird mit Funktion 16 beschrieben, um darüber die eingebaute LED des Arduino Nanos anzusteuern.

   Mit der Anweisung enum können Register einfacher hinzugefügt und entfernt werden. Dies spart den Aufwand des #define-Prozesses bei jeder Änderung der Register und lässt die Registerstruktur auf einen Blick erkennen. */
enum {
    // Das erste Register startet mit der Adresse 0.
    ADC0,
    ADC1,
    ADC2,
    ADC3,
    ADC4,
    ADC5,
    LED_STATE,        // LED_STATE ist das 7. Register, liegt somit auf Adresse 6.
    TOTAL_ERRORS,     // Anzahl aufgetretener Fehler
    TOTAL_REGS_SIZE   // enthält automatisch die Gesamtzahl der Register
};

unsigned int holdingRegs[TOTAL_REGS_SIZE]; // Holding Register Array

void setup()
{
    // Parameter: Baudrate, Slave ID, Transmit enable pin, Größe der Holding Register, low latency Betrieb
    modbus_configure(19200, 2, 2, TOTAL_REGS_SIZE, 0);
    pinMode(ledPin, OUTPUT);  // Integrierte LED als Ausgang festlegen
}

void loop()
{
    // modbus_update() arbeitet die Modbus-Kommunikation ab und gibt die Anzahl Fehler seit Start aus
    holdingRegs[TOTAL_ERRORS] = modbus_update(holdingRegs);
    // 6 ADC-Werte werden nacheinander eingelesen und in die Holding Register geschrieben
    for (byte i = 0; i < 6; i++) {
        holdingRegs[i] = analogRead(i);
        delayMicroseconds(50);
    }

    // Wert des Registers LED_STATE auslesen und eingebaute LED entsprechend Ein-/Ausschalten
    byte ledState = holdingRegs[LED_STATE];
    if (ledState) {digitalWrite(ledPin, HIGH);} else {digitalWrite(ledPin, LOW);}
}
