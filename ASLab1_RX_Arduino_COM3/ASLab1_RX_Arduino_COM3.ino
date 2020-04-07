#include "VirtualWire.h"
#include "Shared.h"

#define RXPIN 11
#define LEDPIN 13

uint32_t bytesReceived = 0;
uint32_t corruptedBytes = 0;
uint32_t missingBytes = 0;

uint32_t messagesReceived = 0;
uint32_t corruptedMessages = 0;
uint32_t missedMessages = 0;

uint32_t missedInARow = 0;

unsigned long start = 0;

#define FCS_OK 0xf0b8

void setup()
{
	vw_set_ptt_inverted(true); // Required for DR3100
	vw_set_rx_pin(RXPIN);
	vw_setup(4000);  // Bits per sec
	pinMode(LEDPIN, OUTPUT);

	vw_rx_start(); // Start the receiver PLL running
}

void PrintTimeRunning()
{
	// Timing
	unsigned long now = millis();
	auto delta = now - start;
	if (delta >= 60000)
	{
		// Minutes
		Serial.print("Time running:\t");
		Serial.print(delta / 60000);
		Serial.print(" minutes.\n");
	}
	else
	{
		// Seconds
		Serial.print("Time running:\t");
		Serial.print(delta / 1000);
		Serial.print(" seconds.\n");
	}
}

void loop()
{
	Serial.print("Starting receive loop!\n");
	start = millis();

	while (true)
	{
		uint8_t buf[VW_MAX_MESSAGE_LEN];
		uint8_t buflen = VW_MAX_MESSAGE_LEN;

		digitalWrite(LEDPIN, 0);

		// Wait for incoming burst
		if (!vw_wait_rx_max(timePerBurst))
		{
			// Missed a message from a burst!
			//Serial.print("Missed a burst message.\n");
			missedMessages++;

			missedInARow++;

			if (missedInARow % 10 == 0)
			{
				Serial.print("Missed messages so far:\t");
				Serial.print(missedInARow);
				Serial.print("\n");
				PrintTimeRunning();
			}
			continue;
		}

		missedInARow = 0;
		auto res = vw_get_message(buf, &buflen);

		Serial.print("-------------------\n");

		// FCS OK?
		digitalWrite(LEDPIN, 1);
		bytesReceived += buflen;
		uint8_t actualBufferSize = sendBufferSize - 1;

		// Fetch corrupted bytes
		for (int i = 0; i < sendBufferSize; ++i)
		{
			if (i > buflen || buf[i] == '\0')
			{
				missingBytes += sendBufferSize - 1 - i;
				actualBufferSize = sendBufferSize - 1 - i;
				break;
			}

			if (buf[i] != sendBuffer[i])
			{
				corruptedBytes++;
			}
		}

		// Messages received
		messagesReceived++;
		if (res != FCS_OK)
		{
			corruptedMessages++;

			// FCS/CRC OK?
			/*Serial.print("Message is corrupted (FCS/CRC mismatch).\n");
			Serial.print("Got: ");
			Serial.print(res, HEX);
			Serial.print(" | Expected: ");
			Serial.print(FCS_OK, HEX);
			Serial.print("\n");*/
		}

		// Stats!
		PrintTimeRunning();

		// Buffer
		Serial.print("\n");
		Serial.print("BUFFER:\n");
		Serial.print("Received:\t\"");
		Serial.print((char*)buf);
		Serial.print("\"\nReported size:\t");
		Serial.print(buflen);
		Serial.print("\nActual size:\t");
		Serial.print(actualBufferSize);
		Serial.print("\n");

		// Messages
		Serial.print("\n");
		Serial.print("MESSAGES:\n");
		Serial.print("Received:\t");
		Serial.print(messagesReceived);

		Serial.print("\nCorrupted:\t");
		Serial.print(corruptedMessages);
		Serial.print(" (");
		Serial.print(100.f / messagesReceived * corruptedMessages);
		Serial.print("\%)");

		Serial.print("\nMissing:\t");
		uint32_t missed = missedMessages - messagesReceived;
		Serial.print(missedMessages - messagesReceived);
		Serial.print(" (");
		Serial.print(100.f / missedMessages * missed);
		Serial.print("\%)");
		Serial.print("\n");

		// Bytes
		Serial.print("\n");
		Serial.print("BYTES:\n");
		Serial.print("Received:\t");
		Serial.print(bytesReceived);

		Serial.print("\nCorrupted:\t");
		Serial.print(corruptedBytes);
		Serial.print(" (");
		Serial.print(100.f / bytesReceived * corruptedBytes);
		Serial.print("\%)");

		Serial.print("\nMissing:\t");
		Serial.print(missingBytes);
		Serial.print(" (");
		Serial.print(100.f / bytesReceived * missingBytes);
		Serial.print("\%)\n");

		Serial.print("-------------------\n");
	}
}
