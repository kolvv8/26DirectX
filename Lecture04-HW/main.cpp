/*
================================================================================
 [가이드: 게임 엔진의 뼈대 만들기]
================================================================================
 1. Component (기능): 캐릭터가 할 수 있는 '일' (이동, 시간 재기 등)
 2. GameObject (객체): 게임에 존재하는 '물체' (플레이어, 타이머 등)
 3. GameWorld (세계): 모든 물체를 담고 있는 '바구니'

 * 구조: Component -> GameObject -> GameWorld 순으로 확장됨.
         (루프 한 번 돌 때 [입력 -> 업데이트 -> 렌더링] 순서로 모든 객체를 훑음.)
 [작동 원리]
 - Start(): 물체가 태어날 때 딱 한 번 실행되는 초기화 코드
 - Input(): 키보드/마우스 상태를 확인.
 - Update(): 수치(좌표 등)를 계산.
 - Render(): 화면에 결과를 출력.

================================================================================
*/

#include <iostream>
#include <chrono>
#include <thread>
#include <windows.h>
#include <vector>
#include <string>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <stdio.h>
#pragma comment(linker, "/entry:WinMainCRTStartup /subsystem:console")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

struct Vertex {
    float x, y, z;
    float r, g, b, a;
};

struct GameContext {
    float posX;
    float posY;
    float speed;

    bool keyUp;
    bool keyDown;
    bool keyLeft;
    bool keyRight;
};

GameContext* g_pGame = nullptr;

ID3D11Device* g_pd3dDevice = nullptr;
ID3D11DeviceContext* g_pImmediateContext = nullptr;
IDXGISwapChain* g_pSwapChain = nullptr;
ID3D11RenderTargetView* g_pRenderTargetView = nullptr;
ID3D11Buffer* g_pVBuffer = nullptr;
ID3D11InputLayout* g_pInputLayout = nullptr;
ID3D11VertexShader* g_pVertexShader = nullptr;
ID3D11PixelShader* g_pPixelShader = nullptr;

const char* shaderSource = R"(
struct VS_INPUT { float3 pos : POSITION; float4 col : COLOR; }; 
struct PS_INPUT { float4 pos : SV_POSITION; float4 col : COLOR; };
PS_INPUT VS(VS_INPUT input) {
    PS_INPUT output;
    output.pos = float4(input.pos, 1.0f);
    output.col = input.col;
    return output;
}
float4 PS(PS_INPUT input) : SV_Target { return input.col; }
)";

// 콘솔의 특정 좌표로 커서를 이동시키는 함수
void MoveCursor(int x, int y)
{
    // \033[y;xH  (y와 x는 1부터 시작함)
    printf("\033[%d;%dH", y, x);
}

// [1단계: 컴포넌트 기저 클래스]
// 모든 기능(이동, 렌더링 등)은 이 클래스를 상속받아야 함.
class Component
{
public:
    class GameObject* pOwner = nullptr; // 이 기능이 누구의 것인지 저장
    bool isStarted = 0;           // Start()가 실행되었는지 체크

    virtual void Start() = 0;              // 초기화
    virtual void Input() {}                // 입력 (선택사항)
    virtual void Update(float dt) = 0;     // 로직 (필수)
    virtual void Render() {}               // 그리기 (선택사항)

    virtual ~Component() {}
};

// [2단계: 게임 오브젝트 클래스]
// 컴포넌트들을 담는 바구니 역할을 함.
class GameObject {
public:
    std::string name;
    std::vector<Component*> components;

    GameObject(std::string n)
    {
        name = n;
    }

    // 객체가 죽을 때 담고 있던 컴포넌트들도 메모리에서 해제함
    ~GameObject() {
        for (int i = 0; i < (int)components.size(); i++)
        {
            delete components[i];
        }
    }

    // 새로운 기능을 추가하는 함수
    void AddComponent(Component* pComp)
    {
        pComp->pOwner = this;
        pComp->isStarted = false;
        components.push_back(pComp);
    }
};

// --- [3단계: 실제 구현할 기능 컴포넌트들] ---

// 기능 1: 플레이어 조종 및 이동
class PlayerControl : public Component {
public:
    float x, y, speed;
    bool moveUp, moveDown, moveLeft, moveRight;
    int playerType = 0;

    PlayerControl(int type)
    {
        playerType = type;
    }
    void Start() override
    {
        x = 50.0f; y = 50.0f; speed = 150.0f;
        moveUp = moveDown = moveLeft = moveRight = false;
        printf("[%s] PlayerControl 기능 시작!\n", pOwner->name.c_str());
    }

    // [입력 단계] 키 상태만 체크함
    void Input() override
    {
        if (playerType == 0)
        {
            moveUp = (GetAsyncKeyState('W') & 0x8000);
            moveDown = (GetAsyncKeyState('S') & 0x8000);
            moveLeft = (GetAsyncKeyState('A') & 0x8000);
            moveRight = (GetAsyncKeyState('D') & 0x8000);
        }
        if (playerType == 1)
        {
            moveUp = (GetAsyncKeyState(VK_UP) & 0x8000);
            moveDown = (GetAsyncKeyState(VK_DOWN) & 0x8000);
            moveLeft = (GetAsyncKeyState(VK_LEFT) & 0x8000);
            moveRight = (GetAsyncKeyState(VK_RIGHT) & 0x8000);
        }
    }

    // [업데이트 단계] 체크된 키 상태로 좌표만 계산함
    void Update(float dt) override
    {
        if (moveUp)    y -= speed * dt;
        if (moveDown)  y += speed * dt;
        if (moveLeft)  x -= speed * dt;
        if (moveRight) x += speed * dt;
    }

    // [렌더링 단계] 계산된 좌표를 화면에 그림
    void Render() override
    {
        Vertex vertices[] = {
            {  0.0f + x,  0.6f + y, 0.5f, 0.0f, 1.0f, 0.0f, 1.0f },
            {  0.4f + x, -0.3f + y, 0.5f, 0.0f, 1.0f, 0.0f, 1.0f },
            { -0.4f + x, -0.3f + y, 0.5f, 0.0f, 1.0f, 0.0f, 1.0f },
        };

        g_pImmediateContext->UpdateSubresource(g_pVBuffer, 0, nullptr, vertices, 0, 0);

        float clearColor[] = { 0.1f, 0.2f, 0.3f, 1.0f };
        g_pImmediateContext->ClearRenderTargetView(g_pRenderTargetView, clearColor);
        g_pImmediateContext->OMSetRenderTargets(1, &g_pRenderTargetView, nullptr);

        D3D11_VIEWPORT vp = { 0, 0, 800, 600, 0.0f, 1.0f };
        g_pImmediateContext->RSSetViewports(1, &vp);
        g_pImmediateContext->IASetInputLayout(g_pInputLayout);

        UINT stride = sizeof(Vertex), offset = 0;
        g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pVBuffer, &stride, &offset);
        g_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        g_pImmediateContext->VSSetShader(g_pVertexShader, nullptr, 0);
        g_pImmediateContext->PSSetShader(g_pPixelShader, nullptr, 0);

        g_pImmediateContext->Draw(3, 0);

        g_pSwapChain->Present(1, 0);
    }
};

// 기능 2: 시스템 정보 출력 (위치 정보 없음)
class InfoDisplay : public Component
{
public:
    float totalTime = 0.0f;

    void Start() override
    {
        totalTime = 0.0f;
        printf("[%s] InfoDisplay 기능 시작!\n", pOwner->name.c_str());
    }

    void Update(float dt) override
    {
        totalTime += dt;
    }

    void Render() override {
        // 화면 최상단에 정보 출력
        MoveCursor(0, 0);
        printf("System Time: %.2f sec\n", totalTime);
        printf("Control: W, A, S, D | Exit: ESC\n");
    }
};



class GameLoop
{
public:
    bool isRunning;
    std::vector<GameObject*> gameWorld;
    std::chrono::high_resolution_clock::time_point prevTime;
    float deltaTime;   //delta time;

    //초기화
    void Initialize()
    {
        //초기화시 동작준비됨
        isRunning = true;

        gameWorld.clear();

        // 시간 측정 준비
        prevTime = std::chrono::high_resolution_clock::now();
        deltaTime = 0.0f;


    }

    void Input()
    {
        // esc 누르면 종료
        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) isRunning = false;

        // B. 입력 단계 (Input Phase)
        for (int i = 0; i < (int)gameWorld.size(); i++)
        {
            for (int j = 0; j < (int)gameWorld[i]->components.size(); j++)
            {
                gameWorld[i]->components[j]->Input();
            }
        }
    }

    void Update()
    {
        // C. 스타트 실행
        for (int i = 0; i < (int)gameWorld.size(); i++)
        {
            for (int j = 0; j < (int)gameWorld[i]->components.size(); j++)
            {
                // Start()가 호출된 적 없다면 여기서 호출 (유니티 방식)
                if (gameWorld[i]->components[j]->isStarted == false)
                {
                    gameWorld[i]->components[j]->Start();
                    gameWorld[i]->components[j]->isStarted = true;
                }
            }
        }

        // D. 업데이트 단계 (Update Phase)
        for (int i = 0; i < (int)gameWorld.size(); i++)
        {
            for (int j = 0; j < (int)gameWorld[i]->components.size(); j++)
            {
                gameWorld[i]->components[j]->Update(deltaTime);
            }
        }
    }

    void Render()
    {
        // E. 렌더링 단계 (Render Phase)
        system("cls");
        for (int i = 0; i < (int)gameWorld.size(); i++)
        {
            for (int j = 0; j < (int)gameWorld[i]->components.size(); j++)
            {
                gameWorld[i]->components[j]->Render();
            }
        }
    }



    void Run()
    {
        // --- [무한 게임 루프] ---
        while (isRunning) {

            // A. 시간 관리 (DeltaTime 계산)
            std::chrono::high_resolution_clock::time_point currentTime = std::chrono::high_resolution_clock::now();
            std::chrono::duration<float> elapsed = currentTime - prevTime;
            deltaTime = elapsed.count();
            prevTime = currentTime;

            Input();
            Update();
            Render();

            // CPU 과부하 방지 (약 60~100 FPS 유지 시도)
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    GameLoop()
    {
        Initialize();
    }
    ~GameLoop()
    {
        // [정리] 메모리 해제
        for (int i = 0; i < (int)gameWorld.size(); i++)
        {
            delete gameWorld[i]; // GameObject 소멸자가 컴포넌트들도 지움
        }
    }

};

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    if (message == WM_DESTROY) { PostQuitMessage(0); return 0; }
    return DefWindowProc(hWnd, message, wParam, lParam);
}

// --- [4단계: 메인 엔진 루프] ---
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    GameContext game = { 0 };
    game.posX = 0.0f;
    game.posY = 0.0f;
    game.speed = 0.01f;

    g_pGame = &game;

    WNDCLASSEXW wcex = { sizeof(WNDCLASSEX) };
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = hInstance;
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.lpszClassName = L"DX11MovingHexagram";
    RegisterClassExW(&wcex);

    HWND hWnd = CreateWindowW(L"DX11MovingHexagram", L"과제: 움직이는 육망성 만들기",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 800, 600, nullptr, nullptr, hInstance, nullptr);
    ShowWindow(hWnd, nCmdShow);

    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 1;
    sd.BufferDesc.Width = 800;
    sd.BufferDesc.Height = 600;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;

    D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0,
        D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, nullptr, &g_pImmediateContext);

    ID3D11Texture2D* pBackBuffer = nullptr;
    g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBackBuffer);
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_pRenderTargetView);
    pBackBuffer->Release();

    ID3DBlob* vsBlob, * psBlob;
    D3DCompile(shaderSource, strlen(shaderSource), nullptr, nullptr, nullptr, "VS", "vs_4_0", 0, 0, &vsBlob, nullptr);
    D3DCompile(shaderSource, strlen(shaderSource), nullptr, nullptr, nullptr, "PS", "ps_4_0", 0, 0, &psBlob, nullptr);
    g_pd3dDevice->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &g_pVertexShader);
    g_pd3dDevice->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &g_pPixelShader);

    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    g_pd3dDevice->CreateInputLayout(layout, 2, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &g_pInputLayout);
    vsBlob->Release(); psBlob->Release();

    D3D11_BUFFER_DESC bd = { sizeof(Vertex) * 6, D3D11_USAGE_DEFAULT, D3D11_BIND_VERTEX_BUFFER, 0, 0, 0 };
    g_pd3dDevice->CreateBuffer(&bd, nullptr, &g_pVBuffer);

    //게임루프
    GameLoop gLoop;
    gLoop.Initialize();

    // 시스템 정보 객체 조립
    GameObject* sysInfo = new GameObject("SystemManager");
    sysInfo->AddComponent(new InfoDisplay());
    gLoop.gameWorld.push_back(sysInfo);

    // 플레이어 객체 조립
    GameObject* player1 = new GameObject("Player1");
    player1->AddComponent(new PlayerControl(0));
    gLoop.gameWorld.push_back(player1);

    // 플레이어 객체 조립
    GameObject* player2 = new GameObject("Player2");
    player2->AddComponent(new PlayerControl(1));
    gLoop.gameWorld.push_back(player2);


    //게임루프 실행
    gLoop.Run();



    return 0;
}