#pragma once

/// BEGIN  RTC part
// Include the RTC library
#include "RTC.h"
//Include the NTP library
#include <NTPClient.h>
// Wifi R4
#include <WiFiS3.h>
#include <WiFiUdp.h>
#include "arduino_secrets.h" 




/// END RTC/Wifi

void connectToWiFi();
void syncClock();
void printWifiStatus();