#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <stdio.h>

#pragma comment(linker, "/entry:WinMainCRTStartup /subsystem:console")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

ID3D11Device* g_pd3dDevice = nullptr;
ID3D11DeviceContext* g_pImmediateContext = nullptr;
IDXGISwapChain* g_pSwapChain = nullptr;
ID3D11RenderTargetView* g_pRenderTargetView = nullptr;

bool g_keyUp = false;
bool g_keyDown = false;
bool g_keyLeft = false;
bool g_keyRight = false;

float posX = 0.0f, posY = 0.0f;
float speed = 0.01f;

struct Vertex {
    float x, y, z;
    float r, g, b, a;
};

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


LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{

    switch (message) {
    case WM_KEYDOWN:
        if (wParam == VK_UP) g_keyUp = true;
        else if (wParam == VK_DOWN) g_keyDown = true;
        else if (wParam == VK_LEFT) g_keyLeft = true;
        else if (wParam == VK_RIGHT) g_keyRight = true;
        break;

    case WM_KEYUP:
        if (wParam == VK_UP) g_keyUp = false;
        else if (wParam == VK_DOWN) g_keyDown = false;
        else if (wParam == VK_LEFT) g_keyLeft = false;
        else if (wParam == VK_RIGHT) g_keyRight = false;
        break;

    case WM_DESTROY:
        printf("[SYSTEM] 윈도우 파괴 메시지 수신. 루프를 탈출합니다.\n");
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    WNDCLASSEXW wcex = { sizeof(WNDCLASSEX) };
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = hInstance;
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.lpszClassName = L"DX11MovingTriangle";
    RegisterClassExW(&wcex);

    HWND hWnd = CreateWindowW(L"DX11MovingTriangle", L"DirectX11 삼각형 이동",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 800, 600, nullptr, nullptr, hInstance, nullptr);
    ShowWindow(hWnd, nCmdShow);

    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 1;
    sd.BufferDesc.Width = 800; sd.BufferDesc.Height = 600;
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
    ID3D11VertexShader* vShader; ID3D11PixelShader* pShader;
    g_pd3dDevice->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &vShader);
    g_pd3dDevice->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &pShader);

    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    ID3D11InputLayout* pInputLayout;
    g_pd3dDevice->CreateInputLayout(layout, 2, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &pInputLayout);
    vsBlob->Release(); psBlob->Release();

    ID3D11Buffer* pVBuffer;
    D3D11_BUFFER_DESC bd = { sizeof(Vertex) * 6, D3D11_USAGE_DEFAULT, D3D11_BIND_VERTEX_BUFFER, 0, 0, 0 };
    g_pd3dDevice->CreateBuffer(&bd, nullptr, &pVBuffer);

    MSG msg = { 0 };
    while (WM_QUIT != msg.message) {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else {
            if (g_keyUp)    posY += speed;
            if (g_keyDown)  posY -= speed;
            if (g_keyLeft)  posX -= speed;
            if (g_keyRight) posX += speed;

            Vertex vertices[] = {
                {  0.0f + posX,  0.6f + posY, 0.5f, 0.0f, 1.0f, 0.0f, 1.0f },
                {  0.4f + posX, -0.3f + posY, 0.5f, 0.0f, 1.0f, 0.0f, 1.0f },
                { -0.4f + posX, -0.3f + posY, 0.5f, 0.0f, 1.0f, 0.0f, 1.0f },

                {  0.0f + posX, -0.6f + posY, 0.5f, 0.0f, 1.0f, 0.0f, 1.0f },
                { -0.4f + posX,  0.3f + posY, 0.5f, 0.0f, 1.0f, 0.0f, 1.0f },
                {  0.4f + posX,  0.3f + posY, 0.5f, 0.0f, 1.0f, 0.0f, 1.0f },
            };

            g_pImmediateContext->UpdateSubresource(pVBuffer, 0, nullptr, vertices, 0, 0);

            float clearColor[] = { 0.1f, 0.2f, 0.3f, 1.0f };
            g_pImmediateContext->ClearRenderTargetView(g_pRenderTargetView, clearColor);
            g_pImmediateContext->OMSetRenderTargets(1, &g_pRenderTargetView, nullptr);

            D3D11_VIEWPORT vp = { 0, 0, 800, 600, 0.0f, 1.0f };
            g_pImmediateContext->RSSetViewports(1, &vp);
            g_pImmediateContext->IASetInputLayout(pInputLayout);
            UINT stride = sizeof(Vertex), offset = 0;
            g_pImmediateContext->IASetVertexBuffers(0, 1, &pVBuffer, &stride, &offset);
            g_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            g_pImmediateContext->VSSetShader(vShader, nullptr, 0);
            g_pImmediateContext->PSSetShader(pShader, nullptr, 0);

            g_pImmediateContext->Draw(6, 0);
            g_pSwapChain->Present(1, 0);
        }
    }

    if (pVBuffer) pVBuffer->Release();
    if (pInputLayout) pInputLayout->Release();
    if (vShader) vShader->Release();
    if (pShader) pShader->Release();
    if (g_pRenderTargetView) g_pRenderTargetView->Release();
    if (g_pSwapChain) g_pSwapChain->Release();
    if (g_pImmediateContext) g_pImmediateContext->Release();
    if (g_pd3dDevice) g_pd3dDevice->Release();

    return (int)msg.wParam;
}