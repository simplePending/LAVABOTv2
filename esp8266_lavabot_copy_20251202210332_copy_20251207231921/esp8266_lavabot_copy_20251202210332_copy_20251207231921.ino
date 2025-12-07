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

// ---------- IR Sensors (20cm Wall Detection) ----------
#define IR_FRONT_LEFT   3   // RX (GPIO3)
#define IR_FRONT_RIGHT  1   // TX (GPIO1)
#define IR_BACK_LEFT    9   // SD2 (GPIO9)
#define IR_BACK_RIGHT   10  // SD3 (GPIO10)

// ---------- Timing Variables ----------
const int MOTOR_SPEED = 150;  // Fixed speed (0-255)
const unsigned long UV_DURATION = 30 * 60 * 1000;  // 30 minutes
const unsigned long MOVE_DURATION = 5 * 1000;      // 5 seconds of movement
const unsigned long POSITION_WAIT_TIME = 20 * 60 * 1000;  // 20 minutes wait time

time_t scheduledTime = 0;
bool scheduleSet = false;
bool autoMode = false;
unsigned long uvStartTime = 0;
bool uvCycleComplete = false;

// Movement cycle tracking
unsigned long moveStartTime = 0;
unsigned long positionStartTime = 0;
bool isMoving = false;
bool waitingAtPosition = false;
bool emergencyStop = false;
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

    .movement-status {
      background: rgba(251, 188, 4, 0.2);
      padding: 15px;
      border-radius: 10px;
      text-align: center;
      margin-top: 10px;
      font-size: 16px;
      border: 2px solid #fbbc04;
    }

    .movement-status.moving {
      background: rgba(15, 157, 88, 0.2);
      border-color: #0f9d58;
      animation: pulse 1.5s infinite;
    }

    .movement-status.stopped {
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

    .cycle-info {
      background: rgba(15, 157, 88, 0.15);
      padding: 12px;
      border-radius: 8px;
      margin-top: 10px;
      font-size: 13px;
      line-height: 1.6;
      border-left: 4px solid #0f9d58;
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

    // Update Movement Status (5sec move / 20min wait)
    async function updateMovementStatus() {
      try {
        const res = await fetch('/movement_status');
        const data = await res.json();
        const display = document.getElementById('movementDisplay');
        const movementDiv = document.getElementById('movementDiv');
        
        if(data.status === 'moving') {
          movementDiv.className = 'movement-status moving';
          display.innerHTML = '🚀 Moving: <strong>' + data.remaining + '</strong>';
        } else if(data.status === 'waiting') {
          movementDiv.className = 'movement-status';
          display.innerHTML = '⏸️ Waiting: <strong>' + data.remaining + '</strong>';
        } else {
          movementDiv.className = 'movement-status stopped';
          display.innerHTML = '⏹️ Status: <strong>Stopped</strong>';
        }
      } catch(e) {
        console.error('Movement update failed:', e);
      }
    }

    setInterval(updateTimer, 1000);
    setInterval(updateMovementStatus, 500);

    // Update IR Sensor status
    async function updateIR() {
      try {
        const res = await fetch('/ir_status');
        const data = await res.json();
        
        document.getElementById('irFL').className = 'ir-sensor' + (data.frontLeft ? ' blocked' : '');
        document.getElementById('irFR').className = 'ir-sensor' + (data.frontRight ? ' blocked' : '');
        document.getElementById('irBL').className = 'ir-sensor' + (data.backLeft ? ' blocked' : '');
        document.getElementById('irBR').className = 'ir-sensor' + (data.backRight ? ' blocked' : '');
        
        let status = 'All Clear ✓ (20cm range)';
        if(data.frontLeft || data.frontRight || data.backLeft || data.backRight) {
          status = 'Wall Detected! ⚠️ (~20cm)';
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
          alert('✓ Automatic Mode ON\n\n📋 Movement Cycle:\n• Move 5 seconds\n• Stop & sterilize 20 minutes\n• Auto wall avoidance (20cm)\n• Repeat for 30 minutes\n• Relay 2 activates when complete\n\n🚨 Use EMERGENCY STOP to halt');
        } else {
          currentMode = 'manual';
          btn.classList.remove('active');
          btn.innerText = '🎮 Auto Sterilization: OFF';
          badge.className = 'mode-badge manual';
          badge.innerText = 'MANUAL MODE';
          alert('✓ Manual Mode ON\nFull manual control enabled');
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
      updateMovementStatus();
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

    <!-- Movement Cycle Status -->
    <div class="card">
      <h3>🔄 Movement Cycle</h3>
      <div id="movementDiv" class="movement-status stopped">
        <span id="movementDisplay">⏹️ Status: <strong>Stopped</strong></span>
      </div>
      <div class="cycle-info">
        <strong>🔁 Auto Cycle Pattern:</strong><br>
        1️⃣ Move for 5 seconds<br>
        2️⃣ Stop and sterilize for 20 minutes<br>
        3️⃣ Repeat until UV cycle completes<br>
        <br>
        <strong>🛡️ Wall Detection:</strong> Auto-change direction at ~20cm
      </div>
    </div>

    <!-- UV Sterilization Timer -->
    <div class="card">
      <h3>⏱️ UV Sterilization Timer</h3>
      <div id="timerDiv" class="timer-display inactive">
        <span id="timerDisplay">💡 UV Cycle: <strong>Inactive</strong></span>
      </div>
      <p style="margin-top: 15px; font-size: 13px; opacity: 0.8; text-align: center;">
        ℹ️ 30-minute UV sterilization cycle<br>
        ℹ️ Relay 2 activates after completion
      </p>
    </div>

    <!-- IR Wall Detection -->
    <div class="card">
      <h3>🚧 Wall Detection (~20cm)</h3>
      <div class="status-display" id="irMainStatus">All Clear ✓ (20cm range)</div>
      <div class="ir-grid">
        <div id="irFL" class="ir-sensor">Front Left ↖</div>
        <div id="irFR" class="ir-sensor">Front Right ↗</div>
        <div id="irBL" class="ir-sensor">Back Left ↙</div>
        <div id="irBR" class="ir-sensor">Back Right ↘</div>
      </div>
      <p style="margin-top: 15px; font-size: 13px; opacity: 0.8; text-align: center;">
        ℹ️ AUTO mode: Changes direction when wall detected<br>
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
MotorState nextDirection = FORWARD;  // Track which direction to try next

// Forward declarations
bool checkObstacle(int sensorPin);

// ---------- IR Sensor Functions ----------
bool checkObstacle(int sensorPin) {
  return digitalRead(sensorPin) == LOW;  // LOW = obstacle detected at ~20cm
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

// ---------- Movement Timer Status ----------
void handleMovementStatus() {
  String json = "{";
  if(isMoving) {
    unsigned long elapsed = millis() - moveStartTime;
    unsigned long remaining = MOVE_DURATION - elapsed;
    int seconds = remaining / 1000;
    json += "\"status\":\"moving\",";
    json += "\"remaining\":\"" + String(seconds) + "s\"";
  } else if(waitingAtPosition) {
    unsigned long elapsed = millis() - positionStartTime;
    unsigned long remaining = POSITION_WAIT_TIME - elapsed;
    int minutes = remaining / 60000;
    int seconds = (remaining % 60000) / 1000;
    char timeStr[20];
    sprintf(timeStr, "%02d:%02d", minutes, seconds);
    json += "\"status\":\"waiting\",";
    json += "\"remaining\":\"" + String(timeStr) + "\"";
  } else {
    json += "\"status\":\"stopped\",";
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
  digitalWrite(RELAY1_PIN, LOW);
  digitalWrite(RELAY2_PIN, LOW);
  isMoving = false;
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
    uvStartTime = millis();
    uvCycleComplete = false;
    isMoving = false;
    waitingAtPosition = false;
    nextDirection = FORWARD;
    digitalWrite(RELAY1_PIN, HIGH);
    digitalWrite(RELAY2_PIN, LOW);
    Serial.println("✓ Automatic Mode ON");
    Serial.println("  → UV Lamp ON (30-minute cycle)");
    Serial.println("  → Movement: 5 sec move, 20 min wait");
    Serial.println("  → Wall avoidance: Active");
    server.send(200, "text/plain", "Automatic Mode ON");
  } else {
    stopMotors();
    digitalWrite(RELAY1_PIN, LOW);
    digitalWrite(RELAY2_PIN, LOW);
    isMoving = false;
    waitingAtPosition = false;
    uvCycleComplete = false;
    Serial.println("✓ Manual Mode ON");
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
  isMoving = false;
}

void forwardMotors() {
  if(emergencyStop) return;
  analogWrite(LPWM_L, MOTOR_SPEED);
  analogWrite(RPWM_L, 0);
  analogWrite(LPWM_R, MOTOR_SPEED);
  analogWrite(RPWM_R, 0);
  currentState = FORWARD;
}

void backwardMotors() {
  if(emergencyStop) return;
  analogWrite(LPWM_L, 0);
  analogWrite(RPWM_L, MOTOR_SPEED);
  analogWrite(LPWM_R, 0);
  analogWrite(RPWM_R, MOTOR_SPEED);
  currentState = BACKWARD;
}

void leftMotors() {
  if(emergencyStop) return;
  analogWrite(LPWM_L, 0);
  analogWrite(RPWM_L, MOTOR_SPEED);
  analogWrite(LPWM_R, MOTOR_SPEED);
  analogWrite(RPWM_R, 0);
  currentState = LEFT;
}

void rightMotors() {
  if(emergencyStop) return;
  analogWrite(LPWM_L, MOTOR_SPEED);
  analogWrite(RPWM_L, 0);
  analogWrite(LPWM_R, 0);
  analogWrite(RPWM_R, MOTOR_SPEED);
  currentState = RIGHT;
}

// ---------- Find Safe Direction ----------
MotorState findSafeDirection() {
  bool frontLeft = checkObstacle(IR_FRONT_LEFT);
  bool frontRight = checkObstacle(IR_FRONT_RIGHT);
  bool backLeft = checkObstacle(IR_BACK_LEFT);
  bool backRight = checkObstacle(IR_BACK_RIGHT);

  // Priority: Try directions with no obstacles
  if(!frontLeft && !frontRight) {
    Serial.println("  → Safe direction: FORWARD");
    return FORWARD;
  }
  if(!backLeft && !backRight) {
    Serial.println("  → Safe direction: BACKWARD");
    return BACKWARD;
  }
  if(!frontLeft && !backLeft) {
    Serial.println("  → Safe direction: LEFT");
    return LEFT;
  }
  if(!frontRight && !backRight) {
    Serial.println("  → Safe direction: RIGHT");
    return RIGHT;
  }
  
  // If all blocked (corner situation), try turning
  Serial.println("  → All sides blocked, trying RIGHT turn");
  return RIGHT;
}

// ---------- Automatic Navigation with 5sec Move / 20min Wait ----------
void autoNavigate() {
  if(!autoMode || emergencyStop) return;

  // STATE 1: Currently moving for 5 seconds
  if(isMoving) {
    unsigned long elapsed = millis() - moveStartTime;
    
    // Check for wall collision during movement
    bool wallDetected = false;
    String wallLocation = "";
    
    if(currentState == FORWARD && (checkObstacle(IR_FRONT_LEFT) || checkObstacle(IR_FRONT_RIGHT))) {
      wallDetected = true;
      wallLocation = "FRONT";
    }
    else if(currentState == BACKWARD && (checkObstacle(IR_BACK_LEFT) || checkObstacle(IR_BACK_RIGHT))) {
      wallDetected = true;
      wallLocation = "BACK";
    }
    else if(currentState == LEFT && (checkObstacle(IR_FRONT_LEFT) || checkObstacle(IR_BACK_LEFT))) {
      wallDetected = true;
      wallLocation = "LEFT";
    }
    else if(currentState == RIGHT && (checkObstacle(IR_FRONT_RIGHT) || checkObstacle(IR_BACK_RIGHT))) {
      wallDetected = true;
      wallLocation = "RIGHT";
    }
    
    // Wall detected! Change direction immediately
    if(wallDetected) {
      Serial.println("⚠️ Wall detected at " + wallLocation + " (~20cm) - Changing direction!");
      stopMotors();
      delay(300);  // Brief pause
      
      // Find new safe direction
      nextDirection = findSafeDirection();
      
      // Start moving in new direction
      switch(nextDirection) {
        case FORWARD: forwardMotors(); Serial.println("  → Moving FORWARD"); break;
        case BACKWARD: backwardMotors(); Serial.println("  → Moving BACKWARD"); break;
        case LEFT: leftMotors(); Serial.println("  → Moving LEFT"); break;
        case RIGHT: rightMotors(); Serial.println("  → Moving RIGHT"); break;
        default: stopMotors(); break;
      }
      
      // Reset move timer to continue for remaining time
      moveStartTime = millis();
      return;
    }
    
    // Check if 5 seconds elapsed
    if(elapsed >= MOVE_DURATION) {
      // Stop and start waiting
      stopMotors();
      isMoving = false;
      waitingAtPosition = true;
      positionStartTime = millis();
      Serial.println("✓ 5 seconds movement complete");
      Serial.println("⏸️ Waiting 20 minutes at this position...");
    }
    return;
  }

  // STATE 2: Waiting at position for 20 minutes
  if(waitingAtPosition) {
    unsigned long elapsed = millis() - positionStartTime;
    if(elapsed >= POSITION_WAIT_TIME) {
      // 20 minutes passed, ready to move again
      waitingAtPosition = false;
      Serial.println("✓ 20-minute wait complete");
      Serial.println("🚀 Finding safe direction to move...");
      
      // Find safe direction before moving
      nextDirection = findSafeDirection();
      
      // Start new 5-second movement cycle
      isMoving = true;
      moveStartTime = millis();
      
      switch(nextDirection) {
        case FORWARD: forwardMotors(); Serial.println("  → Moving FORWARD for 5 sec"); break;
        case BACKWARD: backwardMotors(); Serial.println("  → Moving BACKWARD for 5 sec"); break;
        case LEFT: leftMotors(); Serial.println("  → Moving LEFT for 5 sec"); break;
        case RIGHT: rightMotors(); Serial.println("  → Moving RIGHT for 5 sec"); break;
        default: stopMotors(); break;
      }
    }
    return;
  }

  // STATE 3: Not moving or waiting - start first movement
  if(!isMoving && !waitingAtPosition) {
    Serial.println("🚀 Starting movement cycle");
    nextDirection = findSafeDirection();
    isMoving = true;
    moveStartTime = millis();
    
    switch(nextDirection) {
      case FORWARD: forwardMotors(); Serial.println("  → Moving FORWARD for 5 sec"); break;
      case BACKWARD: backwardMotors(); Serial.println("  → Moving BACKWARD for 5 sec"); break;
      case LEFT: leftMotors(); Serial.println("  → Moving LEFT for 5 sec"); break;
      case RIGHT: rightMotors(); Serial.println("  → Moving RIGHT for 5 sec"); break;
      default: stopMotors(); break;
    }
  }
}

// ---------- Manual Control Handlers ----------
void handleForward() {
  if(emergencyStop) {
    server.send(400, "text/plain", "Emergency stop active");
    return;
  }
  forwardMotors();
  server.send(200, "text/plain", "Moving Forward");
}

void handleBackward() {
  if(emergencyStop) {
    server.send(400, "text/plain", "Emergency stop active");
    return;
  }
  backwardMotors();
  server.send(200, "text/plain", "Moving Backward");
}

void handleLeft() {
  if(emergencyStop) {
    server.send(400, "text/plain", "Emergency stop active");
    return;
  }
  leftMotors();
  server.send(200, "text/plain", "Turning Left");
}

void handleRight() {
  if(emergencyStop) {
    server.send(400, "text/plain", "Emergency stop active");
    return;
  }
  rightMotors();
  server.send(200, "text/plain", "Turning Right");
}

void handleStop() {
  stopMotors();
  server.send(200, "text/plain", "Stopped");
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
  Serial.println("✓ IR sensors initialized (20cm detection)");

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
  server.on("/forward", handleForward);
  server.on("/backward", handleBackward);
  server.on("/left", handleLeft);
  server.on("/right", handleRight);
  server.on("/stop", handleStop);
  server.on("/relay1_on", relay1On);
  server.on("/relay1_off", relay1Off);
  server.on("/relay2_on", relay2On);
  server.on("/relay2_off", relay2Off);
  server.on("/ir_status", handleIRStatus);
  server.on("/timer_status", handleTimerStatus);
  server.on("/movement_status", handleMovementStatus);
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
  Serial.println("\n💡 Auto Mode Cycle:");
  Serial.println("   1. Move 5 seconds");
  Serial.println("   2. Stop 20 minutes");
  Serial.println("   3. Repeat (wall avoidance active)\n");
}

// ---------- Main Loop ----------
void loop() {
  server.handleClient();

  if(emergencyStop) {
    return;
  }

  if(autoMode) {
    // Check if 30-minute UV cycle is complete
    if(!uvCycleComplete && (millis() - uvStartTime >= UV_DURATION)) {
      digitalWrite(RELAY1_PIN, LOW);
      digitalWrite(RELAY2_PIN, HIGH);
      uvCycleComplete = true;
      stopMotors();
      isMoving = false;
      waitingAtPosition = false;
      Serial.println("\n✓ UV Sterilization Complete!");
      Serial.println("  → UV Lamp OFF");
      Serial.println("  → Relay 2 ON");
      Serial.println("  → Motors stopped");
    }
    
    // Continue navigation if cycle not complete
    if(!uvCycleComplete) {
      autoNavigate();
      delay(100);
    }
  }

  // Check scheduled task
  if (scheduleSet && time(nullptr) >= scheduledTime && !emergencyStop) {
    Serial.println("⏰ Scheduled time reached! Starting auto sterilization...");
    autoMode = true;
    uvStartTime = millis();
    uvCycleComplete = false;
    isMoving = false;
    waitingAtPosition = false;
    digitalWrite(RELAY1_PIN, HIGH);
    digitalWrite(RELAY2_PIN, LOW);
    scheduleSet = false;
  }
}