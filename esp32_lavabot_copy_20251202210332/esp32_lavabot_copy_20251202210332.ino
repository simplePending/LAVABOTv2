#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <time.h>
#include <LittleFS.h>

// ---------- WiFi Credentials ----------
const char* ssid     = "HUAWEI-sGJ3";
const char* password = "5VneQzsk";

// ---------- Web Server ----------
ESP8266WebServer server(80);

// ---------- Motor Pins ----------
#define LPWM_L 4   // D2 - Left Motor PWM
#define RPWM_L 5   // D1 - Left Motor Reverse PWM
#define LPWM_R 14  // D5 - Right Motor PWM
#define RPWM_R 12  // D6 - Right Motor Reverse PWM

// ---------- Relays ----------
#define RELAY1_PIN 16  // D0 - UV Light
#define RELAY2_PIN 15  // D8 - Relay 2

// ---------- IR Sensors (Corner Collision Detection) ----------
#define IR_FRONT_LEFT   3   // RX (GPIO3)
#define IR_FRONT_RIGHT  1   // TX (GPIO1)
#define IR_BACK_LEFT    9   // SD2 (GPIO9)
#define IR_BACK_RIGHT   10  // SD3 (GPIO10)

// ---------- Variables ----------
const int MOTOR_SPEED = 100;  // Fixed speed (0-255)
const unsigned long UV_DURATION = 30 * 60 * 1000;  // 30 minutes in milliseconds
const unsigned long POSITION_WAIT_TIME = 5 * 60 * 1000;  // 5 minutes wait time

time_t scheduledTime = 0;
bool scheduleSet = false;
bool autoMode = false;  // Automatic navigation mode
unsigned long uvStartTime = 0;  // Track when UV was turned on
bool uvCycleComplete = false;  // Track if 30-min cycle is done

// Position tracking
unsigned long positionStartTime = 0;  // When robot stopped at current position
bool waitingAtPosition = false;  // Is robot waiting at a position?
bool emergencyStop = false;  // Master emergency stop flag
// ---------- HTML Web Interface ----------
String htmlPage() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>LAVABOT Controller</title>
  <style>
    * {
      margin: 0;
      padding: 0;
      box-sizing: border-box;
    }

    body {
      font-family: 'Segoe UI', Arial, sans-serif;
      background: linear-gradient(135deg, #1e3c72 0%, #2a5298 100%);
      color: white;
      padding: 20px;
      min-height: 100vh;
    }

    .header {
      text-align: center;
      margin-bottom: 30px;
      padding: 20px;
      background: rgba(255, 255, 255, 0.1);
      border-radius: 15px;
      backdrop-filter: blur(10px);
    }

    .header h1 {
      font-size: 24px;
      margin-bottom: 5px;
    }

    .header p {
      font-size: 14px;
      opacity: 0.9;
    }

    .emergency-section {
      max-width: 1200px;
      margin: 0 auto 20px;
      display: grid;
      grid-template-columns: 1fr 1fr;
      gap: 15px;
    }

    .emergency-button {
      padding: 20px;
      border: none;
      border-radius: 15px;
      font-size: 18px;
      font-weight: bold;
      cursor: pointer;
      transition: all 0.3s ease;
      display: flex;
      align-items: center;
      justify-content: center;
      gap: 10px;
    }

    .emergency-stop {
      background: linear-gradient(135deg, #d32f2f 0%, #c62828 100%);
      color: white;
      box-shadow: 0 8px 32px rgba(211, 47, 47, 0.5);
      animation: pulse-red 2s infinite;
    }

    .emergency-stop:hover {
      transform: scale(1.05);
      box-shadow: 0 12px 40px rgba(211, 47, 47, 0.7);
    }

    .emergency-reset {
      background: linear-gradient(135deg, #388e3c 0%, #2e7d32 100%);
      color: white;
    }

    .emergency-reset:hover {
      transform: scale(1.05);
    }

    @keyframes pulse-red {
      0%, 100% { opacity: 1; }
      50% { opacity: 0.7; }
    }

    .container {
      max-width: 1200px;
      margin: 0 auto;
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(300px, 1fr));
      gap: 20px;
    }

    .card {
      background: rgba(255, 255, 255, 0.1);
      border-radius: 15px;
      padding: 20px;
      backdrop-filter: blur(10px);
      border: 1px solid rgba(255, 255, 255, 0.2);
      box-shadow: 0 8px 32px rgba(0, 0, 0, 0.3);
    }

    .card h3 {
      margin-bottom: 15px;
      font-size: 18px;
      text-align: center;
      border-bottom: 2px solid rgba(255, 255, 255, 0.3);
      padding-bottom: 10px;
    }

    .button-grid {
      display: grid;
      grid-template-columns: repeat(3, 1fr);
      gap: 10px;
      margin-top: 15px;
    }

    .button-grid button:nth-child(1) { grid-column: 2; }
    .button-grid button:nth-child(2) { grid-column: 1; grid-row: 2; }
    .button-grid button:nth-child(3) { grid-column: 2; grid-row: 2; }
    .button-grid button:nth-child(4) { grid-column: 3; grid-row: 2; }
    .button-grid button:nth-child(5) { grid-column: 2; grid-row: 3; }

    button {
      padding: 15px;
      border: none;
      border-radius: 10px;
      font-size: 16px;
      font-weight: bold;
      cursor: pointer;
      transition: all 0.3s ease;
      background: rgba(255, 255, 255, 0.2);
      color: white;
      border: 2px solid rgba(255, 255, 255, 0.3);
    }

    button:hover {
      background: rgba(255, 255, 255, 0.3);
      transform: translateY(-2px);
      box-shadow: 0 5px 15px rgba(0, 0, 0, 0.3);
    }

    button:active {
      transform: translateY(0);
    }

    .auto-button {
      width: 100%;
      padding: 15px;
      background: linear-gradient(135deg, #f9ab00 0%, #fbbc04 100%);
      margin-top: 15px;
    }

    .auto-button.active {
      background: linear-gradient(135deg, #0f9d58 0%, #16c172 100%);
      box-shadow: 0 0 20px rgba(15, 157, 88, 0.5);
    }

    .relay-buttons {
      display: grid;
      grid-template-columns: 1fr 1fr;
      gap: 10px;
    }

    .relay-buttons button.on {
      background: linear-gradient(135deg, #56ab2f 0%, #a8e063 100%);
    }

    .relay-buttons button.off {
      background: linear-gradient(135deg, #e53935 0%, #e35d5b 100%);
    }

    .status-display {
      background: rgba(0, 0, 0, 0.3);
      padding: 15px;
      border-radius: 10px;
      text-align: center;
      margin-top: 15px;
      font-size: 16px;
    }

    .status-display strong {
      color: #a8e063;
    }

    .timer-display {
      background: rgba(15, 157, 88, 0.2);
      padding: 15px;
      border-radius: 10px;
      text-align: center;
      margin-top: 15px;
      font-size: 18px;
      border: 2px solid #0f9d58;
    }

    .timer-display.inactive {
      background: rgba(255, 255, 255, 0.1);
      border-color: rgba(255, 255, 255, 0.3);
    }

    .position-timer {
      background: rgba(251, 188, 4, 0.2);
      padding: 15px;
      border-radius: 10px;
      text-align: center;
      margin-top: 10px;
      font-size: 16px;
      border: 2px solid #fbbc04;
    }

    .position-timer.inactive {
      background: rgba(255, 255, 255, 0.1);
      border-color: rgba(255, 255, 255, 0.3);
    }

    .ir-grid {
      display: grid;
      grid-template-columns: 1fr 1fr;
      grid-template-rows: auto auto;
      gap: 10px;
      margin-top: 15px;
    }

    .ir-sensor {
      padding: 12px;
      background: #0f9d58;
      border-radius: 8px;
      font-size: 14px;
      text-align: center;
      transition: all 0.3s;
    }

    .ir-sensor.blocked {
      background: #d93025;
      animation: pulse 1s infinite;
    }

    @keyframes pulse {
      0%, 100% { opacity: 1; }
      50% { opacity: 0.5; }
    }

    .scheduler-input {
      width: 100%;
      padding: 10px;
      border-radius: 8px;
      border: 2px solid rgba(255, 255, 255, 0.3);
      background: rgba(255, 255, 255, 0.1);
      color: white;
      font-size: 14px;
      margin-bottom: 10px;
    }

    .scheduler-input:focus {
      outline: none;
      border-color: #a8e063;
    }

    .schedule-button {
      width: 100%;
      background: linear-gradient(135deg, #f093fb 0%, #f5576c 100%);
      padding: 12px;
    }

    .mode-badge {
      display: inline-block;
      padding: 5px 15px;
      border-radius: 20px;
      font-size: 12px;
      margin-top: 10px;
      background: rgba(255, 255, 255, 0.2);
    }

    .mode-badge.manual {
      background: #fbbc04;
      color: #000;
    }

    .mode-badge.auto {
      background: #0f9d58;
      animation: pulse 2s infinite;
    }

    .mode-badge.emergency {
      background: #d32f2f;
      animation: pulse-red 1s infinite;
    }

    @media (max-width: 768px) {
      .container {
        grid-template-columns: 1fr;
      }
      
      .emergency-section {
        grid-template-columns: 1fr;
      }
      
      .header h1 {
        font-size: 20px;
      }
    }
  </style>

  <script>
    let currentMode = 'manual';
    let emergencyActive = false;

    // Update UV Timer
    async function updateTimer() {
      try {
        const res = await fetch('/timer_status');
        const data = await res.json();
        const display = document.getElementById('timerDisplay');
        const timerDiv = document.getElementById('timerDiv');
        
        if(data.active) {
          timerDiv.className = 'timer-display';
          display.innerHTML = '⏱️ UV Cycle: <strong>' + data.remaining + '</strong>';
        } else {
          timerDiv.className = 'timer-display inactive';
          display.innerHTML = '💡 UV Cycle: <strong>Inactive</strong>';
        }
      } catch(e) {
        console.error('Timer update failed:', e);
      }
    }

    // Update Position Wait Timer
    async function updatePositionTimer() {
      try {
        const res = await fetch('/position_status');
        const data = await res.json();
        const display = document.getElementById('positionDisplay');
        const positionDiv = document.getElementById('positionDiv');
        
        if(data.waiting) {
          positionDiv.className = 'position-timer';
          display.innerHTML = '⏸️ Position Hold: <strong>' + data.remaining + '</strong>';
        } else {
          positionDiv.className = 'position-timer inactive';
          display.innerHTML = '🚶 Status: <strong>Moving</strong>';
        }
      } catch(e) {
        console.error('Position timer update failed:', e);
      }
    }

    setInterval(updateTimer, 1000);
    setInterval(updatePositionTimer, 1000);

    // Update IR Sensor status
    async function updateIR() {
      try {
        const res = await fetch('/ir_status');
        const data = await res.json();
        
        document.getElementById('irFL').className = 'ir-sensor' + (data.frontLeft ? ' blocked' : '');
        document.getElementById('irFR').className = 'ir-sensor' + (data.frontRight ? ' blocked' : '');
        document.getElementById('irBL').className = 'ir-sensor' + (data.backLeft ? ' blocked' : '');
        document.getElementById('irBR').className = 'ir-sensor' + (data.backRight ? ' blocked' : '');
        
        let status = 'All Clear ✓';
        if(data.frontLeft || data.frontRight || data.backLeft || data.backRight) {
          status = 'Obstacle Detected! ⚠️';
        }
        document.getElementById('irMainStatus').innerText = status;
      } catch(e) {
        console.error('IR update failed:', e);
      }
    }
    setInterval(updateIR, 500);

    // Emergency Stop
    async function emergencyStop() {
      if(!confirm('🚨 ACTIVATE EMERGENCY STOP?\n\nThis will:\n• Stop all motors immediately\n• Turn off all relays\n• Disable auto mode\n\nAre you sure?')) {
        return;
      }
      
      try {
        const res = await fetch('/emergency_stop');
        const status = await res.text();
        emergencyActive = true;
        currentMode = 'emergency';
        
        const badge = document.getElementById('modeBadge');
        badge.className = 'mode-badge emergency';
        badge.innerText = 'EMERGENCY STOP';
        
        const autoBtn = document.getElementById('autoBtn');
        autoBtn.disabled = true;
        autoBtn.style.opacity = '0.5';
        
        alert('🚨 EMERGENCY STOP ACTIVATED!\n\nAll systems stopped.\nClick "Reset System" to resume operation.');
      } catch(e) {
        alert('❌ Emergency stop failed: ' + e.message);
      }
    }

    // Reset Emergency
    async function resetEmergency() {
      try {
        const res = await fetch('/reset_emergency');
        const status = await res.text();
        emergencyActive = false;
        currentMode = 'manual';
        
        const badge = document.getElementById('modeBadge');
        badge.className = 'mode-badge manual';
        badge.innerText = 'MANUAL MODE';
        
        const autoBtn = document.getElementById('autoBtn');
        autoBtn.disabled = false;
        autoBtn.style.opacity = '1';
        autoBtn.classList.remove('active');
        autoBtn.innerText = '🎮 Auto Sterilization: OFF';
        
        alert('✓ System Reset Complete\nReady for operation');
      } catch(e) {
        alert('❌ Reset failed: ' + e.message);
      }
    }

    // Toggle automatic mode
    async function toggleAutoMode() {
      if(emergencyActive) {
        alert('⚠️ Cannot start auto mode\nEmergency stop is active. Reset first.');
        return;
      }
      
      try {
        const res = await fetch('/toggle_auto');
        const status = await res.text();
        const btn = document.getElementById('autoBtn');
        const badge = document.getElementById('modeBadge');
        
        if(status.includes('ON')) {
          currentMode = 'auto';
          btn.classList.add('active');
          btn.innerText = '🤖 Auto Sterilization: ON';
          badge.className = 'mode-badge auto';
          badge.innerText = 'AUTO MODE';
          alert('✓ Automatic Mode ON\n\n• 30-min UV sterilization cycle started\n• Robot stops 5 min when hitting walls\n• Auto-navigates after wait time\n• Relay 2 activates after completion\n• Use EMERGENCY STOP to halt immediately');
        } else {
          currentMode = 'manual';
          btn.classList.remove('active');
          btn.innerText = '🎮 Auto Sterilization: OFF';
          badge.className = 'mode-badge manual';
          badge.innerText = 'MANUAL MODE';
          alert('✓ Manual Mode ON\nFull control - All relays manual');
        }
      } catch(e) {
        alert('❌ Failed to toggle mode: ' + e.message);
      }
    }

    // Movement controls
    function sendCommand(cmd) {
      if(emergencyActive && cmd !== 'stop') {
        alert('⚠️ Emergency stop active!\nReset system first.');
        return;
      }
      fetch('/' + cmd).catch(e => console.error('Command failed:', e));
    }

    // Schedule UV light
    async function setSchedule() {
      if(emergencyActive) {
        alert('⚠️ Cannot schedule\nEmergency stop is active.');
        return;
      }
      
      const datetime = document.getElementById('datetime').value;
      if (!datetime) {
        alert('⚠️ Please select date and time');
        return;
      }
      try {
        const res = await fetch('/setSchedule?datetime=' + encodeURIComponent(datetime));
        const msg = await res.text();
        alert('✓ ' + msg);
      } catch(e) {
        alert('❌ Schedule failed: ' + e.message);
      }
    }

    // Initialize on page load
    document.addEventListener('DOMContentLoaded', function() {
      updateTimer();
      updatePositionTimer();
      updateIR();
    });
  </script>
</head>

<body>
  <div class="header">
    <h1>🤖 LAVABOT Controller</h1>
    <p>IoT-Based Automatic Lavatory Sanitation Device</p>
    <span id="modeBadge" class="mode-badge manual">MANUAL MODE</span>
  </div>

  <!-- Emergency Controls -->
  <div class="emergency-section">
    <button class="emergency-button emergency-stop" onclick="emergencyStop()">
      🚨 EMERGENCY STOP
    </button>
    <button class="emergency-button emergency-reset" onclick="resetEmergency()">
      ✓ RESET SYSTEM
    </button>
  </div>

  <div class="container">
    <!-- Movement Control -->
    <div class="card">
      <h3>🎮 Movement Control</h3>
      <div class="button-grid">
        <button onclick="sendCommand('forward')">▲<br>Forward</button>
        <button onclick="sendCommand('left')">◄<br>Left</button>
        <button onclick="sendCommand('stop')">■<br>Stop</button>
        <button onclick="sendCommand('right')">►<br>Right</button>
        <button onclick="sendCommand('backward')">▼<br>Backward</button>
      </div>
      <div class="status-display">
        Speed: <strong>Fixed (60%)</strong>
      </div>
      <button id="autoBtn" class="auto-button" onclick="toggleAutoMode()">
        🎮 Auto Sterilization: OFF
      </button>
    </div>

    <!-- UV Sterilization Timer -->
    <div class="card">
      <h3>⏱️ UV Sterilization Timer</h3>
      <div id="timerDiv" class="timer-display inactive">
        <span id="timerDisplay">💡 UV Cycle: <strong>Inactive</strong></span>
      </div>
      <div id="positionDiv" class="position-timer inactive">
        <span id="positionDisplay">🚶 Status: <strong>Moving</strong></span>
      </div>
      <p style="margin-top: 15px; font-size: 13px; opacity: 0.8; text-align: center;">
        ℹ️ Auto Mode: 30-min UV + 5-min position holds<br>
        ℹ️ Relay 2 activates after UV completion
      </p>
    </div>

    <!-- IR Collision Sensors -->
    <div class="card">
      <h3>🚧 Collision Detection</h3>
      <div class="status-display" id="irMainStatus">All Clear ✓</div>
      <div class="ir-grid">
        <div id="irFL" class="ir-sensor">Front Left ↖</div>
        <div id="irFR" class="ir-sensor">Front Right ↗</div>
        <div id="irBL" class="ir-sensor">Back Left ↙</div>
        <div id="irBR" class="ir-sensor">Back Right ↘</div>
      </div>
      <p style="margin-top: 15px; font-size: 13px; opacity: 0.8; text-align: center;">
        ℹ️ AUTO mode: Stops 5 min when hitting walls<br>
        ℹ️ MANUAL mode: Display only
      </p>
    </div>

    <!-- UV Light Control -->
    <div class="card">
      <h3>💡 UV Light (Relay 1)</h3>
      <div class="relay-buttons">
        <button class="on" onclick="sendCommand('relay1_on')">ON</button>
        <button class="off" onclick="sendCommand('relay1_off')">OFF</button>
      </div>
      <p style="margin-top: 15px; font-size: 12px; opacity: 0.8; text-align: center;">
        Manual control (Auto mode overrides)
      </p>
    </div>

    <!-- Relay 2 Control -->
    <div class="card">
      <h3>🔌 Relay 2</h3>
      <div class="relay-buttons">
        <button class="on" onclick="sendCommand('relay2_on')">ON</button>
        <button class="off" onclick="sendCommand('relay2_off')">OFF</button>
      </div>
      <p style="margin-top: 15px; font-size: 12px; opacity: 0.8; text-align: center;">
        Auto-activates after UV cycle
      </p>
    </div>

    <!-- Scheduler -->
    <div class="card">
      <h3>⏰ Auto-Start Scheduler</h3>
      <input type="datetime-local" id="datetime" class="scheduler-input">
      <button class="schedule-button" onclick="setSchedule()">Set Schedule</button>
      <p style="margin-top: 10px; font-size: 12px; opacity: 0.8; text-align: center;">
        Auto mode will start at scheduled time
      </p>
    </div>
  </div>

</body>
</html>
)rawliteral";
  return html;
}
// Track current motor state
enum MotorState { STOPPED, FORWARD, BACKWARD, LEFT, RIGHT };
MotorState currentState = STOPPED;

// Forward declarations
bool checkObstacle(int sensorPin);

// ---------- IR Sensor Functions ----------
bool checkObstacle(int sensorPin) {
  return digitalRead(sensorPin) == LOW;
}

void handleIRStatus() {
  String json = "{";
  json += "\"frontLeft\":" + String(checkObstacle(IR_FRONT_LEFT)) + ",";
  json += "\"frontRight\":" + String(checkObstacle(IR_FRONT_RIGHT)) + ",";
  json += "\"backLeft\":" + String(checkObstacle(IR_BACK_LEFT)) + ",";
  json += "\"backRight\":" + String(checkObstacle(IR_BACK_RIGHT));
  json += "}";
  server.send(200, "application/json", json);
}

// ---------- Position Wait Timer Status ----------
void handlePositionStatus() {
  String json = "{";
  if(waitingAtPosition) {
    unsigned long elapsed = millis() - positionStartTime;
    unsigned long remaining = POSITION_WAIT_TIME - elapsed;
    int minutes = remaining / 60000;
    int seconds = (remaining % 60000) / 1000;
    
    char timeStr[20];
    sprintf(timeStr, "%02d:%02d", minutes, seconds);
    
    json += "\"waiting\":true,";
    json += "\"remaining\":\"" + String(timeStr) + "\"";
  } else {
    json += "\"waiting\":false,";
    json += "\"remaining\":\"--:--\"";
  }
  json += "}";
  server.send(200, "application/json", json);
}

// ---------- UV Timer Status ----------
void handleTimerStatus() {
  String json = "{";
  if(autoMode && !uvCycleComplete) {
    unsigned long elapsed = millis() - uvStartTime;
    unsigned long remaining = UV_DURATION - elapsed;
    int minutes = remaining / 60000;
    int seconds = (remaining % 60000) / 1000;
    
    char timeStr[20];
    sprintf(timeStr, "%02d:%02d", minutes, seconds);
    
    json += "\"active\":true,";
    json += "\"remaining\":\"" + String(timeStr) + "\"";
  } else {
    json += "\"active\":false,";
    json += "\"remaining\":\"--:--\"";
  }
  json += "}";
  server.send(200, "application/json", json);
}

// ---------- Emergency Stop Handler ----------
void handleEmergencyStop() {
  emergencyStop = true;
  autoMode = false;
  stopMotors();
  digitalWrite(RELAY1_PIN, LOW);   // Turn OFF UV lamp
  digitalWrite(RELAY2_PIN, LOW);   // Turn OFF Relay 2
  waitingAtPosition = false;
  uvCycleComplete = false;
  
  Serial.println("\n🚨 EMERGENCY STOP ACTIVATED!");
  Serial.println("  → All motors stopped");
  Serial.println("  → All relays OFF");
  Serial.println("  → Auto mode disabled");
  
  server.send(200, "text/plain", "Emergency Stop Activated");
}

// ---------- Reset Emergency Stop ----------
void handleResetEmergency() {
  emergencyStop = false;
  Serial.println("✓ Emergency stop reset - System ready");
  server.send(200, "text/plain", "System Reset");
}

// ---------- Toggle Auto Mode ----------
void handleToggleAuto() {
  if(emergencyStop) {
    server.send(400, "text/plain", "Cannot start - Emergency stop active. Reset first.");
    return;
  }
  
  autoMode = !autoMode;
  
  if(autoMode) {
    // Start UV sterilization cycle
    uvStartTime = millis();
    uvCycleComplete = false;
    waitingAtPosition = false;
    digitalWrite(RELAY1_PIN, HIGH);  // Turn ON UV lamp
    digitalWrite(RELAY2_PIN, LOW);   // Ensure Relay 2 is OFF
    Serial.println("✓ Automatic Mode ON");
    Serial.println("  → UV Lamp ON (30-minute cycle started)");
    Serial.println("  → IR collision detection active");
    Serial.println("  → 5-minute position wait enabled");
    server.send(200, "text/plain", "Automatic Mode ON");
  } else {
    // Stop auto mode
    stopMotors();
    digitalWrite(RELAY1_PIN, LOW);   // Turn OFF UV lamp
    digitalWrite(RELAY2_PIN, LOW);   // Turn OFF Relay 2
    uvCycleComplete = false;
    waitingAtPosition = false;
    Serial.println("✓ Manual Mode ON");
    Serial.println("  → All relays OFF");
    Serial.println("  → Manual control enabled");
    server.send(200, "text/plain", "Manual Mode ON");
  }
}

// ---------- Motor Control Functions ----------
void stopMotors() {
  analogWrite(LPWM_L, 0);
  analogWrite(RPWM_L, 0);
  analogWrite(LPWM_R, 0);
  analogWrite(RPWM_R, 0);
  currentState = STOPPED;
}

void forwardMotors() {
  if(emergencyStop) return;
  
  if(autoMode && (checkObstacle(IR_FRONT_LEFT) || checkObstacle(IR_FRONT_RIGHT))) {
    stopMotors();
    if(!waitingAtPosition) {
      waitingAtPosition = true;
      positionStartTime = millis();
      Serial.println("⚠️ [AUTO] Front obstacle - stopping for 5 minutes");
    }
    return;
  }
  
  analogWrite(LPWM_L, MOTOR_SPEED);
  analogWrite(RPWM_L, 0);
  analogWrite(LPWM_R, MOTOR_SPEED);
  analogWrite(RPWM_R, 0);
  currentState = FORWARD;
}

void backwardMotors() {
  if(emergencyStop) return;
  
  if(autoMode && (checkObstacle(IR_BACK_LEFT) || checkObstacle(IR_BACK_RIGHT))) {
    stopMotors();
    if(!waitingAtPosition) {
      waitingAtPosition = true;
      positionStartTime = millis();
      Serial.println("⚠️ [AUTO] Back obstacle - stopping for 5 minutes");
    }
    return;
  }
  
  analogWrite(LPWM_L, 0);
  analogWrite(RPWM_L, MOTOR_SPEED);
  analogWrite(LPWM_R, 0);
  analogWrite(RPWM_R, MOTOR_SPEED);
  currentState = BACKWARD;
}

void leftMotors() {
  if(emergencyStop) return;
  
  if(autoMode && (checkObstacle(IR_FRONT_LEFT) || checkObstacle(IR_BACK_LEFT))) {
    stopMotors();
    if(!waitingAtPosition) {
      waitingAtPosition = true;
      positionStartTime = millis();
      Serial.println("⚠️ [AUTO] Left obstacle - stopping for 5 minutes");
    }
    return;
  }
  
  analogWrite(LPWM_L, 0);
  analogWrite(RPWM_L, MOTOR_SPEED);
  analogWrite(LPWM_R, MOTOR_SPEED);
  analogWrite(RPWM_R, 0);
  currentState = LEFT;
}

void rightMotors() {
  if(emergencyStop) return;
  
  if(autoMode && (checkObstacle(IR_FRONT_RIGHT) || checkObstacle(IR_BACK_RIGHT))) {
    stopMotors();
    if(!waitingAtPosition) {
      waitingAtPosition = true;
      positionStartTime = millis();
      Serial.println("⚠️ [AUTO] Right obstacle - stopping for 5 minutes");
    }
    return;
  }
  
  analogWrite(LPWM_L, MOTOR_SPEED);
  analogWrite(RPWM_L, 0);
  analogWrite(LPWM_R, 0);
  analogWrite(RPWM_R, MOTOR_SPEED);
  currentState = RIGHT;
}

// ---------- Automatic Navigation ----------
void autoNavigate() {
  if(!autoMode || emergencyStop) return;

  // Check if waiting at position
  if(waitingAtPosition) {
    unsigned long elapsed = millis() - positionStartTime;
    if(elapsed >= POSITION_WAIT_TIME) {
      // 5 minutes passed, ready to move again
      waitingAtPosition = false;
      Serial.println("✓ Position wait complete - resuming navigation");
    } else {
      // Still waiting, don't move
      return;
    }
  }

  bool frontLeft = checkObstacle(IR_FRONT_LEFT);
  bool frontRight = checkObstacle(IR_FRONT_RIGHT);
  bool backLeft = checkObstacle(IR_BACK_LEFT);
  bool backRight = checkObstacle(IR_BACK_RIGHT);

  // If any obstacle detected, stop and start wait timer
  if(frontLeft || frontRight || backLeft || backRight) {
    stopMotors();
    if(!waitingAtPosition) {
      waitingAtPosition = true;
      positionStartTime = millis();
      Serial.println("🛑 Obstacle detected - Waiting 5 minutes at this position");
    }
    return;
  }

  // No obstacles, move forward
  if(currentState != FORWARD) {
    forwardMotors();
  }
}

// ---------- Relay Control ----------
void relay1On() {
  if(emergencyStop) {
    server.send(400, "text/plain", "Emergency stop active");
    return;
  }
  digitalWrite(RELAY1_PIN, HIGH);
  server.send(200, "text/plain", "UV Light ON");
  Serial.println("Relay 1 (UV Light) - ON");
}

void relay1Off() {
  digitalWrite(RELAY1_PIN, LOW);
  server.send(200, "text/plain", "UV Light OFF");
  Serial.println("Relay 1 (UV Light) - OFF");
}

void relay2On() {
  if(emergencyStop) {
    server.send(400, "text/plain", "Emergency stop active");
    return;
  }
  digitalWrite(RELAY2_PIN, HIGH);
  server.send(200, "text/plain", "Relay 2 ON");
  Serial.println("Relay 2 - ON");
}

void relay2Off() {
  digitalWrite(RELAY2_PIN, LOW);
  server.send(200, "text/plain", "Relay 2 OFF");
  Serial.println("Relay 2 - OFF");
}

// ---------- Scheduler ----------
void handleSetSchedule() {
  if (!server.hasArg("datetime")) {
    server.send(400, "text/plain", "Missing datetime parameter");
    return;
  }
  
  if(emergencyStop) {
    server.send(400, "text/plain", "Cannot schedule - Emergency stop active");
    return;
  }
  
  String dt = server.arg("datetime");
  struct tm tm;
  strptime(dt.c_str(), "%Y-%m-%dT%H:%M", &tm);
  scheduledTime = mktime(&tm);
  scheduleSet = true;

  server.send(200, "text/plain", "Auto mode scheduled for " + dt);
  Serial.println("Auto sterilization scheduled for: " + dt);
}

// ---------- Setup ----------
void setup() {
  Serial.begin(9600);
  delay(1000);
  
  Serial.println("\n╔════════════════════════════════════╗");
  Serial.println("║     LAVABOT INITIALIZING...        ║");
  Serial.println("╚════════════════════════════════════╝\n");

  pinMode(LPWM_L, OUTPUT);
  pinMode(RPWM_L, OUTPUT);
  pinMode(LPWM_R, OUTPUT);
  pinMode(RPWM_R, OUTPUT);
  stopMotors();
  Serial.println("✓ Motors initialized (Speed: " + String(MOTOR_SPEED) + ")");

  pinMode(RELAY1_PIN, OUTPUT);
  pinMode(RELAY2_PIN, OUTPUT);
  digitalWrite(RELAY1_PIN, LOW);
  digitalWrite(RELAY2_PIN, LOW);
  Serial.println("✓ Relays initialized");

  pinMode(IR_FRONT_LEFT, INPUT);
  pinMode(IR_FRONT_RIGHT, INPUT);
  pinMode(IR_BACK_LEFT, INPUT);
  pinMode(IR_BACK_RIGHT, INPUT);
  Serial.println("✓ IR sensors initialized");

  Serial.print("\nConnecting to WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n✓ WiFi connected!");
  Serial.print("📡 IP Address: ");
  Serial.println(WiFi.localIP());

  configTime(8 * 3600, 0, "pool.ntp.org");
  Serial.println("✓ Time synchronized");

  server.on("/", []() { server.send(200, "text/html", htmlPage()); });
  server.on("/forward", []() { forwardMotors(); server.send(200, "text/plain", "Moving Forward"); });
  server.on("/backward", []() { backwardMotors(); server.send(200, "text/plain", "Moving Backward"); });
  server.on("/left", []() { leftMotors(); server.send(200, "text/plain", "Turning Left"); });
  server.on("/right", []() { rightMotors(); server.send(200, "text/plain", "Turning Right"); });
  server.on("/stop", []() { stopMotors(); server.send(200, "text/plain", "Stopped"); });
  server.on("/relay1_on", relay1On);
  server.on("/relay1_off", relay1Off);
  server.on("/relay2_on", relay2On);
  server.on("/relay2_off", relay2Off);
  server.on("/ir_status", handleIRStatus);
  server.on("/timer_status", handleTimerStatus);
  server.on("/position_status", handlePositionStatus);
  server.on("/toggle_auto", handleToggleAuto);
  server.on("/emergency_stop", handleEmergencyStop);
  server.on("/reset_emergency", handleResetEmergency);
  server.on("/setSchedule", handleSetSchedule);

  server.begin();
  Serial.println("✓ Web server started");
  
  Serial.println("\n╔════════════════════════════════════╗");
  Serial.println("║     LAVABOT READY!                 ║");
  Serial.println("╚════════════════════════════════════╝");
  Serial.print("\n🌐 Open browser: http://");
  Serial.println(WiFi.localIP());
  Serial.println();
}

// ---------- Main Loop ----------
void loop() {
  server.handleClient();

  // Don't do anything if emergency stop is active
  if(emergencyStop) {
    return;
  }

  // Auto mode logic
  if(autoMode) {
    // Check if 30-minute UV cycle is complete
    if(!uvCycleComplete && (millis() - uvStartTime >= UV_DURATION)) {
      digitalWrite(RELAY1_PIN, LOW);   // Turn OFF UV lamp
      digitalWrite(RELAY2_PIN, HIGH);  // Turn ON Relay 2
      uvCycleComplete = true;
      stopMotors();
      waitingAtPosition = false;
      Serial.println("\n✓ UV Sterilization Complete!");
      Serial.println("  → UV Lamp OFF");
      Serial.println("  → Relay 2 ON");
      Serial.println("  → Motors stopped");
    }
    
    // Continue navigation if cycle not complete
    if(!uvCycleComplete) {
      autoNavigate();
      delay(200);
    }
  }

  // Check scheduled task
  if (scheduleSet && time(nullptr) >= scheduledTime && !emergencyStop) {
    Serial.println("⏰ Scheduled time reached! Starting auto sterilization...");
    autoMode = true;
    uvStartTime = millis();
    uvCycleComplete = false;
    waitingAtPosition = false;
    digitalWrite(RELAY1_PIN, HIGH);
    digitalWrite(RELAY2_PIN, LOW);
    scheduleSet = false;
  }
}