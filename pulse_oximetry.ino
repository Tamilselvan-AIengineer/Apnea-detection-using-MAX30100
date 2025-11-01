#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include "MAX30100_PulseOximeter.h"

// --- Web Server & Wi-Fi Config ---
const char* ssid = "ESP32_HealthMonitor";
const char* password = "12345678";
IPAddress local_ip(192, 168, 1, 1);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
WebServer server(80);

// --- Sensor Objects ---
PulseOximeter pox;
uint32_t lastLogicUpdate = 0;
const uint32_t LOGIC_UPDATE_MS = 1000; // Run logic every 1 second

// --- Edge Computing: Apnea Detection Globals ---
const int BUFFER_SIZE = 60; // 60 seconds for baseline
float spo2Buffer[BUFFER_SIZE];
int bufferIndex = 0;
bool bufferIsFull = false;

// Event tracking
bool inDesaturationEvent = false;
unsigned long eventStartTime = 0;
const float DESATURATION_THRESHOLD = 3.0; // 3% drop
const unsigned long MIN_EVENT_DURATION_MS = 10000; // 10 seconds

// --- Global State for Web Server (updated by the loop) ---
float g_currentBPM = 0.0;
float g_currentSPO2 = 0.0;
float g_baselineSPO2 = 99.0; // Start with a healthy assumption
String g_eventStatus = "Initializing...";
int g_apneaEventCount = 0;


/* Callback on heartbeat */
void onBeatDetected() {
  Serial.println("💓 Beat detected!");
}

/* Root page - HTML Dashboard */
void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>AI Edge Health Dashboard</title>
<style>
  :root {
    --bg: #0b132b;
    --card: #1c2541;
    --accent: #5bc0be;
    --text: #ffffff;
    --warn: #fcbf49;
    --danger: #ef233c;
  }
  body {
    margin: 0;
    font-family: 'Poppins', sans-serif;
    background: var(--bg);
    color: var(--text);
    text-align: center;
    min-height: 100vh;
    display: flex;
    flex-direction: column;
    align-items: center;
  }
  h1 {
    font-size: 1.8rem;
    margin-top: 20px;
    background: linear-gradient(90deg, #00ffff, #5bc0be);
    -webkit-background-clip: text;
    -webkit-text-fill-color: transparent;
  }
  .grid {
    display: grid;
    grid-template-columns: repeat(auto-fit, minmax(220px, 1fr));
    gap: 20px;
    width: 90%;
    max-width: 900px;
    margin-top: 25px;
  }
  .card {
    background: var(--card);
    border-radius: 20px;
    box-shadow: 0 0 20px rgba(0,0,0,0.4);
    padding: 20px;
    transition: transform 0.3s;
  }
  .card:hover {
    transform: scale(1.05);
  }
  .metric {
    font-size: 2.2rem;
    font-weight: 600;
    margin-top: 10px;
  }
  .status {
    font-size: 1.8rem;
    font-weight: bold;
    margin-top: 15px;
    color: var(--warn);
    animation: pulse 2s infinite;
  }
  @keyframes pulse {
    0% { transform: scale(1); }
    50% { transform: scale(1.05); }
    100% { transform: scale(1); }
  }
  footer {
    margin-top: auto;
    font-size: 0.85rem;
    color: #aaa;
    padding: 10px;
  }
  /* Gauge styling */
  .gauge {
    width: 120px;
    height: 120px;
    border-radius: 50%;
    background: conic-gradient(var(--accent) var(--val), #333 var(--val));
    display: flex;
    align-items: center;
    justify-content: center;
    margin: 10px auto;
    font-size: 1.1rem;
    font-weight: bold;
  }
</style>
</head>
<body>
  <h1>SRM Institute of Science and Technology</h1>
  <div class="grid">
    <div class="card">
      <h2>❤️ Heart Rate</h2>
      <div class="gauge" id="bpm" style="--val:0deg;">-- bpm</div>
    </div>
    <div class="card">
      <h2>🩸 SpO₂</h2>
      <div class="gauge" id="spo2" style="--val:0deg;">-- %</div>
    </div>
    <div class="card">
      <h2>📊 Baseline SpO₂</h2>
      <div class="metric" id="baseline">-- %</div>
    </div>
    <div class="card">
      <h2>⚠️ Status</h2>
      <div class="status" id="status">--</div>
    </div>
    <div class="card">
      <h2>🚨 Apnea Events</h2>
      <div class="metric" id="events">0</div>
    </div>
  </div>

  <footer>AI Edge Health Dashboard © 2025 | Built by Tamilselvan</footer>

  <script>
    async function update() {
      try {
        const res = await fetch('/data');
        const d = await res.json();
        const bpm = parseFloat(d.bpm);
        const spo2 = parseFloat(d.spo2);

        // Update gauge angles
        document.getElementById('bpm').style.setProperty('--val', (Math.min(bpm, 150) / 150) * 360 + 'deg');
        document.getElementById('spo2').style.setProperty('--val', (Math.min(spo2, 100) / 100) * 360 + 'deg');

        // Update text
        document.getElementById('bpm').textContent = d.bpm;
        document.getElementById('spo2').textContent = d.spo2;
        document.getElementById('baseline').textContent = d.baseline;
        document.getElementById('status').textContent = d.status;
        document.getElementById('events').textContent = d.events;

        // Color logic
        const status = document.getElementById('status');
        if (d.status.includes("APNEA")) status.style.color = "var(--danger)";
        else if (d.status.includes("DESATURATION")) status.style.color = "var(--warn)";
        else status.style.color = "#00ff99";
      } catch (e) {
        console.log("Error updating dashboard");
      }
    }
    setInterval(update, 1000);
    update();
  </script>
</body>
</html>
)rawliteral";

  server.send(200, "text/html", html);
}

/* JSON data endpoint */
void handleData() {
  // Send the globally stored values as JSON
  String json = "{";
  json += "\"bpm\":\"" + String(g_currentBPM, 2) + " bpm\",";
  json += "\"spo2\":\"" + String(g_currentSPO2, 2) + " %\",";
  json += "\"baseline\":\"" + String(g_baselineSPO2, 2) + " %\",";
  json += "\"status\":\"" + g_eventStatus + "\",";
  json += "\"events\":\"" + String(g_apneaEventCount) + "\"";
  json += "}";
  server.send(200, "application/json", json);
}

/* 404 handler */
void handle_NotFound() {
  server.send(404, "text/plain", "Not found");
}

void setup() {
  Serial.begin(115200);
  Serial.println("=== ESP32 Health Monitor Starting ===");

  // Initialize MAX30100
  Wire.begin();
  if (!pox.begin()) {
    Serial.println("❌ MAX30100 not found. Check wiring!");
    while (1);
  } else {
    Serial.println("✅ MAX30100 initialized successfully.");
  }

  pox.setOnBeatDetectedCallback(onBeatDetected);

  // Initialize the SpO2 buffer
  for (int i = 0; i < BUFFER_SIZE; i++) {
    spo2Buffer[i] = 99.0; // Assume a healthy baseline to start
  }
  g_baselineSPO2 = 99.0;

  // Setup WiFi Access Point
  Serial.println("Configuring Access Point...");
  WiFi.softAP(ssid, password);
  WiFi.softAPConfig(local_ip, gateway, subnet);
  delay(100);

  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.onNotFound(handle_NotFound);
  server.begin();

  Serial.println("🌐 Web Server started successfully!");
  Serial.print("SSID: "); Serial.println(ssid);
  Serial.print("Visit: http://"); Serial.println(local_ip);
}

void loop() {
  // These must run as fast as possible
  pox.update();
  server.handleClient();

  // This block runs the edge logic and updates the global variables
  // once per second (defined by LOGIC_UPDATE_MS)
  if (millis() - lastLogicUpdate > LOGIC_UPDATE_MS) {
    
    // Get fresh data
    g_currentBPM = pox.getHeartRate();
    g_currentSPO2 = pox.getSpO2();

    // Make sure we have a valid reading
    if (g_currentBPM > 30 && g_currentSPO2 > 70) {
      
      // --- 1. Calculate the Baseline ---
      float baselineSum = 0;
      for (int i = 0; i < BUFFER_SIZE; i++) {
        baselineSum += spo2Buffer[i];
      }
      g_baselineSPO2 = baselineSum / BUFFER_SIZE;

      // Print status to Serial Monitor
      Serial.print("❤️ HR: "); Serial.print(g_currentBPM);
      Serial.print(" | 🩸 SpO₂: "); Serial.print(g_currentSPO2);
      Serial.print(" | 📊 Baseline: "); Serial.print(g_baselineSPO2);

      // --- 2. Run Apnea Detection Logic ---
      if (g_currentSPO2 < (g_baselineSPO2 - DESATURATION_THRESHOLD)) {
        // We are in a desaturation event
        g_eventStatus = "DESATURATION";
        if (!inDesaturationEvent) {
          // This is the start of a new event
          inDesaturationEvent = true;
          eventStartTime = millis();
          Serial.print("  >> DESATURATION DETECTED <<");
        }
      } else {
        // We are not in a desaturation event (or the event just ended)
        g_eventStatus = "Normal";
        if (inDesaturationEvent) {
          // Check if the event lasted long enough to be an "apnea event"
          if (millis() - eventStartTime >= MIN_EVENT_DURATION_MS) {
            g_apneaEventCount++;
            g_eventStatus = "APNEA EVENT";
            Serial.print("  >> APNEA EVENT LOGGED! Total: ");
            Serial.print(g_apneaEventCount);
          } else {
            Serial.print("  (Event too short, ignoring)");
          }
        }
        inDesaturationEvent = false;
      }
      
      Serial.println(); // New line for the serial monitor

      // --- 3. Update the Baseline Buffer ---
      spo2Buffer[bufferIndex] = g_currentSPO2;
      bufferIndex++;
      if (bufferIndex >= BUFFER_SIZE) {
        bufferIndex = 0;
        bufferIsFull = true; // Buffer is now full of real data
      }

    } else {
      Serial.println("No finger detected.");
      g_eventStatus = "No Finger";
    }
    
    // Update the timestamp
    lastLogicUpdate = millis();
  }
}
