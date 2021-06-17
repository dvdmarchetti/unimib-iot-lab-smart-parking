# Assignment 3 - Smart Parking System
### Idea
We developed a private/condominum smart parking solution which can be composed with different device types: RFID entrances, parking slot monitoring, intrusion detection, automatic lighting and automatic roof control based on weather.

### Project structure
Each subsystem has its own folder which contains all the necessary code to work on a ESP8266. The only exception is `master` which supports only ESP32.

The dashboard is a plain html file which can be opened directly in browser and has to be pointed to the IP address of the master.

### External libraries
We used a few different libraries which cannot be installed thought the Arduino Library Manager. Therefore we provide the zip files for a manual install under the `libraries` folder.
Usage examples and documentation can be seen on the respective github pages of each library:

- [Universal Arduino Telegram Bot](https://github.com/witnessmenow/Universal-Arduino-Telegram-Bot)
- [ESP Async WebServer](https://github.com/me-no-dev/ESPAsyncWebServer)
- [ESP Async TCP](https://github.com/me-no-dev/ESPAsyncTCP)
- [ESP NTP Client](https://github.com/gmag11/ESPNtpClient)

---
## Original assignment proposal

**CORE:**
The idea is to build an IoT monitor system which includes all the technologies we have explored during the lessons: databases (mysql or influxdb), mqtt, smart networks, http/api, telegram, json, web server, low power consumption solutions, ecc.
Use the first and second assignments as starting point. You are free to design your project on the basis of your ideas, the only requirement is that the project should include all the technologies listed above.


**INGREDIENTS:**
- Micro of your choice
- Sensor or actuators of your choice.

**EXPECTED DELIVERABLES:**
- Github code upload at: [Classroom link](https://classroom.github.com/g/QBREQgXa)
- Powerpoint presentation of 10 mins for single student and 15 mins for each group.
