#include <SPI.h>
#include <PN532_SPI.h>
#include <PN532.h>
#include <NfcAdapter.h>
#include <Wire.h>
#include <PN532_I2C.h>
#include <AA_MCP2515.h>

#if 0
PN532_SPI pn532spi(SPI, 10);
NfcAdapter nfc = NfcAdapter(pn532spi);
#else
PN532_I2C pn532_i2c(Wire);
NfcAdapter nfc = NfcAdapter(pn532_i2c);
#endif

// TODO: modify CAN_BITRATE, CAN_PIN_CS(Chip Select) pin, and CAN_PIN_INT(Interrupt) pin as required.
const CANBitrate::Config CAN_BITRATE = CANBitrate::Config_8MHz_250kbps;
const uint8_t CAN_PIN_CS = 53;
const int8_t CAN_PIN_INT = 2;

CANConfig config(CAN_BITRATE, CAN_PIN_CS, CAN_PIN_INT);
CANController CAN(config);

byte payloadArray[8] = {0};
const int PN532_RESET = 4; // Pin digital utilizado para el RESET del PN532

unsigned long lastSendTime = 0;  // Variable para almacenar el último tiempo de envío
const unsigned long sendInterval = 1000;  // Intervalo de envío en milisegundos (1 segundo)
unsigned long lastSendTime2 = 0;  // Variable para almacenar el último tiempo de envío
const unsigned long sendInterval2 = 100;  // Intervalo de envío en milisegundos (1 segundo)

void resetPN532();
void decodeMsj(NfcTag &tag);

void setup(void) {
    Serial.begin(9600);
    //Serial.println("NDEF Reader");
    nfc.begin();

    pinMode(PN532_RESET, OUTPUT); // Configura el pin de RESET como salida
    digitalWrite(PN532_RESET, HIGH); // Mantiene el módulo PN532 activo (sin reset)

    while(CAN.begin(CANController::Mode::Normal) != CANController::OK) {
        Serial.println("CAN begin FAIL - delaying for 1 second");
        delay(250);
    }
    Serial.println("CAN begin OK");
}

void loop(void) {
    Serial.println("\nScan a NFC tag\n");
    if (nfc.tagPresent(500)) {
        NfcTag tag = nfc.read();
        if (!tag.hasNdefMessage()) {
            Serial.println("Tag is not NDEF formatted or read error occurred.");
            resetPN532();
        } else {
                Serial.println("Sending CAN message");
                decodeMsj(tag);  // Procesa y envía el mensaje CAN
        }
    }else{
      Serial.println("No tag");
    }
     unsigned long currentTime2 = millis();
    if (currentTime2 - lastSendTime2 >= sendInterval2) {
      // El tiempo actual es mayor o igual que el último tiempo de envío más el intervalo de envío
      sendALIVE();
      lastSendTime2 = currentTime2;
    }
    delay(10);
}

void resetPN532() {
    digitalWrite(PN532_RESET, LOW); // Activa el RESET en el PN532
    delay(10); // Espera un poco para asegurar que el PN532 se reinicie
    digitalWrite(PN532_RESET, HIGH); // Desactiva el RESET, permitiendo que el PN532 funcione nuevamente
    delay(50); // Da tiempo al PN532 para que se reinicie completamente
}

void decodeMsj(NfcTag &tag) {
    int messageLength = tag.getNdefMessage().getRecordCount();
    for (int i = 0; i < messageLength; i++) {
        NdefRecord record = tag.getNdefMessage().getRecord(i);
        
        // Se obtiene el payload del registro
        int payloadLength = record.getPayloadLength();
        byte payload[payloadLength];
        record.getPayload(payload);

        // Se asegura de que el payload es suficientemente largo para descartar los primeros 3 bytes
        if (payloadLength > 3) {
            // Define un array para almacenar hasta 8 bytes del payload, después de descartar los primeros 3 bytes
            memset(payloadArray, 0, sizeof(payloadArray));  // Reinicia el array con 0s

            // Calcula el número de bytes a copiar (el mínimo entre 8 y payloadLength - 3)
            int bytesToCopy = min(payloadLength - 3, 8);

            // Copia los bytes del payload al array, empezando desde el cuarto byte del payload
            memcpy(payloadArray, payload + 3, bytesToCopy);

            // Opcional: Imprime el array payloadArray para verificar
            Serial.print("Payload (sin primeros 3 bytes): ");
            for (int j = 0; j < bytesToCopy; j++) {
                Serial.print(payloadArray[j], HEX);
                Serial.print(" ");
            }  
            Serial.println();
        } else {
            Serial.println("Payload demasiado corto para descartar primeros 3 bytes.");
        }
    }
    // transmit
    CANFrame frame(0x100, payloadArray, sizeof(payloadArray)); // id:0x100 
    CAN.write(frame);
    frame.print("CAN TX");
}


void sendALIVE(){

    byte aliveMessage[8] = { 'A', 'L', 'I', 'V', 'E', 0, 0, 0 };
    CANFrame frame(0x101, aliveMessage, sizeof(aliveMessage)); //id:0x101 
    CAN.write(frame);
    frame.print("CAN ALIVE TX");
}





