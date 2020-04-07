#pragma once

char sendBuffer[] = "SL";
const uint8_t sendBufferSize = sizeof(sendBuffer);
// Cannot exceed VW_MAX_MESSAGE_LEN (30)!

const uint32_t timePerBurst = 1000; // ms