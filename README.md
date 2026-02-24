# IoT-Air-Quality-Monitor-esp32 🛜
ESP32-based IoT air quality monitoring system includes a real-time alert mechanism that instantly notifies users when environmental conditions become unsafe. The system continuously collects data from MQ135 (CO₂), MQ7 (CO), DHT22/11, and the GP2Y1010 PM2.5 dust sensor, and transmits this data to a cloud dashboard for live monitoring.

# Real-Time Email Alert System 📧

The ESP32-based IoT air quality monitoring system includes a real-time alert mechanism that instantly notifies users when environmental conditions become unsafe. The system continuously collects data from MQ135 (CO₂), MQ7 (CO), DHT22 (temperature and humidity), and the GP2Y1010 PM2.5 dust sensor, and transmits this data to a cloud dashboard for live monitoring.

Whenever any parameter crosses predefined safety thresholds, an automatic alert is triggered immediately. The cloud platform processes the incoming sensor data and sends an email notification in real time to the registered user. This ensures that hazardous conditions are detected and communicated without delay, enabling quick action and preventive safety measures.

# Real-time alert triggers include:

- High CO₂ levels indicating poor indoor ventilation

- Elevated CO concentration indicating toxic gas presence

- Increased PM2.5 levels affecting air quality and respiratory health

- Extreme temperature or humidity conditions impacting comfort and safety

This real-time alert capability transforms the system from a passive monitoring device into an active environmental safety solution. It is suitable for smart homes, industrial safety systems, laboratories, classrooms, and indoor pollution monitoring applications where immediate response is critical.
