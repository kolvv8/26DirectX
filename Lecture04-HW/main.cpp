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

void MoveCursor(int x, int y) {
    printf("\033[%d;%dH", y, x);
}

class Component {
public:
    class GameObject* pOwner = nullptr;
    bool isStarted = false;

    virtual void Start() = 0;
    virtual void Input() {}
    virtual void Update(float dt) = 0;
    virtual void Render() {}

    virtual ~Component() {}
};

class GameObject {
public:
    std::string name;
    std::vector<Component*> components;

    float x = 0.0f;
    float y = 0.0f;

    GameObject(std::string n) {
        name = n;
    }

    ~GameObject() {
        for (int i = 0; i < (int)components.size(); i++) {
            delete components[i];
        }
    }

    void AddComponent(Component* pComp) {
        pComp->pOwner = this;
        pComp->isStarted = false;
        components.push_back(pComp);
    }
};

class PlayerControl : public Component {
public:
    float speed;
    bool moveUp, moveDown, moveLeft, moveRight;
    int playerType = 0;

    float colorR, colorG, colorB;

    PlayerControl(int type) {
        playerType = type;
    }

    void Start() override {
        speed = 1.0f;
        moveUp = moveDown = moveLeft = moveRight = false;

        if (playerType == 0) {
            pOwner->x = -0.5f; pOwner->y = 0.0f;
            colorR = 1.0f; colorG = 0.0f; colorB = 0.0f;
        }
        else {
            pOwner->x = 0.5f; pOwner->y = 0.0f;
            colorR = 0.0f; colorG = 1.0f; colorB = 0.0f;
        }
        printf("[%s] PlayerControl 기능 시작!\n", pOwner->name.c_str());
    }

    void Input() override {
        if (playerType == 0) {
            moveUp = (GetAsyncKeyState(VK_UP) & 0x8000);
            moveDown = (GetAsyncKeyState(VK_DOWN) & 0x8000);
            moveLeft = (GetAsyncKeyState(VK_LEFT) & 0x8000);
            moveRight = (GetAsyncKeyState(VK_RIGHT) & 0x8000);
        }
        if (playerType == 1) {
            moveUp = (GetAsyncKeyState('W') & 0x8000);
            moveDown = (GetAsyncKeyState('S') & 0x8000);
            moveLeft = (GetAsyncKeyState('A') & 0x8000);
            moveRight = (GetAsyncKeyState('D') & 0x8000);
        }
    }

    void Update(float dt) override {
        if (moveUp)    pOwner->y += speed * dt;
        if (moveDown)  pOwner->y -= speed * dt;
        if (moveLeft)  pOwner->x -= speed * dt;
        if (moveRight) pOwner->x += speed * dt;
    }

    void Render() override {
        Vertex vertices[] = {
            {  0.0f + pOwner->x,  0.2f + pOwner->y, 0.5f, colorR, colorG, colorB, 1.0f },
            {  0.2f + pOwner->x, -0.2f + pOwner->y, 0.5f, colorR, colorG, colorB, 1.0f },
            { -0.2f + pOwner->x, -0.2f + pOwner->y, 0.5f, colorR, colorG, colorB, 1.0f },
        };

        g_pImmediateContext->UpdateSubresource(g_pVBuffer, 0, nullptr, vertices, 0, 0);

        UINT stride = sizeof(Vertex), offset = 0;
        g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pVBuffer, &stride, &offset);
        g_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        g_pImmediateContext->VSSetShader(g_pVertexShader, nullptr, 0);
        g_pImmediateContext->PSSetShader(g_pPixelShader, nullptr, 0);

        g_pImmediateContext->Draw(3, 0);
    }
};

class InfoDisplay : public Component {
public:
    float totalTime = 0.0f;

    void Start() override {
        totalTime = 0.0f;
        printf("[%s] InfoDisplay 기능 시작!\n", pOwner->name.c_str());
    }

    void Update(float dt) override {
        totalTime += dt;
    }

    void Render() override {
        MoveCursor(1, 1);
        printf("System Time: %.2f sec                \n", totalTime);
        printf("P1: Arrows | P2: WASD | F: Fullscreen | ESC: Exit\n");
    }
};

class GameLoop {
public:
    bool isRunning;
    std::vector<GameObject*> gameWorld;
    std::chrono::high_resolution_clock::time_point prevTime;
    float deltaTime;

    void Initialize() {
        isRunning = true;
        gameWorld.clear();
        prevTime = std::chrono::high_resolution_clock::now();
        deltaTime = 0.0f;
    }

    void Input() {
        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) isRunning = false;

        static bool isFKeyPressed = false;
        if (GetAsyncKeyState('F') & 0x8000) {
            if (!isFKeyPressed) {
                BOOL isFullScreen;
                g_pSwapChain->GetFullscreenState(&isFullScreen, nullptr);
                g_pSwapChain->SetFullscreenState(!isFullScreen, nullptr);
                isFKeyPressed = true;
            }
        }
        else {
            isFKeyPressed = false;
        }

        for (int i = 0; i < (int)gameWorld.size(); i++) {
            for (int j = 0; j < (int)gameWorld[i]->components.size(); j++) {
                gameWorld[i]->components[j]->Input();
            }
        }
    }

    void Update() {
        for (int i = 0; i < (int)gameWorld.size(); i++) {
            for (int j = 0; j < (int)gameWorld[i]->components.size(); j++) {
                if (!gameWorld[i]->components[j]->isStarted) {
                    gameWorld[i]->components[j]->Start();
                    gameWorld[i]->components[j]->isStarted = true;
                }
            }
        }

        for (int i = 0; i < (int)gameWorld.size(); i++) {
            for (int j = 0; j < (int)gameWorld[i]->components.size(); j++) {
                gameWorld[i]->components[j]->Update(deltaTime);
            }
        }
    }

    void Render() {
        float clearColor[] = { 0.1f, 0.2f, 0.3f, 1.0f };
        g_pImmediateContext->ClearRenderTargetView(g_pRenderTargetView, clearColor);
        g_pImmediateContext->OMSetRenderTargets(1, &g_pRenderTargetView, nullptr);

        D3D11_VIEWPORT vp = { 0, 0, 800, 600, 0.0f, 1.0f };
        g_pImmediateContext->RSSetViewports(1, &vp);
        g_pImmediateContext->IASetInputLayout(g_pInputLayout);

        for (int i = 0; i < (int)gameWorld.size(); i++) {
            for (int j = 0; j < (int)gameWorld[i]->components.size(); j++) {
                gameWorld[i]->components[j]->Render();
            }
        }

        g_pSwapChain->Present(1, 0);
    }

    void Run() {
        MSG msg = { 0 };
        while (isRunning) {
            if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
                if (msg.message == WM_QUIT) {
                    break;
                }
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
            else {
                std::chrono::high_resolution_clock::time_point currentTime = std::chrono::high_resolution_clock::now();
                std::chrono::duration<float> elapsed = currentTime - prevTime;
                deltaTime = elapsed.count();
                prevTime = currentTime;

                Input();
                Update();
                Render();

                std::this_thread::sleep_for(std::chrono::milliseconds(2));
            }
        }
    }

    GameLoop() { Initialize(); }
    ~GameLoop() {
        for (int i = 0; i < (int)gameWorld.size(); i++) {
            delete gameWorld[i];
        }
    }
};

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    if (message == WM_DESTROY) { PostQuitMessage(0); return 0; }
    return DefWindowProc(hWnd, message, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    WNDCLASSEXW wcex = { sizeof(WNDCLASSEX) };
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = hInstance;
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.lpszClassName = L"DX11ComponentEngine";
    RegisterClassExW(&wcex);

    HWND hWnd = CreateWindowW(L"DX11ComponentEngine", L"과제: 컴포넌트 기반 게임 오브젝트 시스템 구현",
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

    D3D11_BUFFER_DESC bd = { sizeof(Vertex) * 3, D3D11_USAGE_DEFAULT, D3D11_BIND_VERTEX_BUFFER, 0, 0, 0 };
    g_pd3dDevice->CreateBuffer(&bd, nullptr, &g_pVBuffer);

    GameLoop gLoop;

    GameObject* sysInfo = new GameObject("SystemManager");
    sysInfo->AddComponent(new InfoDisplay());
    gLoop.gameWorld.push_back(sysInfo);

    GameObject* player1 = new GameObject("Player1");
    player1->AddComponent(new PlayerControl(0));
    gLoop.gameWorld.push_back(player1);

    GameObject* player2 = new GameObject("Player2");
    player2->AddComponent(new PlayerControl(1));
    gLoop.gameWorld.push_back(player2);

    gLoop.Run();

    g_pVBuffer->Release();
    g_pInputLayout->Release();
    g_pVertexShader->Release();
    g_pPixelShader->Release();
    g_pRenderTargetView->Release();
    g_pSwapChain->Release();
    g_pImmediateContext->Release();
    g_pd3dDevice->Release();

    return 0;
}