#include <SimpleModbusMaster.h>

/* To communicate with a slave you need to create a
   packet that will contain all the information
   required to communicate to that slave.

   In general to communicate with a slave using modbus
   RTU you will request information using the specific
   slave id, the function request, the starting address
   and lastly the number of registers to request.
   Function 3 and 16 are supported. In addition to
   this broadcasting (id = 0) is supported on function 16.
   Constants are provided for:
   Function 3 -  READ_HOLDING_REGISTERS
   Function 16 - PRESET_MULTIPLE_REGISTERS

   Dieses Programm schreibt mit Hilfe der Modbus-Funktion 16 in
   jeweils ein Register der Slaves eine 0 oder eine 1, wodurch
   die im Arduino Nano integrierte LED sekündlich getriggert wird.
*/

// Integrierte LED weist auf einen Fehler bei der Kommunikation hin
#define connection_error_led 13

//////////////////// Port information ///////////////////
#define baud 19200
#define timeout 1000
#define polling 200 // Zykluszeit, mit der die Slaves abgefragt werden

// Nach 3 (bzw. selbst definierte Anzahl) fehlgeschlagenen Sendeversuchen des Pakets wird die
// Kommunikation für dieses Paket gestoppt. Das Paket kann wiederaufgenommen werden, indem
// die "connection"-Variable auf true gesetzt wird, wie unten gezeigt
#define retry_count 3

// Pin für Umschaltung zwischen Empfangen und Senden am MAX485
#define TxEnablePin 2

// enum bietet eine einfache Methode um neue Pakete anzulegen
// TOTAL_NO_OF_PACKETS wird automatisch aktualisiert
enum {
    PACKET1,
    PACKET2,
    // Der letzte Eintrag muss immer gleich bleiben:
    TOTAL_NO_OF_PACKETS
};

// Ein Array für die Modbus Pakete wird erstellt, um diese an
// die modbus_update()-Funktion zu übergeben.
Packet packets[TOTAL_NO_OF_PACKETS];

// the array explicitly. E.g. packets[PACKET1].id = 2;
// This does become tedious though...

// Die packetPointer erleichtern den Zugriff auf die einzelnen Pakete.
// Alternativ kann man explizit auf die Array-Elemente zugreifen, z.B.:
// packets[PACKET1].id = 2
packetPointer packet1 = &packets[PACKET1];
packetPointer packet2 = &packets[PACKET2];


unsigned int write_regs[1];
unsigned long last_toggle = 0;
// Eine Hilfsvariable zum Umschalten der LED:
unsigned int led_trig;

void setup()
{
    // Schreibe 1 Wert aus dem Array write_regs in das Register mit der Adresse 6 des Slaves mit der ID 2
    packet1->id = 2;
    packet1->function = PRESET_MULTIPLE_REGISTERS;
    packet1->address = 6;
    packet1->no_of_registers = 1;
    packet1->register_array = write_regs;
    
    // Das gleiche nochmal für den Slave mit der ID 3
    packet2->id = 3;
    packet2->function = PRESET_MULTIPLE_REGISTERS;
    packet2->address = 6;
    packet2->no_of_registers = 1;
    packet2->register_array = write_regs;
    
    // Modbus-Kommunikation wird eingerichtet
    // Parameter: Baudrate, Timeout-Zeit in ms, Anzahl Fehlversuche bis Kommunikation mit Slave gestoppt wird,
    // Driver Enable Pin für MAX485, Array der Modbus-Pakete, Gesamtanzahl der Pakete
    modbus_configure(baud, timeout, polling, retry_count, TxEnablePin, packets, TOTAL_NO_OF_PACKETS);

    // Die integrierte LED wird als Ausgang definiert
    pinMode(connection_error_led, OUTPUT);
}

void loop()
{
    unsigned int connection_status = modbus_update(packets);

    // Die Funktion millis() gibt die Anzahl Millisekunden zurück,
    // die seit Start/Reset des Mikrocontrollers vergangen sind.
    // Wenn mehr als 1000ms seit dem letzten Triggern vergangen sind, wird der neue Triggerzeitpunkt gemerkt
    // und der Wert led_trig getriggert, sprich zwischen 0 und 1 gewechselt. Dieser Wert wird dann auch in
    // write_regs[0] geschrieben, um beim nächsten Modbus-Zyklus in den Slave geschrieben zu werden.
    if (millis() - last_toggle > 1000) {
        last_toggle = millis();
        if (led_trig) {led_trig=0;} else {led_trig=1;}
        write_regs[0] = led_trig;
    }

    if (connection_status != TOTAL_NO_OF_PACKETS) {
        digitalWrite(connection_error_led, HIGH);
        // Eine nach 3 Fehlern deaktivierte Verbindung kann mit folgendem Befehl reaktiviert werden:
        //packets[connection_status].connection = true;
    } else {digitalWrite(connection_error_led, LOW);}
}
