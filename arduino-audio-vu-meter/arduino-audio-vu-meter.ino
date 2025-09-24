#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Arduino.h>
#include <math.h>

// -------------------- OLED CONFIGURATION --------------------
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// -------------------- AUDIO CONFIGURATION --------------------
const int AUDIO_PIN = A0;                 // Analog input pin for audio signal
const unsigned int SAMPLE_COUNT = 400;    // Number of samples per RMS calculation
const unsigned long SAMPLE_INTERVAL_US = 200; // Sampling interval (200 µs ≈ 5 kHz)

// RMS normalization range (depends on microphone sensitivity & environment)
const float RMS_MIN = 0.5;   // Lower bound for detectable volume
const float RMS_MAX = 50.0;  // Upper bound for detectable volume

// Exponential smoothing coefficient (0..1)
// Higher ALPHA = more responsive, lower ALPHA = smoother
const float ALPHA = 0.2;

// Smoothed volume value (percentage)
float emaVolume = 0;

// -------------------- SETUP --------------------
void setup() {
  Serial.begin(115200);

  // Initialize OLED display
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 display not found"));
    for(;;); // Halt execution if display is missing
  }

  // Splash screen
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(2);
  display.setCursor(0,0);
  display.println("Start...");
  display.display();
  delay(1000);
}

// -------------------- MAIN LOOP --------------------
void loop() {
  // --- 1) Collect samples and compute sum & squared sum ---
  long sum = 0;         // For average (DC offset)
  long sumSq = 0;       // For RMS calculation

  for (unsigned int i = 0; i < SAMPLE_COUNT; i++) {
    int v = analogRead(AUDIO_PIN);
    sum += v;
    sumSq += (long)v * v;
    delayMicroseconds(SAMPLE_INTERVAL_US);
  }

  // --- 2) Compute average (DC offset) and RMS ---
  float avg = (float)sum / SAMPLE_COUNT;
  float rms = sqrt(((float)sumSq / SAMPLE_COUNT) - (avg * avg));

  // --- 3) Normalize RMS value into 0..100% range ---
  float norm = (rms - RMS_MIN) / (RMS_MAX - RMS_MIN);
  if (norm < 0.0) norm = 0.0;
  if (norm > 1.0) norm = 1.0;
  float rawPercent = norm * 100.0;

  // --- 4) Apply exponential moving average smoothing ---
  emaVolume = ALPHA * rawPercent + (1.0 - ALPHA) * emaVolume;
  int volumePercent = (int)round(emaVolume);

  // --- 5) Debug output to Serial Monitor ---
  Serial.print("RMS:");
  Serial.print(rms, 2);
  Serial.print("  Volume:");
  Serial.println(volumePercent);

  // --- 6) Display on OLED ---
  display.clearDisplay();

  // Textual volume percentage
  display.setTextSize(2);
  display.setCursor(0,0);
  display.print("Vol: ");
  display.print(volumePercent);
  display.println("%");

  // Volume bar (VU meter)
  int barWidth = map(volumePercent, 0, 100, 0, SCREEN_WIDTH - 10);
  int barHeight = 10;
  int barX = 5;
  int barY = 50;

  // Draw bar outline
  display.drawRect(barX, barY, SCREEN_WIDTH - 10, barHeight, SSD1306_WHITE);
  // Draw filled bar according to current volume
  display.fillRect(barX, barY, barWidth, barHeight, SSD1306_WHITE);

  display.display();

  delay(50); // Small delay for display stability
}
