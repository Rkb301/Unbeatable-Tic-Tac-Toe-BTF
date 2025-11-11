#include <FastLED.h>

// Uncomment for AI vs AI demo mode
// #define DEMO_MODE
// Demo mode not implemented yet

enum Player { EMPTY, HUMAN, AI_OPPONENT };

// Board configuration
static constexpr uint8_t GRID_SIZE = 3;

// Column pins (output) - for scanning sensors
static constexpr uint8_t SENSOR_COL_0_PIN = 4;
static constexpr uint8_t SENSOR_COL_1_PIN = 5;
static constexpr uint8_t SENSOR_COL_2_PIN = 6;

// Row pins (input) - for reading sensors
static constexpr uint8_t SENSOR_ROW_0_PIN = A0;
static constexpr uint8_t SENSOR_ROW_1_PIN = A1;
static constexpr uint8_t SENSOR_ROW_2_PIN = A2;

// LED configuration
static constexpr uint8_t LED_DATA_PIN = 9;
static constexpr uint8_t TOTAL_LEDS = GRID_SIZE * GRID_SIZE;

// Random seed pin
static constexpr uint8_t RANDOM_SEED_PIN = A3;

// Game state
Player currentPlayer = HUMAN;
bool isFirstMove = true;
Player gameBoard[GRID_SIZE][GRID_SIZE];
CRGB ledArray[TOTAL_LEDS];

// LED colors
const CRGB COLOR_HUMAN = CRGB::Blue;
const CRGB COLOR_AI = CRGB::Red;
const CRGB COLOR_EMPTY = CRGB::Black;
const CRGB COLOR_DRAW = CRGB::Green;

void setup() {
  Serial.begin(9600);

  // Initialize LED strip
  FastLED.addLeds<WS2811, LED_DATA_PIN, RGB>(ledArray, TOTAL_LEDS);

  // Setup sensor column pins (output)
  pinMode(SENSOR_COL_0_PIN, OUTPUT);
  pinMode(SENSOR_COL_1_PIN, OUTPUT);
  pinMode(SENSOR_COL_2_PIN, OUTPUT);

  // Setup sensor row pins (input with pullup)
  pinMode(SENSOR_ROW_0_PIN, INPUT_PULLUP);
  pinMode(SENSOR_ROW_1_PIN, INPUT_PULLUP);
  pinMode(SENSOR_ROW_2_PIN, INPUT_PULLUP);

  // Initialize random seed for AI first move randomization
  randomSeed(analogRead(RANDOM_SEED_PIN));

  // Clear board
  resetBoard();
}

// ============================================
// SENSOR POLLING FUNCTIONS
// ============================================

// Activate a specific column for sensor scanning
void activateSensorColumn(uint8_t columnIndex) {
  for (uint8_t col = 0; col < GRID_SIZE; ++col) {
    if (col == columnIndex) {
      digitalWrite(col + SENSOR_COL_0_PIN, LOW);  // Activate this column
    } else {
      digitalWrite(col + SENSOR_COL_0_PIN, HIGH); // Deactivate others
    }
  }
  // Allow signals to settle after switching columns
  delayMicroseconds(50);
}

// Check which row sensor is triggered (-1 if none) with simple debouncing
int8_t getActiveSensorRow() {
  for (uint8_t row = 0; row < GRID_SIZE; ++row) {
    uint8_t hits = 0;
    const uint8_t samples = 3;
    for (uint8_t s = 0; s < samples; ++s) {
      if (digitalRead(row + SENSOR_ROW_0_PIN) == LOW) {
        hits++;
      }
      delayMicroseconds(50);
    }
    if (hits == samples) {
      return row;
    }
  }
  return -1; // No sensor triggered
}

// ============================================
// BOARD DISPLAY FUNCTIONS
// ============================================

void updateLEDDisplay() {
  for (uint8_t row = 0; row < GRID_SIZE; ++row) {
    for (uint8_t col = 0; col < GRID_SIZE; ++col) {
      uint8_t ledIndex = row + col * GRID_SIZE;

      switch (gameBoard[row][col]) {
        case EMPTY:
          ledArray[ledIndex] = COLOR_EMPTY;
          break;
        case HUMAN:
          ledArray[ledIndex] = COLOR_HUMAN;
          break;
        case AI_OPPONENT:
          ledArray[ledIndex] = COLOR_AI;
          break;
      }
    }
  }
  FastLED.show();
}

// ============================================
// GAME LOGIC FUNCTIONS
// ============================================

void resetBoard() {
  for (uint8_t row = 0; row < GRID_SIZE; ++row) {
    for (uint8_t col = 0; col < GRID_SIZE; ++col) {
      gameBoard[row][col] = EMPTY;
    }
  }
  isFirstMove = true;
  currentPlayer = HUMAN;
}

// Check if there are any empty spaces left
bool hasMovesRemaining() {
  for (uint8_t row = 0; row < GRID_SIZE; ++row) {
    for (uint8_t col = 0; col < GRID_SIZE; ++col) {
      if (gameBoard[row][col] == EMPTY) {
        return true;
      }
    }
  }
  return false;
}

// Evaluate board state: +10 for AI win, -10 for human win, 0 for no winner
int16_t evaluateBoardState() {
  // Rows
  for (uint8_t r = 0; r < GRID_SIZE; ++r) {
    if (gameBoard[r][0] != EMPTY &&
        gameBoard[r][0] == gameBoard[r][1] &&
        gameBoard[r][1] == gameBoard[r][2]) {
      return (gameBoard[r][0] == AI_OPPONENT) ? 10 : -10;
    }
  }

  // Columns
  for (uint8_t c = 0; c < GRID_SIZE; ++c) {
    if (gameBoard[0][c] != EMPTY &&
        gameBoard[0][c] == gameBoard[1][c] &&
        gameBoard[1][c] == gameBoard[2][c]) {
      return (gameBoard[0][c] == AI_OPPONENT) ? 10 : -10;
    }
  }

  // Main diagonal
  if (gameBoard[0][0] != EMPTY &&
      gameBoard[0][0] == gameBoard[1][1] &&
      gameBoard[1][1] == gameBoard[2][2]) {
    return (gameBoard[0][0] == AI_OPPONENT) ? 10 : -10;
  }

  // Anti-diagonal
  if (gameBoard[0][2] != EMPTY &&
      gameBoard[0][2] == gameBoard[1][1] &&
      gameBoard[1][1] == gameBoard[2][0]) {
    return (gameBoard[0][2] == AI_OPPONENT) ? 10 : -10;
  }

  return 0; // No winner yet
}

// ============================================
// AI - MINIMAX ALGORITHM
// ============================================

int16_t minimax(Player player, uint8_t depth = 0) {
  int16_t boardValue = evaluateBoardState();

  // Terminal states with depth bias
  if (boardValue == 10) {
    return 10 - depth; // quicker win is better
  }
  if (boardValue == -10) {
    return depth - 10; // delay losses
  }
  if (!hasMovesRemaining()) {
    return 0; // Draw
  }

  if (player == AI_OPPONENT) {
    // AI's turn - maximize score
    int16_t maxScore = -10000;

    for (uint8_t row = 0; row < GRID_SIZE; ++row) {
      for (uint8_t col = 0; col < GRID_SIZE; ++col) {
        if (gameBoard[row][col] != EMPTY) continue;

        gameBoard[row][col] = AI_OPPONENT; // Try move
        int16_t score = minimax(HUMAN, depth + 1);
        maxScore = max(maxScore, score);
        gameBoard[row][col] = EMPTY; // Undo move
      }
    }
    return maxScore;

  } else {
    // Human's turn - minimize score
    int16_t minScore = 10000;

    for (uint8_t row = 0; row < GRID_SIZE; ++row) {
      for (uint8_t col = 0; col < GRID_SIZE; ++col) {
        if (gameBoard[row][col] != EMPTY) continue;

        gameBoard[row][col] = HUMAN; // Try move
        int16_t score = minimax(AI_OPPONENT, depth + 1);
        minScore = min(minScore, score);
        gameBoard[row][col] = EMPTY; // Undo move
      }
    }
    return minScore;
  }
}

bool calculateBestMove(uint8_t& bestRow, uint8_t& bestCol, Player player) {
  bool found = false;
  int16_t bestScore = (player == AI_OPPONENT) ? -10000 : 10000;
  Player opponent = (player == AI_OPPONENT) ? HUMAN : AI_OPPONENT;

  Serial.println("Calculating best move...");

  for (uint8_t row = 0; row < GRID_SIZE; ++row) {
    for (uint8_t col = 0; col < GRID_SIZE; ++col) {
      if (gameBoard[row][col] != EMPTY) continue;

      found = true;

      // Try this move
      gameBoard[row][col] = player;
      int16_t moveScore = minimax(opponent); // depth defaults to 0

      Serial.print("Move [");
      Serial.print(row);
      Serial.print(",");
      Serial.print(col);
      Serial.print("] score: ");
      Serial.println(moveScore);

      gameBoard[row][col] = EMPTY; // Undo move

      // Check if this is better
      bool isBetterMove = (player == AI_OPPONENT) ? (moveScore > bestScore) : (moveScore < bestScore);

      if (isBetterMove) {
        bestRow = row;
        bestCol = col;
        bestScore = moveScore;
      }

      // Early exit if optimal move found
      if ((player == AI_OPPONENT && bestScore == 10) || (player == HUMAN && bestScore == -10)) {
        Serial.println("Optimal move found!");
        return true;
      }
    }
  }

  return found;
}

// ============================================
// ANIMATION FUNCTIONS
// ============================================

void playWinAnimation(CRGB color) {
  // Chase effect
  for (int i = 0; i < TOTAL_LEDS; ++i) {
    for (int j = 0; j < TOTAL_LEDS; ++j) {
      ledArray[j] = (j == i) ? color : COLOR_EMPTY;
    }
    FastLED.show();
    delay(200);
  }

  // Fade out effect
  for (int brightness = 255; brightness >= 0; --brightness) {
    FastLED.setBrightness(brightness);
    for (int i = 0; i < TOTAL_LEDS; ++i) {
      ledArray[i] = color;
    }
    FastLED.show();
    delay(10);
  }

  FastLED.setBrightness(255); // Reset brightness
}

// ============================================
// DEBUG FUNCTIONS
// ============================================

void printBoardToSerial() {
  Serial.println("\n--- Current Board ---");
  for (uint8_t row = 0; row < GRID_SIZE; ++row) {
    for (uint8_t col = 0; col < GRID_SIZE; ++col) {
      Serial.print(gameBoard[row][col]);
      Serial.print(" ");
    }
    Serial.println();
  }
  Serial.println("-------------------\n");
}

// ============================================
// MAIN GAME LOOP
// ============================================

void loop() {
  uint8_t chosenRow = 0;
  uint8_t chosenCol = 0;

  // ===== HUMAN'S TURN =====
  if (currentPlayer == HUMAN) {
    // Scan for sensor input
    for (uint8_t col = 0; col < GRID_SIZE; ++col) {
      activateSensorColumn(col);
      int8_t row = getActiveSensorRow();

      if (row != -1 && gameBoard[row][col] == EMPTY) {
        gameBoard[row][col] = HUMAN;
        currentPlayer = AI_OPPONENT;

        Serial.print("Human played: [");
        Serial.print(row);
        Serial.print(",");
        Serial.print(col);
        Serial.println("]");

        delay(300); // Debounce delay
        break;
      }
    }
  }
  // ===== AI'S TURN =====
  else {
    Serial.println("\nAI is thinking...");

    bool placed = false;

    // Optimal deterministic first move to ensure perfect play
    if (isFirstMove) {
      isFirstMove = false;
      if (gameBoard[1][1] == EMPTY) { chosenRow = 1; chosenCol = 1; placed = true; }
      else {
        const uint8_t corners[4][2] = { {0,0}, {0,2}, {2,0}, {2,2} };
        for (uint8_t i = 0; i < 4 && !placed; ++i) {
          uint8_t r = corners[i][0];
          uint8_t c = corners[i][1];
          if (gameBoard[r][c] == EMPTY) { chosenRow = r; chosenCol = c; placed = true; }
        }
      }
    }

    // Use minimax if not placed yet and the game is not already over
    if (!placed && hasMovesRemaining() && evaluateBoardState() == 0) {
      bool ok = calculateBestMove(chosenRow, chosenCol, AI_OPPONENT);
      placed = ok;
    }

    if (placed) {
      delay(300); // brief thinking pause
      gameBoard[chosenRow][chosenCol] = AI_OPPONENT;
      currentPlayer = HUMAN;

      Serial.print("AI played: [");
      Serial.print(chosenRow);
      Serial.print(",");
      Serial.print(chosenCol);
      Serial.println("]");
    } else {
      Serial.println("AI found no valid move (likely game over).");
    }
  }

  // Update LED display
  updateLEDDisplay();

  // Check for game end
  int16_t gameResult = evaluateBoardState();

  if (gameResult == 10) {
    delay(500);
    Serial.println("\n=== AI WINS! ===");
    playWinAnimation(COLOR_AI);
    resetBoard();
    updateLEDDisplay();
    delay(2000);

  } else if (gameResult == -10) {
    delay(500);
    Serial.println("\n=== HUMAN WINS! ===");
    playWinAnimation(COLOR_HUMAN);
    resetBoard();
    updateLEDDisplay();
    delay(2000);

  } else if (!hasMovesRemaining()) {
    delay(500);
    Serial.println("\n=== DRAW! ===");
    playWinAnimation(COLOR_DRAW);
    resetBoard();
    updateLEDDisplay();
    delay(2000);
  }
}
