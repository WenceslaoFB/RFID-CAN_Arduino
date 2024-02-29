#if 0
#include <SPI.h>
#include <PN532_SPI.h>
#include <PN532.h>
#include <NfcAdapter.h>

PN532_SPI pn532spi(SPI, 10);
NfcAdapter nfc = NfcAdapter(pn532spi);

#else

#include <Wire.h>
#include <PN532_I2C.h>
#include <PN532.h>
#include <NfcAdapter.h>

PN532_I2C pn532_i2c(Wire);
NfcAdapter nfc = NfcAdapter(pn532_i2c);
#endif

#include "AA_MCP2515.h"

// TODO: modify CAN_BITRATE, CAN_PIN_CS(Chip Select) pin, and CAN_PIN_INT(Interrupt) pin as required.
const CANBitrate::Config CAN_BITRATE = CANBitrate::Config_8MHz_250kbps;
const uint8_t CAN_PIN_CS = 53;
const int8_t CAN_PIN_INT = 2;

CANConfig config(CAN_BITRATE, CAN_PIN_CS, CAN_PIN_INT);
CANController CAN(config);

byte payloadArray[8] = {0};
const int PN532_RESET = 4; // Pin digital utilizado para el RESET del PN532

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
    if (nfc.tagPresent())
    {
        NfcTag tag = nfc.read();
        //tag.getNdefMessage();
        if (!tag.hasNdefMessage()) {
            Serial.println("Tag is not NDEF formatted or read error occurred.");
            resetPN532();
        } else {
            // El tag está formateado con NDEF, procesa el mensaje
            decodeMsj(tag);
        }
    }
    delay(500);
}

void resetPN532() {
    digitalWrite(PN532_RESET, LOW); // Activa el RESET en el PN532
    delay(10); // Espera un poco para asegurar que el PN532 se reinicie
    digitalWrite(PN532_RESET, HIGH); // Desactiva el RESET, permitiendo que el PN532 funcione nuevamente
    delay(50); // Da tiempo al PN532 para que se reinicie completamente
}

void decodeMsj(NfcTag &tag){
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
                    payloadArray[0] = {0};  // Reinicio del array con 0s
                    payloadArray[1] = {0};
                    payloadArray[2] = {0};
                    payloadArray[3] = {0};
                    payloadArray[4] = {0};
                    payloadArray[5] = {0};
                    payloadArray[6] = {0};
                    payloadArray[7] = {0};

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
      CANFrame frame(0x100, payloadArray, sizeof(payloadArray));//id:0x100 
      CAN.write(frame);
      frame.print("CAN TX");
}







