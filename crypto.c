#include "crypto.h"

/**
 * AES-128 암호화 함수
 * 주어진 평문(plaintext)을 지정된 키(key)를 사용하여 암호화하고, 결과를 ciphertext에 저장
 *
 * plaintext : 암호화할 16바이트 평문
 * key : 16바이트 암호화 키
 * ciphertext : 암호화된 결과를 저장할 배열
 */

void aes_encrypt(const unsigned char *plaintext, const unsigned char *key, unsigned char *ciphertext) {
	AES_KEY encryptKey; // AES 암호화를 위한 키 구조체
	AES_set_encrypt_key(key, 128, &encryptKey); // 128비트 암호화 키 설정
	AES_encrypt(plaintext, ciphertext, &encryptKey); // AES 암호화 수행
}

/**
 * AES-128 복호화 함수
 * 주어진 암호문(ciphertext)을 지정된 키(key)를 사용하여 복호화하고, 결과를 plaintext에 저장
 *
 * ciphertext : 복호화할 16바이트 암호문
 * key : 16바이트 복호화 키
 * plaintext : 복호화된 결과를 저장할 배열
 */

void aes_decrypt(const unsigned char *ciphertext, const unsigned char *key, unsigned char *plaintext) {
	AES_KEY decryptKey; // AES 복호화를 위한 키 구조체
	AES_set_decrypt_key(key, 128, &decryptKey); // 128비트 복호화 키 설정
	AES_decrypt(ciphertext, plaintext, &decryptKey); // AES 복호화 수행
}

/**
 * 랜덤 바이트 생성
 * 지정된 길이의 랜덤 데이터를 생성하여 output 배열에 채움
 * OpenSSL의 RAND_bytes()을 사용
 * 랜덤 데이터 생성이 실패하면 프로그램 종료
 *
 * output : 생성된 랜덤 바이트를 저장할 배열
 * length : 생성할 랜덤 바이트의 길이
 */

void generate_random_bytes(unsigned char *output, int length) {
	if (RAND_bytes(output, length) != 1) {
		printf("Error generating random bytes\n");
		exit(1);
	}
}

/**
 * XOR 연산
 * 두 입력 배열(input1, input2)을 XOR 연산하여 결과를 output 배열에 저장
 *
 * input1 : 첫 번째 입력 배열
 * input2 : 두 번째 입력 배열
 * output : XOR 연산 결과를 저장할 배열
 * length : 배열의 길이(두 입력 배열과 결과 배열은 같은 길이여야 함)
 */

void xor_bytes(const unsigned char *input1, const unsigned char *input2, unsigned char *output, int length) {
	for (int i = 0; i < length; i++) {
		output[i] = input1[i] ^ input2[i];
	}
}
