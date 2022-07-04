/**
 * "Mastermind" Puzzle
 * 
 * The puzzle has a secret sequence of values (numbers, letters, shapes, objects, colours), 
 * definable in code, or initialised to a random value. Players must deduce what this secret is.
 * To do so, players input a guess - a sequence of the same length as the secret code, and receive 
 * a response indicating how many values were totally correct (appearing in the same position in  
 * the guess and the secret code) and how many were partially correct (values in the guess that appeared
 * in a different position in the secret code).
 * They use this information to refine future guesses until it matches the secret, at which point
 * the puzzle is solved and a relay activated (which could, e.g. release a maglock)
 * 
 * See: https://www.cs.uni.edu/~wallingf/teaching/cs3530/resources/knuth-mastermind.pdf
 */

// INCLUDES
// For controlling Neopixel LED strips - download from http://fastled.io/
#include <FastLED.h>
// Used to control a panel of MAX7219 LED displays showing the input sequence - https://github.com/wayoda/LedControl
#include "src/LedControl.h"
// Used to form structured JSON output to send via serial connection - https://arduinojson.org/
#include <ArduinoJson.h>
// Header file contains definition of the values appeaing on the display
#include "font.h"


signed short minutes, secondes;
char timeline[16];




// CONSTANTS
// Define the Arduino pins to which buttons are attached
const byte buttonPins[] = {2, 3, 4, 5};
const byte submitPin = 6;
// This pin will be sent a HIGH pulse when the correct solution is entered
const byte relayPin = A3;
// Number of values in the secret code
const byte codeLength = 4;
// The number of different types of symbol from which the code is made
const byte numSymbols = 10;
// The number of LEDs used to provide feedback on each guess
const byte numLeds=24;


int NumStep = 0;  //Variable to interate


// GLOBALS
// Initialise a LedControl object
// Parameters are DataIn (Din), Clock (Clk), ChipSelect (CS), Number of MAX72XX modules
LedControl lc = LedControl(A1, 13, A0, 4);
// Keep track of which buttons were pressed last frame
bool lastButtonState[codeLength];
// The secret code players are trying to guess.
// This is an array of byte values, each between 0 and numSymbols-1
// The actual image displayed for each element is defined by the array in the font.h file
byte secret[codeLength] = {2,9,5,1};
// The player's guess
byte guess[codeLength];
// The number of guesses made
int numGuesses = 0;
// Define the array of leds used to provide feedback
CRGB leds[numLeds];

/**
 * Sets the secret to a random sequence
 */
void setRandomSecret() {
  // Create seed value based on random noise on unconnected analog input pin
  randomSeed(analogRead(A5));
  for(int i=0; i<codeLength; i++) {
    // Assign each element of the secret code to a random value up to max allowed
    secret[i] = random(numSymbols);
  }
}





/**
 * Displays the response (blacks and whites) to a player's guess
 */
void displayResponse(int numBlacks, int numWhites){

  // Loop to allow flashing of "white" pegs
  for(int loop=0; loop<7; loop++){
    // Colour in a quarter of all LEDs for each correct "black" peg
    for(int i=0; i<numBlacks*(numLeds/4); i++) {
      leds[i] = CRGB::Green;
    }
    // Now colour in a quarter of remaining LEDs for each "white" peg in the guess
    for(int i=numBlacks*(numLeds/4); i<(numBlacks+numWhites)*(numLeds/4); i++) {
      // Alternate between black and orange
      leds[i] = (loop%2) ? CRGB::Black : CRGB::Orange;
    }
    FastLED.show();
    delay(200);
  }

  // Turn off all the LEDs again
  for(int i=0; i<numLeds; i++){
    leds[i] = CRGB::Black;
  }
  FastLED.show(); 
}

/**
 * Send details of a guess and its response as JSON over serial connection 
 */
bool serialUpdate(byte secret[], byte guess[], byte blackCount, byte whiteCount){
  // Create a JSON document of sufficient size
  StaticJsonDocument<256> doc;
  // Include a unique ID to identify this puzzle
  doc["id"] = "Mastermind";
  // Create a simple property with the number of guesses
  doc["turn"] = numGuesses;
  // Include the secret code as an array of elements
  JsonArray jsonSecret = doc.createNestedArray("secret");
  for(int i=0;i<4; i++) { jsonSecret.add(secret[i]); }
  // Include the player's last guess
  JsonArray jsonGuess = doc.createNestedArray("guess");
  for(int i=0;i<4; i++) { jsonGuess.add(guess[i]); }
  // Create an array containing the response of blacks and whites
  JsonArray result = doc.createNestedArray("result");
  result.add(blackCount);
  result.add(whiteCount);
  doc["state"] = (blackCount == codeLength) ? "SOLVED" : "UNSOLVED" ;
  // Send the result over the serial connection
  serializeJson(doc, Serial);
  Serial.println("");
}

/**
 * Compares the player's last guess to the stored secret code
 */
bool calculateResponse(byte &blackCount, byte &whiteCount) {

  // Keep track of which elements have already been scored
  bool secretUsed[codeLength] = {false, };// = {false, false, false, false};
  bool guessUsed[codeLength] = {false, };//= {false, false, false, false};

  // Count "black" - correct values in correct positions
  blackCount = 0;
  for (int i=0; i<codeLength; i++) {
    if (guess[i] == secret[i]) {
      blackCount++;
      secretUsed[i] = true;
      guessUsed[i] = true;
    } 
  }

  // Count "white" - correct values in *incorrect* position
  whiteCount = 0;
  for (int i=0; i<codeLength; i++) {
    // Don't score elements from guess already counted as black hits
    if(!guessUsed[i]) {
      for(int j=0; j<codeLength; j++) {
        // Don't score elements from secret already counted as black hits
        if(!secretUsed[j] && (guess[i] == secret[j])) {
          whiteCount++;
          // Mark to ensure this element is not double-counted
          secretUsed[j] = true;
          break;
        }
      }
    }
  }

  // Call the serial update function   
  serialUpdate(secret, guess, blackCount, whiteCount);

  // Return true if code was correct
  return (blackCount == codeLength);
}

/**
 * Reads player input to guess the secret code
 */
void getInput() {

  // For testing, use the following code to enter input via serial monitor
  /*
  int counter=0;
  // Wait until something is received
  while (!Serial.available());
  while(true) {
    char incomingByte = Serial.read();
    if (incomingByte == '\n' || incomingByte == '\r') { break; } // exit the while(1), we're done receiving
    if (incomingByte == -1) { continue; } // if no characters are in the buffer read() returns -1
    guess[counter] = (incomingByte - 48);
    counter++;
    if(counter >= codeLength) { break; }
  }
  */

  // Ensure that submit button is not being held down to prevent double-submits
  while(!digitalRead(submitPin)) {
    delay(100);
  }
  // Loop until the submit button is pressed
  while(digitalRead(submitPin)) {
    for(int i=0; i<4; i++){
      // Read each button input
      bool buttonState = digitalRead(buttonPins[i]);
      // If the button wasn't pressed last frame, but is now
      if(!buttonState && lastButtonState[i]) {
        // Increase the value of this guess element
        guess[i]++;
        // And, if the value goes out of allowed range, wrap back to 0
        if(guess[i]>=numSymbols) { guess[i] = 0; }
      }
      // Update the last state for this button
      lastButtonState[i] = buttonState;
      // Display the guess on the LED matrix
      for(int row=0; row<8; row++){
        lc.setRow(i,row,digits[guess[i]][row]);
      }
    }
  }
}

const int outputsignal=10;
void setup() {
  // Loop over and initialise all the MAX72XX modules
  for(int i=0; i<4; i++){
    lc.shutdown(i,false);
    lc.setIntensity(i,5);
    lc.clearDisplay(i);
  }
 

  // Intialise the input pins
  for(int i=0; i<4; i++){
    pinMode(buttonPins[i], INPUT_PULLUP);
  }
  pinMode(submitPin, INPUT_PULLUP);
  pinMode(relayPin, OUTPUT);
  pinMode(outputsignal, OUTPUT);
  digitalWrite(relayPin, LOW);

  // Serial connection used to print debug values
  Serial.begin(9600);

  FastLED.addLeds<WS2812B, A2, GRB>(leds, numLeds);
  // Uncomment the following line if you'd like to set a random secret code
  // setRandomSecret();
}

void loop() {
  
  // Wait for the player to input a guess
  getInput();
  
  // Check their guess against the correct code
  byte whiteCount=0, blackCount=0;
  calculateResponse(blackCount, whiteCount);
  
     
  // Correct code entered
  if(blackCount == codeLength) {
    // Unlock a maglock, flash a light, whatever...
    digitalWrite(outputsignal,HIGH);
    
    
    // Serial monitor
    Serial.print(F("YOU WIN!"));

    // Chase sequence  
    for(int i=0; i<numLeds; i++){
      leds[i] = CRGB::Green;
      FastLED.show();
      delay(20);
    }
    for(int i=0; i<numLeds; i++){
      leds[i] = CRGB::Black;
      FastLED.show();
      delay(20);
    }

    digitalWrite(outputsignal,LOW);
  }
  // Incorrect guess
  else {
    displayResponse(blackCount, whiteCount);
  }

  // If desired, heep count of the number of guesses
  numGuesses++;  

 
}
