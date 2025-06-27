# One-Time Pad Encryption and Decryption

This project implements a simple **One-Time Pad** encryption and decryption system using client-server socket communication in C.

## 📂 Project Overview

The system consists of:
- `enc_client`: Sends plaintext and key to the encryption server.
- `enc_server`: Encrypts the plaintext using the one-time pad method and returns the ciphertext.
- `dec_client`: Sends ciphertext and key to the decryption server.
- `dec_server`: Decrypts the ciphertext using the one-time pad method and returns the plaintext.
- `keygen`: Generates a random key file containing uppercase letters and spaces.

The encryption and decryption processes use modular arithmetic over a 27-character set: **A-Z and space**.

---

## 🗂️ File Descriptions

- **`enc_client.c`**: Encryption client.
- **`enc_server.c`**: Encryption server.
- **`dec_client.c`**: Decryption client.
- **`dec_server.c`**: Decryption server.
- **`keygen.c`**: Random key generator.

---

## ⚙️ Compilation

Compile each file using `gcc`:

```bash
gcc -o enc_client enc_client.c
gcc -o enc_server enc_server.c
gcc -o dec_client dec_client.c
gcc -o dec_server dec_server.c
gcc -o keygen keygen.c
```
---
## 🚀 Usage
### 🔑 Generate Key
```bash
./keygen LENGTH > keyfile
```
Example:
```bash
./keygen 100 > mykey
```
---
## 🖥️ Start Servers
``` bash
./enc_server PORT &
./dec_server PORT &
```
Example:
```bash
./enc_server 5000 &
./dec_server 5001 &
```
---
## ✉️ Run Encryption Client
```bash
./enc_client PLAINTEXT_FILE KEY_FILE PORT > ciphertext
```
Example:
```bash
./enc_client plaintext1 mykey 5000 > ciphertext1
```
---
## 🔓 Run Decryption Client
```bash
./dec_client CIPHERTEXT_FILE KEY_FILE PORT > plaintext
```
Example:
```bash
./dec_client ciphertext1 mykey 5001 > plaintext1_decrypted
```
---
## ❗ Error Handling
* Ensures the key is long enough.

* Validates that plaintext and ciphertext contain only uppercase letters and spaces.

* Handles invalid socket connections.

* The server will reject improperly formatted messages or invalid characters.
---
## 📌 Notes
* Key length must be greater than or equal to the length of the plaintext or ciphertext.

* The system assumes the client and server are running on the same machine (localhost / 127.0.0.1).

* Files should not contain special characters, lowercase letters, or extra whitespace.
---
## 📚 License
This project is for educational purposes and was developed for the CS 374: Operating Systems I course at Oregon State University.

