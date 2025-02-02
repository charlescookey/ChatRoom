// Dear ImGui: standalone example application for DirectX 11

// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp
#include "Network.h"

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <d3d11.h>
#include <tchar.h>


#include <vector>
#include <sstream>
#include <queue>
#include <mutex>


#include "DM.h"

#include "Sound.h"

#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "D3D11.lib")
#pragma comment(lib, "D3DCompiler.lib")
#pragma comment(lib, "DXGI.lib")
#pragma comment(lib, "ws2_32.lib")




// Data
static ID3D11Device*            g_pd3dDevice = nullptr;
static ID3D11DeviceContext*     g_pd3dDeviceContext = nullptr;
static IDXGISwapChain*          g_pSwapChain = nullptr;
static bool                     g_SwapChainOccluded = false;
static UINT                     g_ResizeWidth = 0, g_ResizeHeight = 0;
static ID3D11RenderTargetView*  g_mainRenderTargetView = nullptr;

// Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);


static void OpenDM(std::string name, User& user, std::unordered_map<std::string, DM>& DMs, Network& net)
{
    //static DM console;
    //console.Draw( user);

    DMs[user.name].Draw(name, user, net);
}

void parseUserList(std::string userList , std::vector<User>& Users) {
    Users.clear();
    std::stringstream ss(userList);
    std::string user;
    while (std::getline(ss, user, delimiter)) {
        if (user.empty())continue;
        Users.push_back({ user , false});
    }
}

void parseGroupMessage(std::string message, std::vector <std::string>& GroupMessage) {
    GroupMessage.push_back(message);
}

void parsePrivateMessage(std::string message, std::unordered_map<std::string, DM>& DMs) {
    std::string sender;

    size_t start = 0;
    size_t end = message.find(delimiter);

    sender = message.substr(start, end - start);
    

    start = end + 1;
    message = message.substr(start);

    if (DMs.count(sender) <= 0) {
        DMs[sender] = DM();
    }
    DMs[sender].Items.push_back(message);
}

int parseServerMessage(std::string message, std::vector<User>& Users , std::vector <std::string>& GroupMessage, std::unordered_map<std::string, DM>& DMs, Sound& sound) {
    if (message.empty())return 0;
    std::string temp;

    size_t start = 0;
    size_t end = message.find(delimiter);

    temp = message.substr(start, end - start);
    int type = std::stoi(temp);

    start = end + 1;
    temp = message.substr(start);

    switch (type) {
    case 1:
        parseUserList(temp, Users);
        break;
    case 2:
        parseGroupMessage(temp, GroupMessage);
        sound.playGMSound();
        break;
    case 3:
        parsePrivateMessage(temp, DMs);
        sound.playPMSound();
        break;
    case 4:
        parseGroupMessage(temp, GroupMessage);
        break;
    case 5:
        return 1;
    case 6:
        return -1;
    case 7:
        parsePrivateMessage(temp, DMs);
        break;
    }
    return 0;
}

std::queue<std::string> messageQueue;
std::mutex queueMutex;


void recieverThread(Network& net) {
    std::string message;
    std::string stopMessage = "!bye" + delimiter + std::string("!stop");
    while (true) {
        if (net.receiveMessage(message) == -1)break;

        {
            std::lock_guard<std::mutex> lock(queueMutex);
            messageQueue.push(message);
        }

        if (message == stopMessage)break;
    }
    std::cout << "Reciver thread stopped\n";
}

// Main code
int main(int, char**)
{
    // Create application window
    //ImGui_ImplWin32_EnableDpiAwareness();
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"ImGui Example", nullptr };
    ::RegisterClassExW(&wc);
    HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"Chat Room", WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, nullptr, nullptr, wc.hInstance, nullptr);

    std::string name;

    // Initialize Direct3D
    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    // Show the window
    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    char                  InputBuf[256];
    memset(InputBuf, 0, sizeof(InputBuf));
    std::vector <std::string> GroupMessage;
    std::vector <User>       Users;
    std::unordered_map<std::string, DM> DMs;

    Network net;
    if (net.initialize() != 0) {
        std::cout << "Couldn't connect to server\n";
        return -1;
    }
    
    //net.sendMessage(std::string("1") + delimiter + name);

    // Our state
    bool show_demo_window = true;
    bool show_another_window = false;
    bool sendMessage = false;
    bool open_dm = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    bool window_open = false;
    bool login_open = true;
    bool stopped = false;

    bool login_error = false;
    std::string login_error_message{};

    Sound sound;

    // Main loop
    bool done = false;

    //start a thread to recieve messages
    std::thread* receivingThread = new std::thread(recieverThread , std::ref(net));
    while (!done)
    {
        // Poll and handle messages (inputs, window resize, etc.)
        // See the WndProc() function below for our to dispatch events to the Win32 backend.

        while (!messageQueue.empty()) {
            std::string message = messageQueue.front();
            messageQueue.pop();
            std::cout << "Message received by "<<name<<": " << message << std::endl;
            int ret = parseServerMessage(message, Users, GroupMessage, DMs, sound);
            if (ret == -1){
                login_error_message = "Username already in use";
                login_error = true;
            }else if (ret == 1) {
                login_open = false;
                window_open = true;
            }
        }

        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;

        // Handle window being minimized or screen locked
        if (g_SwapChainOccluded && g_pSwapChain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED)
        {
            ::Sleep(10);
            continue;
        }
        g_SwapChainOccluded = false;

        // Handle window resize (we don't resize directly in the WM_SIZE handler)
        if (g_ResizeWidth != 0 && g_ResizeHeight != 0)
        {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
            g_ResizeWidth = g_ResizeHeight = 0;
            CreateRenderTarget();
        }

        // Start the Dear ImGui frame
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        //ImGui::SetNextWindowSize(ImVec2(500, 300));
        if (login_open) {
            if (ImGui::Begin("Login", &login_open, ImGuiWindowFlags_AlwaysAutoResize)) {   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
                ImGui::Text("Please enter your username!");
                if (login_error) {
                    ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), login_error_message.c_str());
                }
                if (ImGui::InputText("##hidden", InputBuf, IM_ARRAYSIZE(InputBuf)))
                {

                }
                ImGui::SameLine();
                if (ImGui::Button("Login")) {
                    name = std::string(InputBuf);
                    if (!name.empty()) {
                        net.sendMessage(std::string("1") + delimiter + name);
                        memset(InputBuf, 0, sizeof(InputBuf));
                        login_error = false;
                    }
                    else {
                        login_error_message = "Username cannot be empty!";
                        login_error = true;
                    }
                }
            }
            ImGui::End();
        }
        ImGui::SetNextWindowSizeConstraints(ImVec2(424, 208), ImVec2(FLT_MAX, FLT_MAX));
        const float footer_height_to_reserve = ImGui::GetStyle().ItemSpacing.y * 2 + ImGui::GetFrameHeightWithSpacing();
        if (window_open) {
            if (ImGui::Begin("Chat Client", &window_open)) {

                const float children_width = ImGui::GetWindowWidth()/2 - ImGui::GetStyle().ItemSpacing.x * 2 ;

                if (ImGui::BeginChild("UsersRegion", ImVec2(children_width, -footer_height_to_reserve), true, ImGuiWindowFlags_None)) {
                    ImGui::TextUnformatted("Users:");
                    ImGui::Separator();
                    for (int i =0; i < Users.size() ; i++)
                    {
                        if (Users[i].name == name)continue;
                        ImGui::Selectable(Users[i].name.c_str(), &Users[i].selected);
                    }
                }ImGui::EndChild();
                ImGui::SameLine();
                if (ImGui::BeginChild("TextRegion", ImVec2(children_width, -footer_height_to_reserve), true, ImGuiWindowFlags_None)) {
                    for (std::string item : GroupMessage)
                    {
                        ImGui::TextUnformatted(item.c_str());
                    }
                }ImGui::EndChild();
                if (ImGui::InputText("Message", InputBuf, IM_ARRAYSIZE(InputBuf)))
                {

                }
                ImGui::SameLine();
                if (ImGui::SmallButton("Send")) {
                    sendMessage = true;
                }
                else {
                    sendMessage = false;
                }
            }ImGui::End();
        }
        else if (!window_open && !login_open) {
            //send disconnect message here
            stopped = true;
            std::string stopMessage = std::string("5") + delimiter + "bye" + delimiter + name;
            std::cout << "Sent disconnect\n";
            net.sendMessage(stopMessage);
            break;
        }

        for (int i = 0; i < Users.size(); i++)
        {
            if (Users[i].selected) {
                OpenDM(name , Users[i], DMs, net);
            }
        }
        if (sendMessage) {
            std::string s(InputBuf);
            if (!s.empty()) {
                s = std::string("2") + delimiter + name + ": " + s;
                net.sendMessage(s);
            }
            memset(InputBuf, 0, sizeof(InputBuf));
        }

        // Rendering
        ImGui::Render();
        const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        // Present
        HRESULT hr = g_pSwapChain->Present(1, 0);   // Present with vsync
        //HRESULT hr = g_pSwapChain->Present(0, 0); // Present without vsync
        g_SwapChainOccluded = (hr == DXGI_STATUS_OCCLUDED);
    }

    // Cleanup

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);


    if (!stopped) {
        std::string stopMessage = std::string("5") + delimiter + "bye" + delimiter + name;
        std::cout << "Sent disconnect\n";
        net.sendMessage(stopMessage);
    }
    receivingThread->join();
    return 0;
}

// Helper functions

bool CreateDeviceD3D(HWND hWnd)
{
    // Setup swap chain
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    //createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res == DXGI_ERROR_UNSUPPORTED) // Try high-performance WARP software driver if hardware is not available.
        res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

void CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Win32 message handler
// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED)
            return 0;
        g_ResizeWidth = (UINT)LOWORD(lParam); // Queue resize
        g_ResizeHeight = (UINT)HIWORD(lParam);
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}
