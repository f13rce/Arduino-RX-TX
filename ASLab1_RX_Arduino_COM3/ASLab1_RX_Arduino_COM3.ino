#include "VirtualWire.h"
#include "Shared.h"

//#include "LinkedList.hpp"

#define RXPIN 11
#define LEDPIN 13

uint32_t totalBytes = 0;
uint32_t bytesReceived = 0;
uint32_t corruptedBytes = 0;
uint32_t missingBytes = 0;

uint32_t totalMessages = 0;
uint32_t messagesReceived = 0;
uint32_t corruptedMessages = 0;
uint32_t missedMessages = 0;

uint32_t missedInARow = 0;

unsigned long start = 0;
const unsigned long stopTime = 300000;
bool allowLoop = true;

bool hasYetToReceiveFirstMessage = true;

#define FCS_OK 0xF0B8

struct Statistic
{
	float time;

	uint32_t totalMessages;
	uint32_t receivedMessages;
	uint32_t corruptedMessages;
	uint32_t missingMessages;

	uint32_t totalBytes;
	uint32_t receivedBytes;
	uint32_t corruptedBytes;
	uint32_t missingBytes;

	uint8_t buffer[VW_MAX_MESSAGE_LEN];

};
//LinkedList<Statistic> statistics;

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

		auto minutes = delta / 60000;
		auto seconds = (delta / 60000) / 1000;

		Serial.print(" minutes and ");
		Serial.print(seconds);
		Serial.print("seconds.\n");
	}
	else
	{
		// Seconds
		Serial.print("Time running:\t");
		Serial.print(delta / 1000);
		Serial.print(" seconds.\n");
	}
}

void PrintCSVHeader()
{
	// Header
	Serial.print("Time (seconds), Total messages, Received messages, Corrupted messages, Missing messages, Total bytes, Received bytes, Corrupted bytes, Missing bytes, Buffer\n");
}

void PrintCSVStatistic(const Statistic& acStatistic)
{
	Serial.print((int32_t)acStatistic.time);
	Serial.print(", ");

	Serial.print(acStatistic.totalMessages);
	Serial.print(", ");
	Serial.print(acStatistic.receivedMessages);
	Serial.print(", ");
	Serial.print(acStatistic.corruptedMessages);
	Serial.print(", ");
	Serial.print(acStatistic.missingMessages);
	Serial.print(", ");

	Serial.print(acStatistic.totalBytes);
	Serial.print(", ");
	Serial.print(acStatistic.receivedBytes);
	Serial.print(", ");
	Serial.print(acStatistic.corruptedBytes);
	Serial.print(", ");
	Serial.print(acStatistic.missingBytes);
	Serial.print(", ");

	Serial.print((char*)acStatistic.buffer);
	Serial.print("\n");
}

void PrintCSV()
{
	// Header
	Serial.print("Time (seconds), Total messages, Received messages, Corrupted messages, Missing messages, Total bytes, Received bytes, Corrupted bytes, Missing bytes\n");

	/*if (statistics.moveToStart())
	{
		do {
			auto stat = statistics.getCurrent();
		
			Serial.print(stat.time);
			Serial.print(", ");

			Serial.print(stat.totalMessages);
			Serial.print(", ");
			Serial.print(stat.receivedMessages);
			Serial.print(", ");
			Serial.print(stat.corruptedMessages);
			Serial.print(", ");
			Serial.print(stat.missingMessages);
			Serial.print(", ");

			Serial.print(stat.totalBytes);
			Serial.print(", ");
			Serial.print(stat.receivedBytes);
			Serial.print(", ");
			Serial.print(stat.corruptedBytes);
			Serial.print(", ");
			Serial.print(stat.missingBytes);
			Serial.print("\n");
		} while (statistics.next());
	}*/
}

void loop()
{
	if (!allowLoop)
	{
		return;
	}

	Serial.print("Starting receive loop!\n");

	while (true)
	{
		uint8_t buf[VW_MAX_MESSAGE_LEN];
		uint8_t buflen = VW_MAX_MESSAGE_LEN;

		digitalWrite(LEDPIN, 0);

		if (hasYetToReceiveFirstMessage)
		{
			Serial.print("Waiting for first ever message to start the test...\n");
			vw_wait_rx();
			Serial.print("First ever message has been received!\n");
			hasYetToReceiveFirstMessage = false;
			start = millis();
			PrintCSVHeader();
		}
		else
		{
			if (millis() - start >= stopTime)
			{
				Serial.print("--- DONE RUNNING TEST ---");
				allowLoop = false;
				//PrintCSV();
				return;
			}
		}

		totalMessages = (millis() - start) / timePerBurst;
		totalBytes = totalMessages * (sendBufferSize - 1);

		// Wait for incoming burst
		if (!vw_wait_rx_max(timePerBurst))
		{
			// Missed a message from a burst!
			//Serial.print("Missed a burst message.\n");
			missedMessages++;

			missedInARow++;

			if (missedInARow % 10 == 0)
			{
				//Serial.print("Missed messages so far:\t");
				//Serial.print(missedInARow);
				//Serial.print("\n");
				//PrintTimeRunning();
			}
			continue;
		}

		missedInARow = 0;
		auto res = vw_get_message(buf, &buflen);

		//Serial.print("-------------------\n");

		// FCS OK?
		digitalWrite(LEDPIN, 1);
		bytesReceived += buflen;
		uint8_t actualBufferSize = sendBufferSize - 1;

		// Fetch corrupted bytes
		for (int i = 0; i < sendBufferSize; ++i)
		{
			if (i > buflen || (buf[i] < 'A' && buf[i] > 'Z' && buf[i] != '@'))
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
		Statistic stat;
		stat.time = float(millis() - start) / 1000.f;

		//PrintTimeRunning();

		// Buffer
		/*Serial.print("\n");
		Serial.print("BUFFER:\n");
		Serial.print("Received:\t\"");
		Serial.print((char*)buf);
		Serial.print("\"\nReported size:\t");
		Serial.print(buflen);
		Serial.print("\nActual size:\t");
		Serial.print(actualBufferSize);
		Serial.print("\n");*/

		// Messages
		stat.totalMessages = totalMessages;

		/*Serial.print("\n");
		Serial.print("MESSAGES:\n");
		Serial.print("Received:\t");
		Serial.print(messagesReceived);*/
		stat.receivedMessages = messagesReceived;

		/*Serial.print("\nCorrupted:\t");
		Serial.print(corruptedMessages);
		Serial.print(" (");
		Serial.print(100.f / totalMessages * corruptedMessages);
		Serial.print("\%)");*/
		stat.corruptedMessages = corruptedMessages;

		//Serial.print("\nMissing:\t");
		int32_t missed = totalMessages - messagesReceived;
		if (missed < 0)
		{
			missed = 0;
		}
		/*Serial.print(missed);
		Serial.print(" (");
		Serial.print(100.f / totalMessages * missed);
		Serial.print("\%)");
		Serial.print("\n");*/
		stat.missingMessages = missed;

		// Bytes
		stat.totalBytes = totalBytes;

		/*Serial.print("\n");
		Serial.print("BYTES:\n");
		Serial.print("Received:\t");
		Serial.print(bytesReceived);*/
		stat.receivedBytes = bytesReceived;

		/*Serial.print("\nCorrupted:\t");
		Serial.print(corruptedBytes);
		Serial.print(" (");
		Serial.print(100.f / bytesReceived * corruptedBytes);
		Serial.print("\%)");*/
		stat.corruptedBytes = corruptedBytes;

		/*Serial.print("\nMissing:\t");
		Serial.print(missingBytes);
		Serial.print(" (");
		Serial.print(100.f / bytesReceived * missingBytes);
		Serial.print("\%)\n");*/
		stat.missingBytes = missingBytes;

		memcpy(&stat.buffer[0], &buf[0], VW_MAX_MESSAGE_LEN);

		PrintCSVStatistic(stat);
		//statistics.Append(stat);

		//Serial.print("-------------------\n");
	}
}
