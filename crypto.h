#ifndef CRYPTO_H
#define CRYPTO_H

#include <openssl/aes.h>
#include <stdio.h>
#include <string.h>
#include <openssl/rand.h>

// AES-128 암호화 함수 선언
void aes_encrypt(const unsigned char *plaintext, const unsigned char *key, unsigned char *ciphertext);

// AES-128 복호화 함수 선언
void aes_decrypt(const unsigned char *ciphertext, const unsigned char *key, unsigned char *plaintext);

// 랜덤 바이트 생성 함수
void generate_random_bytes(unsigned char *buffer, int length);

// XOR 연산 함수
void xor_bytes(const unsigned char *input1, const unsigned char *input2, unsigned char *output, int length);

#endif
