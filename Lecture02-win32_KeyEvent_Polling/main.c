/*
 * [실습 주제: GetAsyncKeyState를 이용한 실시간 키 폴링]
 * * 1. 0x8000 (이진수: 1000 0000 0000 0000)
 * - GetAsyncKeyState의 리턴값 중 '현재 키가 눌림'을 나타내는 상태 비트임.
 * - & 연산을 통해 이 비트가 켜져 있는지 확인하면 실시간 입력 감지가 가능함.
 * * 2. 왜 0x8001이나 1이 아닌 0x8000인가?
 * - 하위 비트는 시스템마다 동작이 달라 신뢰할 수 없음.
 * - 게임 엔진의 표준은 항상 0x8000(최상위 비트)을 체크하는 것임.
 * 
 * 
 * [OS 입출력(I/O) 모델 심화 학습]
 * 1. Synchronous Blocking I/O (가장 고전적인 방식)
 * - 데이터가 올 때까지 스레드가 잠들고(Sleep), 데이터가 오면 깨어나서 읽음.
 * - 게임 엔진에서는 화면이 멈추기 때문에 절대 사용 금지.
 * 
 * 2. Synchronous Non-blocking I/O (현 주차 실습 방식: Polling)
 * - 데이터가 없어도 즉시 리턴함. 대신 내가 '직접' 계속 물어봐야 함(Polling).
 * - GetAsyncKeyState()가 이 방식임. "지금 상태 어때?" -> "안 눌림(즉시 리턴)"
 * 
 * 3. Asynchronous Non-blocking I/O (가장 진보된 방식)
 * - OS에게 "키 눌리면 나한테 알려줘(Callback)"라고 던져두고 내 할 일 함.
 * - 유저 모드와 커널 모드 사이의 오버헤드가 가장 적음.
 */
 

#include <windows.h>
#include <stdio.h>

int main() {
    printf("=== Real-time Key Polling Demo ===\n");
    printf("Press 'A' to see the status.\n");
    printf("Press 'ESC' to exit the game loop.\n\n");

    // 이전 프레임의 키 상태를 저장 (눌렀을 때와 뗐을 때를 구분하기 위함)
    int isAPressedPrev = 0;

    // --- [정석 게임 루프 (Simplified)] ---
    while (1) {
        // 1. ESC 키가 눌렸는지 폴링 (탈출 조건)
        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
            printf("\nExiting Game Loop...\n");
            break;
        }

        // 2. 'A' 키 실시간 폴링
        // 현재 프레임의 눌림 상태 확인
        int isAPressedCurrent = (GetAsyncKeyState('A') & 0x8000);

        // [이벤트 처리 로직]
        // 지난 프레임엔 안 눌렸는데, 지금 눌렸다면? -> "Key Down!"
        if (isAPressedCurrent && !isAPressedPrev) {
            printf("[EVENT] 'A' Key Pressed (Down)\n");
        }
        // 지난 프레임엔 눌려 있었는데, 지금 안 눌렸다면? -> "Key Up!"
        else if (!isAPressedCurrent && isAPressedPrev) {
            printf("[EVENT] 'A' Key Released (Up)\n");
        }
        // 지금도 눌려 있고 지난번에도 눌려 있다면? -> "Key Staying Pressed (Hold)"
        else if (isAPressedCurrent && isAPressedPrev) {
            // 너무 많이 찍히니까 주석 처리하거나 특정 타이밍에만 출력
            printf("'A' is being held...\n");
        }

        // 현재 상태를 이전 상태로 저장 (다음 프레임을 위해)
        isAPressedPrev = isAPressedCurrent;

        // CPU 부하를 줄이기 위한 짧은 휴식 (실제 엔진에선 프레임 동기화 로직이 들어감)
        Sleep(10);
    }

    return 0;
}