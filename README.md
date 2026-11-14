# Minimax Tic-Tac-Toe — Arduino Sensor Board

A physical 3×3 Tic-Tac-Toe prototype combining **Arduino firmware**, a **sensor-matrix input**, an **addressable LED display**, and an AI opponent based on **minimax** game-tree search.

## Highlights
- Reads a 3×3 sensor board using scanned digital columns and sensor row inputs
- Models game state, legal moves, and alternating turns on the microcontroller
- Evaluates possible continuations with depth-aware minimax search
- Displays human, AI, and draw states using nine addressable LEDs
- Prints candidate move scores and game events to Serial

## Technology
Arduino/C++, [FastLED](https://github.com/FastLED/FastLED), WS2811-compatible LEDs, and digital sensor-matrix scanning.

## Pin mapping
| Function | Pin |
| --- | --- |
| Sensor columns 0, 1, 2 | D4, D5, D6 |
| Sensor rows 0, 1, 2 | A0, A1, A2 |
| LED data | D9 |
| Random seed input | A3 |
| Serial Monitor | 9600 baud |

The firmware defines nine LEDs and a column-major LED index (`row + column * 3`). Check the actual physical LED order and power requirements.

## Build and run
1. Install Arduino IDE and the board package appropriate to your hardware.
2. Install the **FastLED** library.
3. Wire sensors, columns, and LEDs according to the sketch; use appropriate power supply and electrical precautions.
4. Open `UnbeatableTicTacToe.ino`, select board and serial port, and upload.
5. Open Serial Monitor at **9600 baud**; sensor input registers a human move and the firmware selects an AI response.

## Algorithm
`evaluateBoardState()` scores terminal boards (+10 AI win, −10 human win, 0 otherwise). `minimax()` recursively evaluates legal moves and biases by search depth. `calculateBestMove()` selects among free positions. The first AI turn prioritizes the center then a corner.

This uses **deterministic game-tree search**, not a trained machine-learning model.

## Project status
- Firmware source is available; a wiring schematic, photographs, and hardware validation report are not included in the repository.
- `DEMO_MODE` is commented out and not implemented.
- Blocking animations and delays make this a prototype, not a production real-time control system.
- Behavior should be validated against the actual sensor matrix, electrical wiring, and chosen microcontroller.

## Source
[UnbeatableTicTacToe.ino](./UnbeatableTicTacToe.ino)

**Portfolio focus:** Embedded systems · Arduino/C++ · adversarial game search · physical computing
