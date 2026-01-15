# indoor_smoke_exp

Indoor smoke exposure is a major health concern in communities that use biomass fuels for
cooking, leading to elevated levels of carbon monoxide, ammonia, and particulate matter.
Continuous exposure to these pollutants significantly increases the risk of respiratory and
cardiovascular diseases, especially among women and children. This work presents a compact,
low-cost IoT-based system for real-time monitoring of indoor air quality in household kitchens.
The system employs an MQ135 gas sensor for detecting harmful gases, a DHT11 module for
temperature and humidity measurement, and an LDR to check visibility changes caused by
dense smoke. These sensors interface with an ESP32 microcontroller, which processes
incoming data, compares it against calibrated thresholds, and triggers suitable responses. When
unsafe levels are detected, visual and audio alerts are activated, and a servo-driven mechanism
helps ventilation. Air-quality status and sensor values are continuously displayed through an
OLED interface. The system was evaluated under controlled clean-air and smoke-rich
conditions. Results showed consistent pollutant detection, fast threshold response, and clear
classification of safe, warning, and hazardous conditions. Its low power usage, affordability,
and modularity make it suitable for deployment in resource-limited homes. This work proves
a practical foundation for future extensions involving wireless data logging and cloud-based
analysis.
