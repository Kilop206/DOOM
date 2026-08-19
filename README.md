# 🕹️ DOOM: Survival 3D (ESP32 + OLED SSD1306)

A retro first-person shooter (FPS) featuring a **3D Raycasting Engine** built for the ESP32 and an OLED SSD1306 display. Inspired by 16-bit classics, this project features DDA real-time raycasting graphics, buzzer audio, procedural maps, a tactical minimap, and a dynamic difficulty system.

---

## 🚀 Features

* **3D Raycasting Engine**: Real-time DDA raycasting rendering walls and enemy sprites at 128x64 resolution.
* **Difficulty System**: Three selectable difficulty levels on the title screen that directly affect enemy attack speed, damage, and score multipliers.
* **Dynamic Audio**: Background DOOM theme music with sound effects for gunfire, player damage, and menu navigation.
* **Tactical Minimap**: Quick toggle to view the map layout and real-time enemy positions.
* **Procedural Generation**: Randomly generated map layouts and enemy spawn points for every match.

---

## 🛠️ Hardware Requirements

| Component | Specification / Description |
| :--- | :--- |
| **Microcontroller** | ESP32 (38-pin or 30-pin development board) |
| **Display** | 0.96" I2C OLED SSD1306 (128x64) |
| **Audio** | 5V Passive Buzzer |
| **Buttons** | 4x Push Buttons (utilizes internal active pull-up resistors) |
| **Breadboard & Wires** | Standard breadboard and jumper wires |

---

## 🔌 Wiring & Pinout

| Component | Component Pin | ESP32 Pin (GPIO) |
| :--- | :--- | :--- |
| **OLED SSD1306** | SDA | **GPIO 21** |
| **OLED SSD1306** | SCL | **GPIO 22** |
| **OLED SSD1306** | VCC / GND | **3V3 / GND** |
| **Forward Button** | Terminal A | **GPIO 18** (Terminal B to GND) |
| **Turn Right Button** | Terminal A | **GPIO 32** (Terminal B to GND) |
| **Turn Left Button** | Terminal A | **GPIO 33** (Terminal B to GND) |
| **Fire Button** | Terminal A | **GPIO 23** (Terminal B to GND) |
| **Buzzer** | Positive (+) | **GPIO 25** (Negative to GND) |

---

## 🎮 Controls

### Title / Main Menu
* **Turn Left / Turn Right**: Select Difficulty
* **Fire**: Confirm selection and start game

### In-Game
* **Move Forward**: Move player forward
* **Turn Left / Turn Right**: Rotate camera view
* **Fire**: Shoot weapon
* **Turn Left + Turn Right (Simultaneous)**: Toggle Minimap

---

## ⚖️ Difficulty Matrix

| Difficulty | Damage per Attack | Enemy Attack Cooldown | Score Multiplier | HUD Indicator |
| :--- | :---: | :---: | :---: | :---: |
| **EASY** | 15 HP | 1.8s | 1x (50 pts / kill) | `D:E` |
| **MEDIUM** | 30 HP | 1.1s | 2x (100 pts / kill) | `D:M` |
| **HARD** | 50 HP | 0.6s | 4x (200 pts / kill) | `D:H` |

---

## 📦 Required Libraries

Install these dependencies via the Arduino IDE or PlatformIO **Library Manager**:

1. `Adafruit SSD1306`
2. `Adafruit GFX Library`
3. `Wire` (Built-in ESP32 core library)

---

## 💻 How to Build & Upload

1. Open **Arduino IDE**.
2. Install the ESP32 board package under **Tools > Board > Boards Manager**.
3. Select your ESP32 module (e.g., *ESP32 Dev Module*).
4. Install the required libraries listed above.
5. Paste the code into your sketch, connect your ESP32 via USB, and click **Upload**.

---

## 🎥 Filming

https://drive.google.com/file/d/162PQG-hKj2oQitum-GFrnRV6FVsqX24q/view?usp=sharing

---

## 👥 Contributing

Contributions are welcome! Feel free to open an **Issue** to report bugs or submit a **Pull Request** with enhancements (such as new enemy types, sprite animations, or extra weapon mechanics).
