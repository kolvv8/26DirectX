#include <windows.h>
#include <stdio.h>

/*
 * [하위 시스템과 진입점]
 * - /subsystem:console -> 창을 띄우되, 배후에 콘솔(검은 창)을 함께 띄움 (printf 디버깅용).
 * - /entry:WinMainCRTStartup -> 윈도우 프로그램의 시작점인 WinMain을 호출하라고 링커에게 명령함.er
 */
#pragma comment(linker, "/entry:WinMainCRTStartup /subsystem:console")

 /*
  * [윈도우 프로시저 (WndProc)]
  * - 운영체제(OS)가 하드웨어(키보드, 마우스) 변화를 감지하면 '메시지'를 생성함.
  * - 이 함수는 OS가 우리 프로그램에게 "야, 이런 일이 생겼어!"라고 이벤트 발생시 보고하는 '콜백(Callback)'으로 수행할 함수임.
  * - 여기서 처리하지 않은 메시지는 반드시 DefWindowProc(기본 처리)으로 넘겨줘야 창이 정상 동작함.
  */
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)    //윈도우의 이벤트는 메시지에 담김
    {
        // --- [키보드 메시지 처리] ---
        // WM_KEYDOWN: 키보드가 눌리는 '사건'이 발생했을 때 OS가 호출함.
    case WM_KEYDOWN:
        /*
         * [wParam의 정체]
         * - wParam(Word Parameter)에는 '어떤 키'가 눌렸는지에 대한 가상 키 코드(VK)가 담겨 있음.
         * - VK_LEFT(방향키 좌), VK_RIGHT(방향키 우) 혹은 'A', 'S' 등으로 판별 가능함.
         */
        printf("[EVENT] Key Pressed: %c (Virtual Key: %lld)\n", (char)wParam, wParam);

        if (wParam == VK_LEFT || wParam == 'A')  printf("  >> 로직: 이런식으로 왼쪽 이동에 대한 키 조작 가능!\n");
        if (wParam == VK_RIGHT || wParam == 'D') printf("  >> 로직: 이런식으로 오른쪽 이동에 대한 키 조작 가능!\n");
        if (wParam == 'Q') {
            printf("  >> 로직: Q 입력 감지, 프로그램 종료 요청!\n");
            PostQuitMessage(0); // 메시지 큐에 WM_QUIT을 넣음
        }
        break;

    case WM_KEYUP:
        // 키보드에서 손을 떼는 순간 발생하는 메시지
        printf("[EVENT] Key Released: %c\n", (char)wParam);
        break;

        // --- [마우스 메시지 처리] ---
    case WM_LBUTTONDOWN:
        /*
         * [lParam]
         * - lParam(Long Parameter)에는 마우스의 좌표 같은 부가 정보가 담겨 있음.
         * - 32비트 값 안에 X좌표(하위 16비트), Y좌표(상위 16비트)가 합쳐져 들어있음.
         */
    {
        int mouseX = LOWORD(lParam); // 하위 비트 추출 (X)
        int mouseY = HIWORD(lParam); // 상위 비트 추출 (Y)
        printf("[MOUSE] 왼쪽 클릭됨! 위치: (%d, %d)\n", mouseX, mouseY);
    }
    break;

    case WM_RBUTTONDOWN:
        printf("[MOUSE] 오른쪽 클릭됨!\n");
        break;

        // --- [시스템 메시지 처리] ---
    case WM_DESTROY:
        // 사용자가 'X' 버튼을 눌러 창을 닫으려 할 때 호출됨.
        printf("[SYSTEM] 윈도우 파괴 메시지 수신. 루프를 탈출합니다.\n");
        PostQuitMessage(0);
        break;

    default:
        // 우리가 관심 없는 메시지(창 크기 조절, 포커스 변경 등)는 OS가 기본값으로 처리함.
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

/*
 * [WinMain - 프로그램의 심장부]
 */
// hlnstance
// hPrevInstance
// lpCmdLine
// nCmdShow
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    // (A) 윈도우 클래스 등록: 창의 '설계도'를 OS에 등록함.
    WNDCLASSEXW wcex = { sizeof(WNDCLASSEX) };
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc; // 위에서 정의한 메시지 처리 함수를 연결 (핵심!)
    wcex.hInstance = hInstance;
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszClassName = L"MyLectureClass";

    RegisterClassExW(&wcex);

    // (B) 윈도우 생성: 설계도를 바탕으로 실제 '객체(창)'를 메모리에 만듦.
    // hWnd : 윈도우 컨트롤 -> gpu가 직접그릴수 있는 권한 부여 가능 or 여러개의 윈도우를 가진 앱에서 서로 커뮤니케이션하도록 짜는 등 가능.
    HWND hWnd = CreateWindowW(L"MyLectureClass", L"Input Study Window", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 600, NULL, NULL, hInstance, NULL);

    if (!hWnd) return FALSE;

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    printf("=== Win32 Message System 감시 시작 ===\n");
    printf("창 위에서 키보드나 마우스를 조작해보자.\n\n");

    /*
     * [메시지 루프 (The Message Loop)]
     * 1. GetMessage: OS의 메시지 큐에서 메시지가 올 때까지 '무한 대기'함.
     * 2. TranslateMessage: 키보드 메시지를 문자 데이터(WM_CHAR)로 변환함.
     * 3. DispatchMessage: 이 함수가 실행되면 비로소 'WndProc'이 호출되어 실행됨.
     *
     * ※ 주의: 이 방식은 '이벤트'가 없으면 멈춰 있으므로(Wait),
     * 실시간 게임 루프를 만들 때는 나중에 PeekMessage로 교체하게 됨.
     */
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    printf("\n메시지 루프가 종료되었습니다. 프로그램 끝.\n");
    return (int)msg.wParam;
}