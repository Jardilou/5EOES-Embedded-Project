#include <Arduino.h>
#include <SoftwareSerial.h>
#include <Crypto.h>
#include <SHA3.h>
#include <avr/pgmspace.h>

const int salt_size = 4;
const char *secret = "Je suis une petite tortue";

// Buffer for digest (32 bytes for SHA3-256)
uint8_t digest[32];

const uint8_t MY_RX_PIN = 10;
const uint8_t MY_TX_PIN = 11;
SoftwareSerial comms(MY_RX_PIN, MY_TX_PIN); // RX, TX


///////////////////////////////////////////////////////////////////////////////////////////////////////////
//CHANGED SECTION
// Computing Hash locally in order not to store the plaintext password
//This step is done before uploading on the arduino

const uint8_t SAVED_SALT[] PROGMEM = {
  0x12, 0x34, 0x56, 0x78
};

//Hash of my password
const uint8_t SAVED_HASH[] PROGMEM = {
  0x81, 0xB4, 0x4C, 0xAE, 0x27, 0x0C, 0xB3, 0x31,
  0x27, 0xAD, 0xD7, 0x8C, 0x8C, 0x5A, 0xA7, 0xF5,
  0x95, 0x26, 0xF5, 0xE4, 0xBA, 0xF2, 0x8B, 0x20,
  0x5D, 0x1D, 0x9B, 0x0F, 0xA7, 0xE9, 0x40, 0x35
};

// Compute SHA3-256(salt || passwordAttempt) into outDigest
void hashAttemptWithSalt(const char *attempt, const uint8_t *salt, size_t saltLen, uint8_t *outDigest) {
  SHA3_256 sha3;
  sha3.reset();
  sha3.update(salt, saltLen);
  sha3.update((const uint8_t *)attempt, strlen(attempt));
  sha3.finalize(outDigest, 32);
}

// Constant-time compare: returns true iff a == b (both len bytes)
bool consttime_eq(const uint8_t *a, const uint8_t *b, size_t len) {
  uint8_t diff = 0;
  for (size_t i = 0; i < len; ++i) {
    diff |= a[i] ^ b[i];
  }
  // diff == 0 => equal
  // We return (diff == 0) without any branching that depends on contents
  return diff == 0;
}

bool check_password_attempt(const char *attempt) {
  // load salt from PROGMEM
  uint8_t salt[4];
  for (size_t i = 0; i < sizeof(salt); ++i) salt[i] = pgm_read_byte_near(SAVED_SALT + i);

  // compute hash of salt || attempt
  hashAttemptWithSalt(attempt, salt, sizeof(salt), digest);

  // load stored hash from PROGMEM
  uint8_t stored[32];
  for (size_t i = 0; i < sizeof(stored); ++i) stored[i] = pgm_read_byte_near(SAVED_HASH + i);

  // constant-time compare
  return consttime_eq(digest, stored, 32);
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//UNCHANGED SECTION


// Read a full line 
String readLine(Stream &s) {
  String str;
  while (true) {
    if (s.available()) {
      char c = s.read();
      if (c == '\r' or c == '\n') break;
      str += c;
    }
  }
  return str;
}


void hashPassword(const char *password, const uint8_t *salt, size_t saltLen, uint8_t *outDigest) {
  SHA3_256 sha3;
  sha3.reset();

  // Add salt
  sha3.update(salt, saltLen);

  // Add password
  sha3.update((const uint8_t *)password, strlen(password));

  // Finalize
  sha3.finalize(outDigest, 32);
}

void protected_section(Stream &out) {
  out.println(">> ACCESS GRANTED: running protected section");

  uint8_t SALT[4];
  for (int i = 0; i < salt_size; i++) {
    SALT[i] = (uint8_t) random(0x21, 0x7E);
  }

  hashPassword(secret, SALT, sizeof(SALT), digest);

  out.print("Here is your salt: ");
  for (int i = 0; i < 4; i++) {
    if (SALT[i] < 0x10) out.print("0");
    out.print(SALT[i], HEX);
  }
  out.println();
  out.print("Here is your hash: ");
  for (int i = 0; i < 32; i++) {
    if (digest[i] < 0x10) out.print("0");
    out.print(digest[i], HEX);
  }
  out.println();
}

void setup() {
  Serial.begin(115200);
  while (!Serial) { delay(10); }
  Serial.println();
  Serial.println("Welcome to the vault. This is not the main entrance.");
  comms.begin(9600);
  comms.println("Enter password:");
  pinMode(13, OUTPUT);
  digitalWrite(13, LOW);
}

void loop() {
  if (comms.available()) {
    String attempt = readLine(comms);
    attempt.trim();
    if (attempt.length() == 0) {
      comms.println("No input. Enter password:");
      return;
    }
    char attempt_c[64];
    attempt.toCharArray(attempt_c, sizeof(attempt_c));
    digitalWrite(13, HIGH);
    if (check_password_attempt(attempt_c)) {
      protected_section(comms);
    } else {
      comms.println(">> ACCESS DENIED");
    }
    digitalWrite(13, LOW);
    comms.println();
    comms.println("Enter password:");
  }
}
