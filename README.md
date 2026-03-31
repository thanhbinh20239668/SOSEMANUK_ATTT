🔐 SOSEMANUK Stream Cipher (C Implementation)
📌 Overview

This project implements the SOSEMANUK stream cipher in C.
SOSEMANUK is a high-performance stream cipher designed in the eSTREAM project, combining:

Linear Feedback Shift Register (LFSR)
Finite State Machine (FSM)
Reduced-round Serpent block cipher

The program supports encryption, decryption, benchmarking, and file inspection.

⚙️ Features
🔒 Encrypt file (input.txt → encrypted.bin)
🔓 Decrypt file (encrypted.bin → decrypted.txt)
✍️ Input custom plaintext from keyboard
⚡ Performance benchmark (MB/s)
🔍 Hex dump of encrypted data
🏗️ Project Structure
.
├── main.c
├── input.txt
├── encrypted.bin
├── decrypted.txt
└── README.md
🔑 Algorithm Components
1. Key Setup
Expands a 128-bit key into 100 subkeys
Uses:
Golden ratio constant (PHI)
Serpent S-Boxes
Bit rotations (ROTL)
2. IV Setup
Uses a 128-bit IV
Runs 24 rounds of Serpent-like transformation
Initializes:
LFSR state (s[10])
FSM registers (r1, r2)
3. LFSR (Linear Feedback Shift Register)
10 × 32-bit registers
Feedback function:
s_new = s9 ^ div_alpha(s3) ^ mul_alpha(s0);
4. FSM (Finite State Machine)

Generates intermediate value:

f_t = (s9 + r1) ^ r2;
5. S-Box (Serpent S2)
Non-linear transformation
Operates on 4 × 32-bit words using bitslice technique
6. Keystream Generation
Generates 16 bytes per iteration
Combines:
FSM output
S-Box transformation
Previous LFSR state
🔐 Encryption Principle

SOSEMANUK is a stream cipher, so:

Ciphertext = Plaintext XOR Keystream
Plaintext  = Ciphertext XOR Keystream

👉 Encryption and decryption use the same function.

🚀 Usage
Compile
gcc main.c -o sosemanuk
Run
./sosemanuk
📋 Menu Options
Option	Description
1	Encrypt file
2	Decrypt file
3	Input new plaintext
4	Benchmark (1MB × 100 iterations)
5	Hex dump encrypted file
0	Exit
🧪 Example Workflow
Choose 3 → enter plaintext
Choose 1 → encrypt → encrypted.bin
Choose 2 → decrypt → decrypted.txt
⚡ Benchmark
Processes 100 MB of data
Outputs performance in MB/s
🧠 Technical Details
Rotate Left (ROTL)
#define ROTL32(v, n) (((v) << (n)) | ((v) >> (32 - (n))))
Finite Field Operations
mul_alpha() → multiplication in GF(2^32)
div_alpha() → division in GF(2^32)
Implemented using lookup tables
⚠️ Notes
Key and IV are currently hardcoded:
uint8_t key[16] = {0xAB, 0x01, 0x02};
uint8_t iv[16]  = {0xCD, 0x03, 0x04};

👉 For real applications:

Use secure random key/IV
Never reuse IV with same key
🔧 Future Improvements
 User-defined key and IV
 Streaming for large files
 SIMD optimization (AVX/NEON)
 Integration with embedded systems (ESP32, RP2040)
 Compare with AES / ChaCha20
📚 References
SOSEMANUK Specification (eSTREAM)
Serpent Block Cipher
Cryptography Engineering
👨‍💻 Author
Cryptography implementation for study and benchmarking purposes
🏁 Conclusion

This project demonstrates:

Practical implementation of a stream cipher
Integration of LFSR, FSM, and S-Box
Real-world encryption/decryption workflow

👉 Suitable for:

Cryptography learning
Performance testing
Embedded system applications
