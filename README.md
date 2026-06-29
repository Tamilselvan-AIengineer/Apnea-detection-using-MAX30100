# 🫁 Apnea Detection using MAX30100 and ESP32

## 📌 Overview

This project implements a **real-time edge-based sleep apnea detection system** using an **ESP32** and the **MAX30100 Pulse Oximeter Sensor**. The system continuously monitors **Heart Rate (BPM)** and **Blood Oxygen Saturation (SpO₂)**, calculates a dynamic baseline, detects oxygen desaturation events, and identifies potential apnea episodes without relying on cloud processing.

A built-in **Wi-Fi Access Point** hosts a responsive web dashboard that displays live health metrics, apnea alerts, and event statistics.

---

# Features

* ❤️ Real-time Heart Rate Monitoring
* 🩸 Real-time SpO₂ Monitoring
* 📊 Dynamic Baseline SpO₂ Calculation
* ⚠️ Oxygen Desaturation Detection
* 🚨 Automatic Apnea Event Detection
* 🌐 ESP32 Wi-Fi Access Point
* 📱 Responsive Web Dashboard
* ⚡ Edge Computing (No Internet Required)
* 📈 Live JSON API for Sensor Data
* 🔄 Auto-refresh Dashboard Every Second

---

# Hardware Requirements

| Component                      | Quantity    |
| ------------------------------ | ----------- |
| ESP32 Development Board        | 1           |
| MAX30100 Pulse Oximeter Sensor | 1           |
| Jumper Wires                   | As Required |
| USB Cable                      | 1           |
| Power Supply                   | 5V          |

---

# Software Requirements

* Arduino IDE
* ESP32 Board Package
* MAX30100 Pulse Oximeter Library
* WiFi Library
* WebServer Library
* Wire Library

---

# Circuit Connections

| MAX30100 | ESP32   |
| -------- | ------- |
| VIN      | 3.3V    |
| GND      | GND     |
| SDA      | GPIO 21 |
| SCL      | GPIO 22 |

---

# Working Principle

## Step 1: Sensor Initialization

The ESP32 initializes the MAX30100 sensor and verifies communication over I2C.

---

## Step 2: Data Acquisition

Every second, the ESP32 reads:

* Heart Rate (BPM)
* Blood Oxygen Saturation (SpO₂)

---

## Step 3: Baseline Calculation

The last **60 seconds** of SpO₂ values are stored in a circular buffer.

Average Baseline:

Baseline SpO₂ = Average of Previous 60 Readings

This allows the system to adapt to the user's normal oxygen level.

---

## Step 4: Desaturation Detection

If

Current SpO₂ < Baseline − 3%

the system enters a **Desaturation Event**.

---

## Step 5: Apnea Detection

If the oxygen desaturation lasts for at least

10 seconds

the event is classified as a potential **Apnea Event**.

The event counter is incremented automatically.

---

# Detection Parameters

| Parameter              | Value      |
| ---------------------- | ---------- |
| Baseline Window        | 60 seconds |
| Desaturation Threshold | 3%         |
| Minimum Event Duration | 10 seconds |
| Update Interval        | 1 second   |

---

# Web Dashboard

The ESP32 creates its own Wi-Fi hotspot.

SSID

ESP32_HealthMonitor

Password

12345678

Open your browser and visit

http://192.168.1.1

The dashboard displays:

* ❤️ Heart Rate
* 🩸 SpO₂
* 📊 Baseline SpO₂
* ⚠️ Current Status
* 🚨 Total Apnea Events

Dashboard updates automatically every second.

---

# Status Messages

| Status       | Meaning                       |
| ------------ | ----------------------------- |
| Initializing | Sensor Starting               |
| Normal       | Healthy Reading               |
| DESATURATION | Oxygen Drop Detected          |
| APNEA EVENT  | Possible Sleep Apnea Detected |
| No Finger    | Finger Not Detected           |

---

# API Endpoint

Live sensor values can also be accessed as JSON.

GET

```
http://192.168.1.1/data
```

Example Response

```json
{
  "bpm":"74.35 bpm",
  "spo2":"98.12 %",
  "baseline":"98.85 %",
  "status":"Normal",
  "events":"2"
}
```

---

# Project Structure

```
Apnea-Detection-using-MAX30100
│
├── README.md
├── Apnea_Detection.ino
├── images
│   ├── dashboard.png
│   ├── circuit.png
│   └── setup.jpg
└── LICENSE
```

---

# Edge Computing Logic

```
MAX30100 Sensor
        │
        ▼
ESP32 Reads BPM & SpO₂
        │
        ▼
Calculate Baseline
        │
        ▼
Compare Current SpO₂
        │
        ▼
Detect Desaturation
        │
        ▼
Duration ≥ 10 sec?
       / \
     Yes  No
      │    │
      ▼    ▼
Log Apnea  Ignore
      │
      ▼
Update Dashboard
```

---

# Applications

* Sleep Apnea Screening
* Home Health Monitoring
* Remote Patient Monitoring
* IoT Healthcare
* Edge AI Medical Devices
* Biomedical Engineering Projects
* Smart Health Monitoring Systems

---

# Future Improvements

* Cloud Data Storage
* Mobile Application
* Firebase Integration
* Machine Learning-based Apnea Prediction
* Patient History Logging
* SMS and Email Alerts
* Doctor Dashboard
* MQTT Integration
* TinyML-based Classification
* Multi-Patient Monitoring

---

# Author

**Tamilselvan M**
**NIMALAN ARASU S**

B.Tech Artificial Intelligence

SRM Institute of Science and Technology

---

# License

This project is released under the MIT License.

Feel free to use, modify, and improve this project for educational and research purposes.
