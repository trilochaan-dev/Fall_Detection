#include <Wire.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ESP_Mail_Client.h>
#include <ESP8266WebServer.h>

// ---------------- WIFI ----------------
#define WIFI_SSID "XXXX"
#define WIFI_PASSWORD "XXXX"

// ---------------- EMAIL ----------------
#define SMTP_HOST "smtp.gmail.com"
#define SMTP_PORT 587
#define AUTHOR_EMAIL "XXXX@gmail.com"
#define AUTHOR_PASSWORD "XXXX"
#define RECIPIENT_EMAIL "XXXX@gmail.com"

// ---------------- WEB SERVER ----------------
ESP8266WebServer server(80);
String history = "";

// ---------------- MPU6050 ----------------
const int MPU_addr = 0x68;
int16_t AcX, AcY, AcZ, GyX, GyY, GyZ;
float ax, ay, az, gx, gy, gz;

// ---------------- FALL ----------------
bool fallDetected = false;
unsigned long fallTime = 0;

// ---------------- PULSE ----------------
#define PULSE_PIN A0
int signal;
int threshold = 550;

bool spikeDetected = false;
bool pulseAbnormal = false;
unsigned long spikeTime = 0;
unsigned long pulseTime = 0;

// ---------------- EMAIL CONTROL ----------------
unsigned long lastEmailTime = 0;

SMTPSession smtp;

// ---------------- WIFI CONNECT ----------------
void connectWiFi() {
  Serial.print("Connecting to WiFi");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi Connected!");
  Serial.println(WiFi.localIP());
}

// ---------------- WEB PAGE ----------------
void handleRoot() {

  String page = "<html><head>";
  page += "<meta http-equiv='refresh' content='3'>";
  page += "<style>";
  page += "body{font-family:Arial;background:#111;color:#fff;text-align:center}";
  page += "table{margin:auto;border-collapse:collapse}";
  page += "th,td{border:1px solid #555;padding:8px}";
  page += "th{background:#ff4444}";
  page += "</style></head><body>";

  page += "<h2>Critical Event History</h2>";
  page += "<table>";
  page += "<tr><th>Time(ms)</th><th>ACC</th><th>GYRO</th><th>Status</th></tr>";
  page += history;
  page += "</table></body></html>";

  server.send(200, "text/html", page);
}

// ---------------- SEND EMAIL ----------------
void sendEmail() {

  if (millis() - lastEmailTime < 10000) return;

  Serial.println("Sending Email...");

  SMTP_Message message;

  message.sender.name = "Health Monitor";
  message.sender.email = AUTHOR_EMAIL;
  message.subject = "CRITICAL ALERT!";
  message.addRecipient("User", RECIPIENT_EMAIL);

  message.text.content =
    "Fall + Abnormal Pulse detected!\nImmediate attention required.";

  ESP_Mail_Session session;
  session.server.host_name = SMTP_HOST;
  session.server.port = SMTP_PORT;
  session.login.email = AUTHOR_EMAIL;
  session.login.password = AUTHOR_PASSWORD;

  if (!smtp.connect(&session)) {
    Serial.println("SMTP Failed");
    return;
  }

  if (MailClient.sendMail(&smtp, &message)) {
    Serial.println("Email Sent!");
    lastEmailTime = millis();
  } else {
    Serial.println("Email Failed");
  }
}

// ---------------- MPU READ ----------------
void readMPU() {

  Wire.beginTransmission(MPU_addr);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_addr, 14, true);

  AcX = Wire.read() << 8 | Wire.read();
  AcY = Wire.read() << 8 | Wire.read();
  AcZ = Wire.read() << 8 | Wire.read();

  Wire.read(); Wire.read();

  GyX = Wire.read() << 8 | Wire.read();
  GyY = Wire.read() << 8 | Wire.read();
  GyZ = Wire.read() << 8 | Wire.read();

  ax = AcX / 16384.0;
  ay = AcY / 16384.0;
  az = AcZ / 16384.0;

  gx = GyX / 131.0;
  gy = GyY / 131.0;
  gz = GyZ / 131.0;
}

// ---------------- PULSE CHECK ----------------
void checkPulse() {

  signal = analogRead(PULSE_PIN);

  Serial.print("Pulse: ");
  Serial.println(signal);

  // Spike
  if (signal > threshold && !spikeDetected) {
    spikeDetected = true;
    spikeTime = millis();
    Serial.println("⚡ Spike");
  }

  // Drop
  if (spikeDetected && signal < threshold) {
    if (millis() - spikeTime < 1000) {
      pulseAbnormal = true;
      pulseTime = millis();
      Serial.println("🚨 Pulse abnormal");
    }
    spikeDetected = false;
  }
}

// ---------------- SETUP ----------------
void setup() {

  Serial.begin(115200);

  Wire.begin(D2, D1);

  connectWiFi();

  server.on("/", handleRoot);
  server.begin();

  // Wake MPU6050
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x6B);
  Wire.write(0);
  Wire.endTransmission(true);

  Serial.println("System Ready");
}

// ---------------- LOOP ----------------
void loop() {

  server.handleClient();

  checkPulse();
  readMPU();

  float acc = sqrt(ax * ax + ay * ay + az * az);
  float gyro = sqrt(gx * gx + gy * gy + gz * gz);

  Serial.print("ACC: ");
  Serial.print(acc);
  Serial.print(" | GYRO: ");
  Serial.println(gyro);

  // -------- FALL DETECTION --------
  if (acc > 2.2 && gyro > 30) {
    fallDetected = true;
    fallTime = millis();
    Serial.println("💥 Fall detected");
  }

  // -------- COMBINED CONDITION --------
  if (fallDetected && pulseAbnormal &&
      abs((long)(fallTime - pulseTime)) < 5000) {

    Serial.println("🚨 CRITICAL CONDITION!");

    String row = "<tr><td>";
    row += String(millis());
    row += "</td><td>";
    row += String(acc, 2);
    row += "</td><td>";
    row += String(gyro, 2);
    row += "</td><td>CRITICAL</td></tr>";

    history = row + history;

    sendEmail();

    fallDetected = false;
    pulseAbnormal = false;
  }

  delay(100);
}
