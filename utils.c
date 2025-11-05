#include "utils.h"

/**
 * 노트 출력 함수
 * 지정된 파일에서 내용을 읽어 터미널에 출력
 * 파일은 읽기 전용 모드로 열림
 *
 * filename : 출력할 파일의 이름
 */

void print_note(const char *filename) {

	// 파일을 읽기 전용 모드로 열기
	int fd = open(filename, O_RDONLY);
	if (fd == -1) {

		// 파일 열기에 실패한 경우 에러 메시지 출력
		printf("open failed\n");
		return;
	}

	char buffer[256]; // 파일 내용을 임시로 저장할 버퍼
	ssize_t bytes_read;
	
	// 파일에서 데이터 읽어와 버퍼에 저장하고 터미널에 출력
	while ((bytes_read = read(fd, buffer, sizeof(buffer) - 1)) > 0) {
		buffer[bytes_read] = '\0'; // 문자열 끝을 표시하기 위해 널 문자 추가
		printf("%s", buffer); // 버퍼의 내용 출력
	}

	// 파일 읽기 중 에러 발생 시 메시지 출력
	if (bytes_read == -1) {
		printf("read failed\n");
	}

	// 파일 디스크립터 닫기
	close(fd);
}
