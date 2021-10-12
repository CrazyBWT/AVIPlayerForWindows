// AVIPlayer.cpp : 定义应用程序的入口点。
//

#include "framework.h"
#include "AVIPlayer.h"
#include "Avi.h"

#define MAX_LOADSTRING 100

// 全局变量:
HINSTANCE hInst;                                // 当前实例
WCHAR szTitle[MAX_LOADSTRING];                  // 标题栏文本
WCHAR szWindowClass[MAX_LOADSTRING];            // 主窗口类名
LPDIRECT3D9				g_pD3D;
LPDIRECT3DDEVICE9		g_pd3dDevice;
LPD3DXSPRITE			g_pSpirit;
CAvi				m_xAvi;

// 此代码模块中包含的函数的前向声明:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
VOID                Display();

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: 在此处放置代码。

    // 初始化全局字符串
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_AVIPLAYER, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // 执行应用程序初始化:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }
  
    m_xAvi.Init();
    m_xAvi.Create(g_pd3dDevice, NULL, ".\\wemade.dat", FALSE);
    m_xAvi.SetDestRect(0, 60, 640, 420);
    m_xAvi.Start();
    
    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_AVIPLAYER));

    MSG msg;
    ::ZeroMemory(&msg, sizeof(MSG));

    // 主消息循环:
    while (msg.message != WM_QUIT)
    {
        if (::PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
        }
        else
        {
            Display();
        }
    }
    return (int) msg.wParam;
}

VOID                Display()
{
    if (g_pd3dDevice)
    {
        //
        // Render
        //

        g_pd3dDevice->Clear(0, 0, D3DCLEAR_TARGET, 0, 0, 0);

        g_pd3dDevice->BeginScene();
        
        if (!m_xAvi.Draw(NULL))
        {
            Sleep(100);

            m_xAvi.Release();
        }
        g_pd3dDevice->EndScene();
        g_pd3dDevice->Present(0, 0, 0, 0);
    }
}

//
//  函数: MyRegisterClass()
//
//  目标: 注册窗口类。
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_AVIPLAYER));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_AVIPLAYER);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   函数: InitInstance(HINSTANCE, int)
//
//   目标: 保存实例句柄并创建主窗口
//
//   注释:
//
//        在此函数中，我们在全局变量中保存实例句柄并
//        创建和显示主程序窗口。
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // 将实例句柄存储在全局变量中

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, 640, 480, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   HRESULT hr;
   g_pD3D = Direct3DCreate9(D3D_SDK_VERSION);

   D3DPRESENT_PARAMETERS d3dpp;
   ZeroMemory(&d3dpp, sizeof(d3dpp));
   d3dpp.BackBufferWidth = 640;					//后台缓冲区宽
   d3dpp.BackBufferHeight = 480;					//后台缓冲区高
   d3dpp.BackBufferFormat = D3DFMT_A8R8G8B8;		//后台缓冲区像素格式
   d3dpp.BackBufferCount = 1;						//后台缓冲区数量
   d3dpp.MultiSampleType = D3DMULTISAMPLE_NONE;	//多重采样类型
   d3dpp.MultiSampleQuality = 0;					//多重采样格式
   d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;		//默认D3DSWAPEFFECT_DISCARD，表示后台缓冲区复制到前台时清除后台缓冲区内容
   d3dpp.hDeviceWindow = hWnd;						//指定绘制图形的窗口句柄
   d3dpp.Windowed = true;							//true表示试用窗口、false表示使用全屏
   //这里如果开启EnableAutoDepthStencil ， 那么g_pd3dDevice->Clear的时候，一定要带上D3DCLEAR_ZBUFFER
   d3dpp.EnableAutoDepthStencil = false;			//是否自动管理深度缓存
   d3dpp.AutoDepthStencilFormat = D3DFMT_D24S8;	//深度缓存的像素格式
   d3dpp.Flags = 0;								//
   d3dpp.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;	//默认值 D3DPRESENT_RATE_DEFAULT
   d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;	//垂直同步

   if (FAILED(hr = g_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &d3dpp, &g_pd3dDevice)))
       return FALSE;

   if (FAILED(D3DXCreateSprite(g_pd3dDevice, &g_pSpirit)))
   {
       return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  函数: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  目标: 处理主窗口的消息。
//
//  WM_COMMAND  - 处理应用程序菜单
//  WM_PAINT    - 绘制主窗口
//  WM_DESTROY  - 发送退出消息并返回
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_DESTROY:
        ::PostQuitMessage(0);
        break;

    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE)
            ::DestroyWindow(hWnd);
        break;
    }
    return ::DefWindowProc(hWnd, message, wParam, lParam);
}

// “关于”框的消息处理程序。
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
