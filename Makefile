CC = gcc # 컴파일러
TARGET = dkuware # 프로그램명
OBJS = dkuware.o crypto.o utils.o # 오브젝트 파일들
CFLAGS = -Wall -g # 컴파일 옵션
LDFLAGS = -pthread -lcrypto -lssl # 링커 옵션, 링크할 라이브러리들, -lpthread 등

all: $(TARGET) # 최종 파일 명시

$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

.c.o:
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(TARGET)
