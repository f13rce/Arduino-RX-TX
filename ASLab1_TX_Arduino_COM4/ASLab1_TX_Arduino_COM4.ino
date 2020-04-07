#include "VirtualWire.h"
#include "Shared.h"

char* pController;

#define TXPIN 12
#define LEDPIN 13

uint32_t messagesTransmitted = 0;
uint32_t bytesTransmitted = 0;
const uint32_t bytesPerBurst = sendBufferSize;

//const uint32_t totalBursts = 20;
const uint32_t totalBursts = 0xFFFFFFFF;

const uint32_t totalBytesToSend = bytesPerBurst * totalBursts;
bool hasBursted = false;

void setup()
{
	pinMode(LEDPIN, OUTPUT);
	vw_set_ptt_inverted(true);
	vw_set_tx_pin(TXPIN);
	vw_setup(4000);
}

void loop()
{
	if (!hasBursted)
	{
		hasBursted = true;

		for (int i = 0; i < totalBursts; ++i)
		{
			messagesTransmitted++;

			digitalWrite(LEDPIN, 1);
			// Create buffer to send
			// Send data
			vw_send((uint8_t*)sendBuffer, sendBufferSize);
			vw_wait_tx();

			// Stats
			bytesTransmitted += bytesPerBurst;

			//Serial.print("Bytes sent:\t");
			//Serial.print(buffer);
			//Serial.print("\n");

			Serial.print("Burst sent!\n");
			
			Serial.print("Total msgs sent:\t");
			Serial.print(messagesTransmitted);
			Serial.print("\n");

			Serial.print("Total bytes sent:\t");
			Serial.print(bytesTransmitted);
			Serial.print("/");
			Serial.print(totalBytesToSend);
			Serial.print(" (");
			Serial.print(100.f / totalBytesToSend * bytesTransmitted);
			Serial.print("\%)\n");
			digitalWrite(LEDPIN, 0);

			// Wait for next burst
			delay(timePerBurst);
		}

		Serial.print("All done! Total bytes submitted:\n\t");
		Serial.print(bytesTransmitted);
	}
}
