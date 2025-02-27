// example how to set up D3D11 rendering on Windows in C
// require Windows 7 Platform Update or newer Windows version

#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <d3dcompiler.h>

#define _USE_MATH_DEFINES
#include <math.h>
#include <string.h>
#include <stddef.h>
#include <stdbool.h>
#include <time.h>
#include <stdio.h>

// replace this with your favorite Assert() implementation
#include <intrin.h>
#define Assert(cond) do { if (!(cond)) __debugbreak(); } while (0)
#define AssertHR(hr) Assert(SUCCEEDED(hr))

#pragma comment (lib, "gdi32.lib")
#pragma comment (lib, "user32.lib")
#pragma comment (lib, "dxguid.lib")
#pragma comment (lib, "d3d11.lib")
#pragma comment (lib, "dxgi.lib")
#pragma comment (lib, "d3dcompiler.lib")

#define STR2(x) #x
#define STR(x) STR2(x)

#define PI 3.14159265f

#undef NDEBUG

//#define LOGGING

typedef struct Float3 {
  float x;
  float y;
  float z;
} Float3;

typedef struct Float4 {
  float x;
  float y;
  float z;
  float a;
} Float4;

typedef struct Matrix {
    float m[4][4];
} Matrix;

typedef struct {
    Matrix projection;
    Matrix view;
    float color[3];
    float light[3]; 
} Constants;

typedef struct {
    Matrix transform;
    float color[3];
} Instance;

typedef struct Camera {
    float rotation[3];
    float position[3];
} Camera;

typedef struct Cube {
    float rotation[3];
    float scale[3];
    float position[3];
    float color[3];
    float length;
    bool alive;
} Cube;

void print_matrix(Matrix m)
{
    for (int i = 0; i < 4; i += 1)
    {
        for (int j = 0; j < 4; j += 1)
        {
            printf("%f ", m.m[i][j]);
        }
        printf("\n");
    }
}

int check(Cube *cubes, int rows, int columns, int r, int c)
{
    if (r < 0 || r >= rows ||
        c < 0 || c >= columns)
    {
        return 0;
    }

    if (cubes[r*columns + c].alive)
    {
        return 1;
    }

    return 0;
}

Float3 float3_normalize(Float3 f)
{
    float magnitude = sqrt(f.x*f.x + f.y*f.y + f.z*f.z);

    Float3 result;
    result.x = f.x/magnitude;
    result.y = f.y/magnitude;
    result.z = f.z/magnitude;

    return result;
}

Float3 float3_subtract(Float3 a, Float3 b)
{
    Float3 result;
    result.x = a.x - b.x; 
    result.y = a.y - b.y; 
    result.z = a.z - b.z; 

    return result;
}

Float3 float3_cross(Float3 a, Float3 b)
{
    Float3 result;
    result.x = a.y*b.z - a.z*b.y;
    result.y = a.z*b.x - a.x*b.z;
    result.z = a.x*b.y - a.y*b.x;

    return result;
}

float float3_dot(Float3 a, Float3 b)
{
    return a.x*b.x + a.y*b.y;
}

Matrix matrix_transpose(Matrix m)
{
    Matrix result = {
        m.m[0][0], m.m[1][0], m.m[2][0], m.m[3][0], 
        m.m[0][1], m.m[1][1], m.m[2][1], m.m[3][1], 
        m.m[0][2], m.m[1][2], m.m[2][2], m.m[3][2], 
        m.m[0][3], m.m[1][3], m.m[2][3], m.m[3][3], 
    };

    return result;
}

Matrix matrix_multiply(Matrix m1, Matrix m2) {
    Matrix result;

    result.m[0][0] = m1.m[0][0]*m2.m[0][0] + m1.m[0][1]*m2.m[1][0] + m1.m[0][2]*m2.m[2][0] + m1.m[0][3]*m2.m[3][0];
    result.m[0][1] = m1.m[0][0]*m2.m[0][1] + m1.m[0][1]*m2.m[1][1] + m1.m[0][2]*m2.m[2][1] + m1.m[0][3]*m2.m[3][1];
    result.m[0][2] = m1.m[0][0]*m2.m[0][2] + m1.m[0][1]*m2.m[1][2] + m1.m[0][2]*m2.m[2][2] + m1.m[0][3]*m2.m[3][2];
    result.m[0][3] = m1.m[0][0]*m2.m[0][3] + m1.m[0][1]*m2.m[1][3] + m1.m[0][2]*m2.m[2][3] + m1.m[0][3]*m2.m[3][3];
    result.m[1][0] = m1.m[1][0]*m2.m[0][0] + m1.m[1][1]*m2.m[1][0] + m1.m[1][2]*m2.m[2][0] + m1.m[1][3]*m2.m[3][0];
    result.m[1][1] = m1.m[1][0]*m2.m[0][1] + m1.m[1][1]*m2.m[1][1] + m1.m[1][2]*m2.m[2][1] + m1.m[1][3]*m2.m[3][1];
    result.m[1][2] = m1.m[1][0]*m2.m[0][2] + m1.m[1][1]*m2.m[1][2] + m1.m[1][2]*m2.m[2][2] + m1.m[1][3]*m2.m[3][2];
    result.m[1][3] = m1.m[1][0]*m2.m[0][3] + m1.m[1][1]*m2.m[1][3] + m1.m[1][2]*m2.m[2][3] + m1.m[1][3]*m2.m[3][3];
    result.m[2][0] = m1.m[2][0]*m2.m[0][0] + m1.m[2][1]*m2.m[1][0] + m1.m[2][2]*m2.m[2][0] + m1.m[2][3]*m2.m[3][0];
    result.m[2][1] = m1.m[2][0]*m2.m[0][1] + m1.m[2][1]*m2.m[1][1] + m1.m[2][2]*m2.m[2][1] + m1.m[2][3]*m2.m[3][1];
    result.m[2][2] = m1.m[2][0]*m2.m[0][2] + m1.m[2][1]*m2.m[1][2] + m1.m[2][2]*m2.m[2][2] + m1.m[2][3]*m2.m[3][2];
    result.m[2][3] = m1.m[2][0]*m2.m[0][3] + m1.m[2][1]*m2.m[1][3] + m1.m[2][2]*m2.m[2][3] + m1.m[2][3]*m2.m[3][3];
    result.m[3][0] = m1.m[3][0]*m2.m[0][0] + m1.m[3][1]*m2.m[1][0] + m1.m[3][2]*m2.m[2][0] + m1.m[3][3]*m2.m[3][0];
    result.m[3][1] = m1.m[3][0]*m2.m[0][1] + m1.m[3][1]*m2.m[1][1] + m1.m[3][2]*m2.m[2][1] + m1.m[3][3]*m2.m[3][1];
    result.m[3][2] = m1.m[3][0]*m2.m[0][2] + m1.m[3][1]*m2.m[1][2] + m1.m[3][2]*m2.m[2][2] + m1.m[3][3]*m2.m[3][2];
    result.m[3][3] = m1.m[3][0]*m2.m[0][3] + m1.m[3][1]*m2.m[1][3] + m1.m[3][2]*m2.m[2][3] + m1.m[3][3]*m2.m[3][3];

    return result;
}


// TODO(bkaylor): Actually understand this.
//     What are F, S, and U? 
Matrix look_at(Float3 eye, Float3 center, Float3 up)
{
    Matrix result;

    // Vector from the eye to the center
    Float3 F = float3_normalize(float3_subtract(eye, center));
    // Cross product of eye direction and world-space up vector, gives a "right" vector
    // Eye-space right 
    Float3 S = float3_normalize(float3_cross(F, up));
    // Up vector relative to eye 
    // Eye-space up
    Float3 U = float3_cross(S, F);

    /*
    result.m[0][0] = S.x;
    result.m[0][1] = U.x;
    result.m[0][2] = -F.x;
    result.m[0][3] = 0.0f;

    result.m[1][0] = S.y;
    result.m[1][1] = U.y;
    result.m[1][2] = -F.y;
    result.m[1][3] = 0.0f;

    result.m[2][0] = S.z;
    result.m[2][1] = U.z;
    result.m[2][2] = -F.z;
    result.m[2][3] = 0.0f;

    result.m[3][0] = -float3_dot(S, eye);
    result.m[3][1] = -float3_dot(U, eye);
    result.m[3][2] =  float3_dot(F, eye);
    result.m[3][3] =  1.0f;
    */

    // TODO(bkaylor): What's wrong with this?
    Matrix rotation = {
        S.x, U.x, -F.x, 0,
        S.y, U.y, -F.y, 0,
        S.z, U.z, -F.z, 0,
        0,   0,   0,   1,
    };

    Matrix position = {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        -eye.x, -eye.y, -eye.z, 1,
    };

    //rotation = matrix_transpose(rotation);
    //position = matrix_transpose(position);

    // TODO(bkaylor): Why this order?
    //result = matrix_multiply(rotation, position);
    result = matrix_multiply(position, rotation);

    return result;
}

static void FatalError(const char* message)
{
    MessageBoxA(NULL, message, "Error", MB_ICONEXCLAMATION);
    ExitProcess(0);
}

typedef struct Input {
    bool up;
    bool left;
    bool right;
    bool down;
    bool in;
    bool out;
    bool reset;
    bool vsync;
} Input;

Input input;

void update_input()
{
    if (GetAsyncKeyState('W'))
    {
        input.up = true;
    }
    if (GetAsyncKeyState('A'))
    {
        input.left = true;
    }
    if (GetAsyncKeyState('S'))
    {
        input.down = true;
    }
    if (GetAsyncKeyState('D'))
    {
        input.right = true;
    }
    if (GetAsyncKeyState('Q'))
    {
        input.in = true;
    }
    if (GetAsyncKeyState('E'))
    {
        input.out = true;
    }
    if (GetAsyncKeyState('R'))
    {
        input.reset = true;
    }
    if (GetAsyncKeyState('V'))
    {
        input.vsync = true;
    }
}

static LRESULT CALLBACK WindowProc(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
    case WM_KEYDOWN:
    case WM_KEYUP:
        if (wparam == VK_ESCAPE) 
        {
            SendMessage(wnd, WM_DESTROY, 0, 0);
        }
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(wnd, msg, wparam, lparam);
}

static void reset_cubes(Cube *cubes, int entity_count)
{
    for (int i = 0; i < entity_count; i += 1)
    {
        Cube *cube = &cubes[i];

        cube->rotation[0] = 0.0f;
        cube->rotation[1] = 0.0f;
        cube->rotation[2] = 0.0f;

        cube->position[2] = 1001.0f;

        cube->alive = (rand() % 20 == 0);
    }
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE previnstance, LPSTR cmdline, int cmdshow)
{
    // register window class to have custom WindowProc callback
    WNDCLASSEXW wc =
    {
        .cbSize = sizeof(wc),
        .lpfnWndProc = WindowProc,
        .hInstance = instance,
        .hIcon = LoadIcon(NULL, IDI_APPLICATION),
        .hCursor = LoadCursor(NULL, IDC_ARROW),
        .lpszClassName = L"d3d11_window_class",
    };
    ATOM atom = RegisterClassExW(&wc);
    Assert(atom && "Failed to register window class");

    // window properties - width, height and style
    int width = CW_USEDEFAULT;
    int height = CW_USEDEFAULT;
    // WS_EX_NOREDIRECTIONBITMAP flag here is needed to fix ugly bug with Windows 10
    // when window is resized and DXGI swap chain uses FLIP presentation model
    // DO NOT use it if you choose to use non-FLIP presentation model
    DWORD exstyle = WS_EX_APPWINDOW | WS_EX_NOREDIRECTIONBITMAP;
    DWORD style = WS_OVERLAPPEDWINDOW;

    // uncomment in case you want fixed size window
    //style &= ~WS_THICKFRAME & ~WS_MAXIMIZEBOX;
    //RECT rect = { 0, 0, 1280, 720 };
    //AdjustWindowRectEx(&rect, style, FALSE, exstyle);
    //width = rect.right - rect.left;
    //height = rect.bottom - rect.top;

    srand(time(NULL));

    // create window
    HWND window = CreateWindowExW(
        exstyle, wc.lpszClassName, L"D3D11 Window", style,
        CW_USEDEFAULT, CW_USEDEFAULT, width, height,
        NULL, NULL, wc.hInstance, NULL);
    Assert(window && "Failed to create window");

    HRESULT hr;

    ID3D11Device* device;
    ID3D11DeviceContext* context;

    // Get a console to print to.
    BOOL console = AllocConsole();
    Assert(console && "Failed to alloc console");
    FILE *dummy;
    freopen_s(&dummy, "CONOUT$", "w", stdout);
    printf("Hello, console!\n");

    // create D3D11 device & context
    {
        UINT flags = 0;
#ifndef NDEBUG
        // this enables VERY USEFUL debug messages in debugger output
        flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
        D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_11_0 };
        hr = D3D11CreateDevice(
            NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, flags, levels, ARRAYSIZE(levels),
            D3D11_SDK_VERSION, &device, NULL, &context);
        // make sure device creation succeeeds before continuing
        // for simple applciation you could retry device creation with
        // D3D_DRIVER_TYPE_WARP driver type which enables software rendering
        // (could be useful on broken drivers or remote desktop situations)
        AssertHR(hr);
    }

#ifndef NDEBUG
    // for debug builds enable VERY USEFUL debug break on API errors
    {
        ID3D11InfoQueue* info;
        ID3D11Device_QueryInterface(device, &IID_ID3D11InfoQueue, (void**)&info);
        ID3D11InfoQueue_SetBreakOnSeverity(info, D3D11_MESSAGE_SEVERITY_CORRUPTION, TRUE);
        ID3D11InfoQueue_SetBreakOnSeverity(info, D3D11_MESSAGE_SEVERITY_ERROR, TRUE);
        ID3D11InfoQueue_Release(info);
    }
    // after this there's no need to check for any errors on device functions manually
    // so all HRESULT return  values in this code will be ignored
    // debugger will break on errors anyway
#endif

    // create DXGI 1.2 factory for creating swap chain
    IDXGIFactory2* factory;
    hr = CreateDXGIFactory(&IID_IDXGIFactory2, &factory);
    AssertHR(hr);

    // create DXGI swap chain
    IDXGISwapChain1* swapChain;
    {
        DXGI_SWAP_CHAIN_DESC1 desc =
        {
            // default 0 value for width & height means to get it from HWND automatically
            //.Width = 0,
            //.Height = 0,

            // or use DXGI_FORMAT_R8G8B8A8_UNORM_SRGB for storing sRGB
            .Format = DXGI_FORMAT_R8G8B8A8_UNORM,

            // FLIP presentation model does not allow MSAA framebuffer
            // if you want MSAA then you'll need to render offscreen and manually
            // resolve to non-MSAA framebuffer
            .SampleDesc = { 1, 0 },

            .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
            .BufferCount = 2,

            // we don't want any automatic scaling of window content
            // this is supported only on FLIP presentation model
            .Scaling = DXGI_SCALING_NONE,

            // use more efficient FLIP presentation model
            // Windows 10 allows to use DXGI_SWAP_EFFECT_FLIP_DISCARD
            // for Windows 8 compatibility use DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL
            // for Windows 7 compatibility use DXGI_SWAP_EFFECT_DISCARD
            .SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
        };

        hr = IDXGIFactory2_CreateSwapChainForHwnd(factory, (IUnknown*)device, window, &desc, NULL, NULL, &swapChain);
        // make sure swap chain creation succeeds before continuing
        AssertHR(hr);
    }
    IDXGIFactory_Release(factory);

    // disable silly Alt+Enter changing monitor resolution to match window size
    {
        IDXGISwapChain1_GetParent(swapChain, &IID_IDXGIFactory, &factory);
        IDXGIFactory_MakeWindowAssociation(factory, window, DXGI_MWA_NO_ALT_ENTER);
        IDXGIFactory_Release(factory);
    }

    // Create MSAA render target
    int msaax = 8;
    ID3D11Texture2D* msaaRenderTarget;
    ID3D11Texture2D* resolvedTexture;
    {
        D3D11_TEXTURE2D_DESC msaaDesc =
        {
            // TODO(bkaylor): Get actual width and height
            .Width = 1904,
            .Height = 990,
            .MipLevels = 1,
            .ArraySize = 1,
            .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
            .SampleDesc = { msaax, 0 },
            .Usage = D3D11_USAGE_DEFAULT,
            .BindFlags = D3D11_BIND_RENDER_TARGET,
            .CPUAccessFlags = 0,
            .MiscFlags = 0,
        };

        hr = ID3D11Device_CreateTexture2D(device, &msaaDesc, NULL, &msaaRenderTarget);

        {
            msaaDesc.SampleDesc.Count = 1;
            hr = ID3D11Device_CreateTexture2D(device, &msaaDesc, NULL, &resolvedTexture);
        }

    }

    ID3D11RenderTargetView* msaaRenderTargetView;
    hr = ID3D11Device_CreateRenderTargetView(device, (ID3D11Resource*)msaaRenderTarget, NULL, &msaaRenderTargetView);

    typedef struct
    {
        float position[3];
        float normal[3];
        float uv[2];
        float color[3];
    } Vertex;

    ID3D11Buffer* vbuffer;
    {
        Vertex data[] =
        {
            /*
            { { -0.00f, +0.75f }, { 25.0f, 50.0f }, { 1, 0, 0 } },
            { { +0.75f, -0.50f }, {  0.0f,  0.0f }, { 0, 1, 0 } },
            { { -0.75f, -0.50f }, { 50.0f,  0.0f }, { 0, 0, 1 } },
            */

            // Back face
            /* 0 */ { { -1.00f, -1.00f, 1.00f },  { 0.0f, 0.0f, -1.0f }, {  0.0f,  0.0f }, { 1, 0, 0 } },
            /* 1 */ { { +1.00f, -1.00f, 1.00f },  { 0.0f, 0.0f, -1.0f }, { 50.0f,  0.0f }, { 1, 0, 0 } },
            /* 2 */ { { +1.00f, +1.00f, 1.00f },  { 0.0f, 0.0f, -1.0f }, { 50.0f, 50.0f }, { 1, 0, 0 } },
            /* 3 */ { { -1.00f, +1.00f, 1.00f },  { 0.0f, 0.0f, -1.0f }, {  0.0f, 50.0f }, { 1, 0, 0 } },

            // Left face
            /* 4 */ { { -1.00f, -1.00f, -1.00f }, { -1.0f, 0.0f, 0.0f }, {  0.0f,  0.0f }, { 1, 1, 1 } },
            /* 0 */ { { -1.00f, -1.00f, 1.00f },  { -1.0f, 0.0f, 0.0f }, {  0.0f,  0.0f }, { 1, 1, 1 } },
            /* 3 */ { { -1.00f, +1.00f, 1.00f },  { -1.0f, 0.0f, 0.0f }, {  0.0f, 50.0f }, { 1, 1, 1 } },
            /* 7 */ { { -1.00f, +1.00f, -1.00f }, { -1.0f, 0.0f, 0.0f }, {  0.0f, 50.0f }, { 1, 1, 1 } },
            
            // Right face
            /* 1 */ { { +1.00f, -1.00f, 1.00f },  { 1.0f, 0.0f, 0.0f }, { 50.0f,  0.0f }, { 1, 1, 1 } },
            /* 5 */ { { +1.00f, -1.00f, -1.00f }, { 1.0f, 0.0f, 0.0f }, { 50.0f,  0.0f }, { 1, 1, 1 } },
            /* 6 */ { { +1.00f, +1.00f, -1.00f }, { 1.0f, 0.0f, 0.0f }, { 50.0f, 50.0f }, { 1, 1, 1 } },
            /* 2 */ { { +1.00f, +1.00f, 1.00f },  { 1.0f, 0.0f, 0.0f }, { 50.0f, 50.0f }, { 1, 1, 1 } },

            // Top face
            /* 6 */ { { +1.00f, +1.00f, -1.00f }, { 0.0f, 1.0f, 0.0f }, { 50.0f, 50.0f }, { 1, 1, 1 } },
            /* 7 */ { { -1.00f, +1.00f, -1.00f }, { 0.0f, 1.0f, 0.0f }, {  0.0f, 50.0f }, { 1, 1, 1 } },
            /* 3 */ { { -1.00f, +1.00f, 1.00f },  { 0.0f, 1.0f, 0.0f }, {  0.0f, 50.0f }, { 1, 1, 1 } },
            /* 2 */ { { +1.00f, +1.00f, 1.00f },  { 0.0f, 1.0f, 0.0f }, { 50.0f, 50.0f }, { 1, 1, 1 } },

            // Bottom face
            /* 1 */ { { +1.00f, -1.00f, 1.00f },  { 0.0f, -1.0f, 0.0f }, { 50.0f,  0.0f }, { 1, 1, 1 } },
            /* 0 */ { { -1.00f, -1.00f, 1.00f },  { 0.0f, -1.0f, 0.0f }, {  0.0f,  0.0f }, { 1, 1, 1 } },
            /* 4 */ { { -1.00f, -1.00f, -1.00f }, { 0.0f, -1.0f, 0.0f }, {  0.0f,  0.0f }, { 1, 1, 1 } },
            /* 5 */ { { +1.00f, -1.00f, -1.00f }, { 0.0f, -1.0f, 0.0f }, { 50.0f,  0.0f }, { 1, 1, 1 } },

            // Front face
            /* 4 */ { { -1.00f, -1.00f, -1.00f }, { 0.0f, 0.0f, 1.0f }, {  0.0f,  0.0f }, { 1, 1, 1 } },
            /* 5 */ { { +1.00f, -1.00f, -1.00f }, { 0.0f, 0.0f, 1.0f }, { 50.0f,  0.0f }, { 1, 1, 1 } },
            /* 6 */ { { +1.00f, +1.00f, -1.00f }, { 0.0f, 0.0f, 1.0f }, { 50.0f, 50.0f }, { 1, 1, 1 } },
            /* 7 */ { { -1.00f, +1.00f, -1.00f }, { 0.0f, 0.0f, 1.0f }, {  0.0f, 50.0f }, { 1, 1, 1 } },
        };

        D3D11_BUFFER_DESC desc =
        {
            .ByteWidth = sizeof(data),
            .Usage = D3D11_USAGE_IMMUTABLE,
            .BindFlags = D3D11_BIND_VERTEX_BUFFER,
        };

        D3D11_SUBRESOURCE_DATA initial = { .pSysMem = data };
        ID3D11Device_CreateBuffer(device, &desc, &initial, &vbuffer);
    }

    UINT indexData[] =
    {
		// back  
		0, 1, 2,
		2, 3, 0,
		// left
        4, 5, 6,
        6, 7, 4,
		// right
        8, 9, 10,
        10, 11, 8,
		// top
        12, 13, 14,
        14, 15, 12,
		// bottom
        16, 17, 18,
        18, 19, 16,
		// front 
        22, 21, 20,
        20, 23, 22,
    };


    ID3D11Buffer* ibuffer;
    {
        D3D11_BUFFER_DESC desc = 
        {
            .ByteWidth = sizeof(indexData),
            .Usage = D3D11_USAGE_IMMUTABLE,
            .BindFlags = D3D11_BIND_INDEX_BUFFER,
        };

        D3D11_SUBRESOURCE_DATA initial = { .pSysMem = indexData };
        ID3D11Device_CreateBuffer(device, &desc, &initial, &ibuffer);
    }

    // vertex & pixel shaders for drawing triangle, plus input layout for vertex input
    ID3D11InputLayout* layout;
    ID3D11VertexShader* vshader;
    ID3D11PixelShader* pshader;
    {
        // these must match vertex shader input layout (VS_INPUT in vertex shader source below)
        D3D11_INPUT_ELEMENT_DESC desc[] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Vertex, position), D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Vertex, normal),   D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, offsetof(Vertex, uv),       D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "COLOR",    0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Vertex, color),    D3D11_INPUT_PER_VERTEX_DATA, 0 },

            { "TRANSFORM", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0,  D3D11_INPUT_PER_INSTANCE_DATA, 1 },
            { "TRANSFORM", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 16, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
            { "TRANSFORM", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 32, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
            { "TRANSFORM", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 48, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
            { "ICOLOR",    0, DXGI_FORMAT_R32G32B32_FLOAT,    1, 64, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
        };

#if 0
        // alternative to hlsl compilation at runtime is to precompile shaders offline
        // it improves startup time - no need to parse hlsl files at runtime!
        // and it allows to remove runtime dependency on d3dcompiler dll file

        // a) save shader source code into "shader.hlsl" file
        // b) run hlsl compiler to compile shader, these run compilation with optimizations and without debug info:
        //      fxc.exe /nologo /T vs_5_0 /E vs /O3 /WX /Zpc /Ges /Fh d3d11_vshader.h /Vn d3d11_vshader /Qstrip_reflect /Qstrip_debug /Qstrip_priv shader.hlsl
        //      fxc.exe /nologo /T ps_5_0 /E ps /O3 /WX /Zpc /Ges /Fh d3d11_pshader.h /Vn d3d11_pshader /Qstrip_reflect /Qstrip_debug /Qstrip_priv shader.hlsl
        //    they will save output to d3d11_vshader.h and d3d11_pshader.h files
        // c) change #if 0 above to #if 1

        // you can also use "/Fo d3d11_*shader.bin" argument to save compiled shader as binary file to store with your assets
        // then provide binary data for Create*Shader functions below without need to include shader bytes in C

        #include "d3d11_vshader.h"
        #include "d3d11_pshader.h"

        ID3D11Device_CreateVertexShader(device, d3d11_vshader, sizeof(d3d11_vshader), NULL, &vshader);
        ID3D11Device_CreatePixelShader(device, d3d11_pshader, sizeof(d3d11_pshader), NULL, &pshader);
        ID3D11Device_CreateInputLayout(device, desc, ARRAYSIZE(desc), d3d11_vshader, sizeof(d3d11_vshader), &layout);
#else
        // SHADER_SOURCE
        const char hlsl[] =
            "#line " STR(__LINE__) "                                  \n\n" // actual line number in this file for nicer error messages
            "                                                           \n"
            "struct VS_INPUT                                            \n"
            "{                                                          \n"
            "     float3   pos       : POSITION;                        \n" // these names must match D3D11_INPUT_ELEMENT_DESC array
            "     float3   normal    : NORMAL;                          \n" 
            "     float2   uv        : TEXCOORD;                        \n"
            "     float3   color     : COLOR;                           \n"
            "     float4x4 transform : TRANSFORM;                       \n"
            "     float3   icolor    : ICOLOR;                          \n"
            "};                                                         \n"
            "                                                           \n"
            "struct PS_INPUT                                            \n"
            "{                                                          \n"
            "  float4 pos   : SV_POSITION;                              \n" // these names do not matter, except SV_... ones
            "  float2 uv    : TEXCOORD;                                 \n"
            "  float4 color : COLOR;                                    \n"
            "};                                                         \n"
            "                                                           \n"
            "cbuffer cbuffer0 : register(b0)                            \n" // b0 = constant buffer bound to slot 0
            "{                                                          \n"
            "    float4x4 uProjection;                                  \n"
            "    float4x4 uView;                                        \n"
            "    float3   uColor;                                       \n"
            "    float3   uLight;                                       \n"
            "}                                                          \n"
            "                                                           \n"
            "sampler sampler0 : register(s0);                           \n" // s0 = sampler bound to slot 0
            "                                                           \n"
            "Texture2D<float4> texture0 : register(t0);                 \n" // t0 = shader resource bound to slot 0
            "                                                           \n"
            "PS_INPUT vs(VS_INPUT input)                                \n"
            "{                                                          \n"
            "    float light = clamp(dot(normalize(mul(float4(input.normal, 0.0f), input.transform).xyz), normalize(-uLight)), 0.0f, 1.0f) * 0.8f + 0.2f;                                                          \n"
            "                                                           \n"
            "    PS_INPUT output;                                       \n"
            "    output.pos = mul(mul(mul(uProjection, uView),input.transform), float4(input.pos, 1));    \n"
            "    output.uv = input.uv;                                  \n"
            "    output.color = float4(uColor * light, 1.0f) * float4(input.color, 1.0f) * float4(input.icolor, 1.0f); \n"
            "    // output.color = float4(uColor * light, 1.0f) * float4(input.color, 1.0f); \n"
            "    return output;                                         \n"
            "}                                                          \n"
            "                                                           \n"
            "float4 ps(PS_INPUT input) : SV_TARGET                      \n"
            "{                                                          \n"
            "    // float4 tex = texture0.Sample(sampler0, input.uv);   \n"
            "    // return float4(0.2, 0, 0, 1);                        \n"
            "    return input.color;                                    \n"
            "}                                                          \n";
        ;

        UINT flags = D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR | D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_WARNINGS_ARE_ERRORS;
#ifndef NDEBUG
        flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
        flags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif

        ID3DBlob* error;

        ID3DBlob* vblob;
        hr = D3DCompile(hlsl, sizeof(hlsl), NULL, NULL, NULL, "vs", "vs_5_0", flags, 0, &vblob, &error);
        if (FAILED(hr))
        {
            const char* message = ID3D10Blob_GetBufferPointer(error);
            OutputDebugStringA(message);
            Assert(!"Failed to compile vertex shader!");
        }

        ID3DBlob* pblob;
        hr = D3DCompile(hlsl, sizeof(hlsl), NULL, NULL, NULL, "ps", "ps_5_0", flags, 0, &pblob, &error);
        if (FAILED(hr))
        {
            const char* message = ID3D10Blob_GetBufferPointer(error);
            OutputDebugStringA(message);
            Assert(!"Failed to compile pixel shader!");
        }

        ID3D11Device_CreateVertexShader(device, ID3D10Blob_GetBufferPointer(vblob), ID3D10Blob_GetBufferSize(vblob), NULL, &vshader);
        ID3D11Device_CreatePixelShader(device, ID3D10Blob_GetBufferPointer(pblob), ID3D10Blob_GetBufferSize(pblob), NULL, &pshader);
        ID3D11Device_CreateInputLayout(device, desc, ARRAYSIZE(desc), ID3D10Blob_GetBufferPointer(vblob), ID3D10Blob_GetBufferSize(vblob), &layout);

        ID3D10Blob_Release(pblob);
        ID3D10Blob_Release(vblob);
#endif
    }

    ID3D11Buffer* ubuffer;
    {
        D3D11_BUFFER_DESC desc =
        {
            // space for some 4x4 float matrix (cbuffer0 from pixel shader)
            .ByteWidth = sizeof(Constants) + (32 - (sizeof(Constants) % 32)),
            .Usage = D3D11_USAGE_DYNAMIC,
            .BindFlags = D3D11_BIND_CONSTANT_BUFFER,
            .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
        };
        ID3D11Device_CreateBuffer(device, &desc, NULL, &ubuffer);
    }

    ID3D11ShaderResourceView* textureView;
    {
        // checkerboard texture, with 50% transparency on black colors
        /*
        unsigned int pixels[] =
        {
            0x80000000, 0xffffffff,
            0xffffffff, 0x80000000,
        }
        */
        unsigned int pixels[] =
        {
            0xffffffff, 0xffffffff,
            0xffffffff, 0xffffffff,
        };
        UINT width = 2;
        UINT height = 2;

        D3D11_TEXTURE2D_DESC desc =
        {
            .Width = width,
            .Height = height,
            .MipLevels = 1,
            .ArraySize = 1,
            .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
            .SampleDesc = { 1, 0 },
            .Usage = D3D11_USAGE_IMMUTABLE,
            .BindFlags = D3D11_BIND_SHADER_RESOURCE,
        };

        D3D11_SUBRESOURCE_DATA data =
        {
            .pSysMem = pixels,
            .SysMemPitch = width * sizeof(unsigned int),
        };

        ID3D11Texture2D* texture;
        ID3D11Device_CreateTexture2D(device, &desc, &data, &texture);
        ID3D11Device_CreateShaderResourceView(device, (ID3D11Resource*)texture, NULL, &textureView);
        ID3D11Texture2D_Release(texture);
    }

    ID3D11SamplerState* sampler;
    {
        D3D11_SAMPLER_DESC desc =
        {
            .Filter = D3D11_FILTER_MIN_MAG_MIP_POINT,
            .AddressU = D3D11_TEXTURE_ADDRESS_WRAP,
            .AddressV = D3D11_TEXTURE_ADDRESS_WRAP,
            .AddressW = D3D11_TEXTURE_ADDRESS_WRAP,
        };

        ID3D11Device_CreateSamplerState(device, &desc, &sampler);
    }

    ID3D11BlendState* blendState;
    {
        // enable alpha blending
        D3D11_BLEND_DESC desc =
        {
            .RenderTarget[0] =
            {
                .BlendEnable = FALSE,
                .SrcBlend = D3D11_BLEND_SRC_ALPHA,
                .DestBlend = D3D11_BLEND_INV_SRC_ALPHA,
                .BlendOp = D3D11_BLEND_OP_ADD,
                .SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA,
                .DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA,
                .BlendOpAlpha = D3D11_BLEND_OP_ADD,
                .RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL,
            },
        };
        ID3D11Device_CreateBlendState(device, &desc, &blendState);
    }

    ID3D11RasterizerState* rasterizerState;
    {
        // disable culling
        D3D11_RASTERIZER_DESC desc =
        {
            .FillMode = D3D11_FILL_SOLID,
            .CullMode = D3D11_CULL_BACK,
        };
        ID3D11Device_CreateRasterizerState(device, &desc, &rasterizerState);
    }

    ID3D11DepthStencilState* depthState;
    {
        // disable depth & stencil test
        D3D11_DEPTH_STENCIL_DESC desc =
        {
            .DepthEnable = TRUE,
            .DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL,
            .DepthFunc = D3D11_COMPARISON_LESS,
            .StencilEnable = FALSE,
            .StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK,
            .StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK,
            // .FrontFace = ... 
            // .BackFace = ...
        };
        ID3D11Device_CreateDepthStencilState(device, &desc, &depthState);
    }

    ID3D11RenderTargetView* rtView = NULL;
    ID3D11DepthStencilView* dsView = NULL;

    // show the window
    ShowWindow(window, SW_SHOWDEFAULT);

    LARGE_INTEGER freq, c1;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&c1);

    float angle = 0;
    DWORD currentWidth = 0;
    DWORD currentHeight = 0;

    RECT rect;
    GetClientRect(window, &rect);
    width = rect.right - rect.left;
    height = rect.bottom - rect.top;

    float w = (float) width;
    float h = (float) height;

    //
    // Initialize camera
    //

    Camera camera;
    {
        camera.position[0] = 0.0f;
        camera.position[1] = 0.0f;
        camera.position[2] = 0.0f;

        camera.rotation[0] = 0.0f;
        camera.rotation[1] = 0.0f;
        camera.rotation[2] = 0.0f;
    }

    //
    // Define our grid of cubes here.
    // 
    int rows = 50;
    int columns = 100;
    int entity_count = rows*columns;

    Cube *cubes = malloc(sizeof(Cube) * entity_count);
    for (int i = 0; i < rows; i += 1)
    {
        for (int j = 0; j < columns; j += 1)
        {
            Cube *cube = &cubes[i*columns + j];

            cube->color[0] = 0.0f;
            cube->color[1] = 0.0f;
            cube->color[2] = 0.0f;

            cube->rotation[0] = 0.0f;
            cube->rotation[1] = 0.0f;
            cube->rotation[2] = 0.0f;

            cube->length = width/(columns*3);

            cube->scale[0] = cube->length;
            cube->scale[1] = cube->length;
            cube->scale[2] = cube->length;

            // ??? random numbers .......
            // TODO(bkaylor)
            // trying to get this based on currentWidth or width / height, but they seem to be 0 ...... how do i get the real window size?  
            float x_offset = w/2.6;
            float y_offset = h/2.6;
            float x_space = w/columns - 5;
            float y_space = h/rows - 5;
            cube->position[0] = j*x_space - x_offset;
            cube->position[1] = i*y_space - y_offset;
            cube->position[2] = 1001.0f;

            cube->alive = (rand() % 20 == 0);
        }
    }

    // Create instance buffer for entities
    Instance *instanceData = malloc(sizeof(Instance) * entity_count);
    ID3D11Buffer* instanceBuffer;
    {
        D3D11_BUFFER_DESC desc = 
        {
            .ByteWidth = (sizeof(Instance) * entity_count),
            .Usage = D3D11_USAGE_DYNAMIC,
            .BindFlags = D3D11_BIND_VERTEX_BUFFER,
            .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
        };
        D3D11_SUBRESOURCE_DATA initial = {.pSysMem = instanceData};
        ID3D11Device_CreateBuffer(device, &desc, &initial, &instanceBuffer);
    }

    input.up    = false;
    input.left  = false;
    input.right = false;
    input.down  = false;
    input.in    = false;
    input.out   = false;
    input.reset = false;
    input.vsync = false;

    int frame = 0;
    BOOL vsync = TRUE;

    for (;;)
    {
        //
        // Input
        //
        // process all incoming Windows messages

        MSG msg;
        if (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
            {
                break;
            }
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
            continue;
        }

        update_input();

        // get current size for window client area
        RECT rect;
        GetClientRect(window, &rect);
        width = rect.right - rect.left;
        height = rect.bottom - rect.top;

        float w = (float) width;
        float h = (float) height;
        //
        // Update
        //

        float speed = 10.0f;
        if (input.up)
        {
            camera.position[1] += speed;
        }
        if (input.left)
        {
            camera.position[0] -= speed;
        }
        if (input.right)
        {
            camera.position[0] += speed;
        }
        if (input.down)
        {
            camera.position[1] -= speed;
        }
        if (input.in)
        {
            camera.position[2] += speed;
        }
        if (input.out)
        {
            camera.position[2] -= speed;
        }
        if (input.reset)
        {
            reset_cubes(cubes, entity_count);
        }
        if (input.vsync)
        {
            vsync = !vsync;
        }

        input.up    = false;
        input.left  = false;
        input.right = false;
        input.down  = false;
        input.in    = false;
        input.out   = false;
        input.reset = false;
        input.vsync = false;

        //
        // Render
        //

        // resize swap chain if needed
        if (rtView == NULL || width != currentWidth || height != currentHeight)
        {
            if (rtView)
            {
                // release old swap chain buffers
                ID3D11DeviceContext_ClearState(context);
                ID3D11RenderTargetView_Release(rtView);
                ID3D11DepthStencilView_Release(dsView);
                rtView = NULL;
            }

            // resize to new size for non-zero size
            if (width != 0 && height != 0)
            {
                hr = IDXGISwapChain1_ResizeBuffers(swapChain, 0, width, height, DXGI_FORMAT_UNKNOWN, 0);
                if (FAILED(hr))
                {
                    FatalError("Failed to resize swap chain!");
                }

                // create RenderTarget view for new backbuffer texture
                ID3D11Texture2D* backbuffer;
                IDXGISwapChain1_GetBuffer(swapChain, 0, &IID_ID3D11Texture2D, (void**)&backbuffer);
                ID3D11Device_CreateRenderTargetView(device, (ID3D11Resource*)backbuffer, NULL, &rtView);
                ID3D11Texture2D_Release(backbuffer);

                D3D11_TEXTURE2D_DESC depthDesc =
                {
                    .Width = width,
                    .Height = height,
                    .MipLevels = 1,
                    .ArraySize = 1,
                    .Format = DXGI_FORMAT_D32_FLOAT, // or use DXGI_FORMAT_D32_FLOAT_S8X24_UINT if you need stencil
                    .SampleDesc = { msaax, 0 },
                    .Usage = D3D11_USAGE_DEFAULT,
                    .BindFlags = D3D11_BIND_DEPTH_STENCIL,
                };

                // create new depth stencil texture & DepthStencil view
                ID3D11Texture2D* depth;
                ID3D11Device_CreateTexture2D(device, &depthDesc, NULL, &depth);
                ID3D11Device_CreateDepthStencilView(device, (ID3D11Resource*)depth, NULL, &dsView);
                ID3D11Texture2D_Release(depth);
            }

            currentWidth = width;
            currentHeight = height;
        }

        // can render only if window size is non-zero - we must have backbuffer & RenderTarget view created
        if (rtView)
        {
            LARGE_INTEGER c2;
            QueryPerformanceCounter(&c2);
            float delta = (float)((double)(c2.QuadPart - c1.QuadPart) / freq.QuadPart);
            c1 = c2;

            // output viewport covering all client area of window
            D3D11_VIEWPORT viewport =
            {
                .TopLeftX = 0,
                .TopLeftY = 0,
                .Width = (FLOAT)width,
                .Height = (FLOAT)height,
                .MinDepth = 0,
                .MaxDepth = 1,
            };

            // clear screen
            FLOAT color[] = { 0.392f, 0.584f, 0.929f, 1.f };
            ID3D11DeviceContext_ClearRenderTargetView(context, msaaRenderTargetView, color);
            ID3D11DeviceContext_ClearDepthStencilView(context, dsView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.f, 0);

            // Update constant buffer (once per frame)
            float aspect = (float)height / width;

            Cube *center_cube = &cubes[(entity_count/2) + (columns/2)];

            Float3 eye = {
                .x = camera.position[0], 
                .y = camera.position[1], 
                .z = camera.position[2],
            };

            Float3 center = {
                .x = center_cube->position[0],
                .y = center_cube->position[1],
                .z = center_cube->position[2],
            };

            Float3 up = {
                .x = 0.0f, 
                .y = 1.0f, 
                .z = 0.0f, 
            };

            Matrix view = look_at(eye, center, up);

            float pnear = 0.1;
            float pfar  = 5000.0;

            float fov = 103;
            float scale = 1 / (tan(fov * 0.5 * PI/180));

            Matrix projection =
            {
                2*pnear/w,    0,             0,                       0,
                0,            2*pnear/h,     0,                       0,
                0,            0,             pfar/(pfar-pnear),       1,
                0,            0,             pnear*pfar/(pnear-pfar), 0,
            };

            projection.m[0][0] = scale*aspect;
            projection.m[1][1] = scale;

            Constants constants;
            constants.view = view;
            constants.projection = projection;

            constants.light[0] =  1.0f; 
            constants.light[1] =  1.0f;
            constants.light[2] = +1.0f; 

            constants.color[0] = 1.0f;
            constants.color[1] = 1.0f;
            constants.color[2] = 1.0f;

            D3D11_MAPPED_SUBRESOURCE mapped;
            ID3D11DeviceContext_Map(context, (ID3D11Resource*)ubuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
            memcpy(mapped.pData, &constants, sizeof(constants));
            ID3D11DeviceContext_Unmap(context, (ID3D11Resource*)ubuffer, 0);

            //
            // Loop over entities here
            //
            D3D11_MAPPED_SUBRESOURCE mappedInstanceBuffer;
            ID3D11DeviceContext_Map(context, (ID3D11Resource*)instanceBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedInstanceBuffer);

            for (int i = 0; i < entity_count; i += 1)
            {
                Cube *cube = &cubes[i];

                // Game of life simulation
                if (frame % 10 == 0)
                {
                    int r = i / columns;
                    int c = i % columns; 

                    int living_neighbors = 0;

                    living_neighbors += check(cubes, rows, columns, r-1, c-1); 
                    living_neighbors += check(cubes, rows, columns, r-1, c); 
                    living_neighbors += check(cubes, rows, columns, r-1, c+1); 

                    living_neighbors += check(cubes, rows, columns, r, c-1); 
                    living_neighbors += check(cubes, rows, columns, r, c+1); 

                    living_neighbors += check(cubes, rows, columns, r+1, c-1); 
                    living_neighbors += check(cubes, rows, columns, r+1, c); 
                    living_neighbors += check(cubes, rows, columns, r+1, c+1); 

                    if (cube->alive)
                    {
                        cube->alive = (living_neighbors == 2 || living_neighbors == 3);
                    }
                    else
                    {
                        cube->alive = (living_neighbors == 3);
                    }
                }

                if (cube->alive)
                {
                    cube->color[0] = 0.9f;
                    cube->color[1] = 0.9f;
                    cube->color[2] = 0.9f;

                    cube->rotation[2] += 0.01f;

                    cube->position[2] -= 0.05f;
                }
                else
                {
                    cube->color[0] = 0.1f;
                    cube->color[1] = 0.1f;
                    cube->color[2] = 0.1f;

                    // cube->position[2] += 0.05f;
                }

                //
                // Rotate/Scale/Translate
                // 

                // setup 4x4c rotation matrix in uniform
                {
                    // angle += delta * 2.0f * (float)M_PI / 20.0f; // full rotation in 20 seconds
                    // angle = fmodf(angle, 2.0f * (float)M_PI);

                    Matrix transform; 
                    {
                        // Rotate
                        Matrix rotate_x = 
                        {
                            1, 0,                       0,                       0,
                            0, cos(cube->rotation[0]),  -sin(cube->rotation[0]), 0,
                            0, sin(cube->rotation[0]),  cos(cube->rotation[0]),  0,
                            0, 0,                       0,                       1,
                        };

                        Matrix rotate_y =
                        {
                            cos(cube->rotation[1]),  0, sin(cube->rotation[1]), 0,
                            0,                       1, 0,                      0,
                            -sin(cube->rotation[1]), 0, cos(cube->rotation[1]), 0,
                            0,                       0, 0,                      1,
                        };

                        Matrix rotate_z =
                        {
                            cos(cube->rotation[2]), -sin(cube->rotation[2]), 0, 0,
                            sin(cube->rotation[2]), cos(cube->rotation[2]),  0, 0,
                            0,                      0,                       1, 0, 
                            0,                      0,                       0, 1,
                        };

                        // Scale
                        Matrix scale =
                        {
                            cube->scale[0], 0,              0,              0,
                            0,              cube->scale[1], 0,              0,
                            0,              0,              cube->scale[2], 0, 
                            0,              0,              0,              1,
                        };

                        // Translate
                        Matrix translate =
                        {
                            1,                 0,                 0,                 0,
                            0,                 1,                 0,                 0,
                            0,                 0,                 1,                 0,
                            cube->position[0], cube->position[1], cube->position[2], 1,
                        };

                        // Rotate_X * Rotate_Y * Rotate_Z * Scale * Translate
                        transform = matrix_multiply(rotate_x, rotate_y);
                        transform = matrix_multiply(transform, rotate_z);
                        transform = matrix_multiply(transform, scale);
                        transform = matrix_multiply(transform, translate);
                    }

                    instanceData[i].transform = transform;
                    memcpy(instanceData[i].color, cube->color, (sizeof(float) * 3));

                }
            }

            memcpy(mappedInstanceBuffer.pData, instanceData, (sizeof(Instance) * entity_count));
            ID3D11DeviceContext_Unmap(context, (ID3D11Resource*)instanceBuffer, 0);

            // Input Assembler
            ID3D11DeviceContext_IASetInputLayout(context, layout);
            ID3D11DeviceContext_IASetPrimitiveTopology(context, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            UINT strides[2] = {sizeof(Vertex), sizeof(Instance)};
            UINT offsets[2] = {0, 0};
            ID3D11Buffer *buffers[2] = {vbuffer, instanceBuffer};
            ID3D11DeviceContext_IASetVertexBuffers(context, 0, 2, buffers, strides, offsets);
            ID3D11DeviceContext_IASetIndexBuffer(context, ibuffer, DXGI_FORMAT_R32_UINT, 0);

            // Vertex Shader
            ID3D11DeviceContext_VSSetConstantBuffers(context, 0, 1, &ubuffer);
            ID3D11DeviceContext_VSSetShader(context, vshader, NULL, 0);

            // Rasterizer StagFALSEe
            ID3D11DeviceContext_RSSetViewports(context, 1, &viewport);
            ID3D11DeviceContext_RSSetState(context, rasterizerState);

            // Pixel Shader
            ID3D11DeviceContext_PSSetSamplers(context, 0, 1, &sampler);
            ID3D11DeviceContext_PSSetShaderResources(context, 0, 1, &textureView);
            ID3D11DeviceContext_PSSetShader(context, pshader, NULL, 0);

            // Output Merger
            ID3D11DeviceContext_OMSetBlendState(context, blendState, NULL, ~0U);
            ID3D11DeviceContext_OMSetDepthStencilState(context, depthState, 0);
            // ID3D11DeviceContext_OMSetRenderTargets(context, 1, &rtView, dsView);

            // Render to msaa texture
            ID3D11DeviceContext_OMSetRenderTargets(context, 1, &msaaRenderTargetView, dsView);

            // draw all vertices in index buffer
            ID3D11DeviceContext_DrawIndexedInstanced(context, ARRAYSIZE(indexData), entity_count, 0, 0, 0);
        }

        ID3D11DeviceContext_ResolveSubresource(context, (ID3D11Resource*)resolvedTexture, 0, (ID3D11Resource*)msaaRenderTarget, 0, DXGI_FORMAT_R8G8B8A8_UNORM);

        ID3D11Texture2D* backBuffer;
        IDXGISwapChain1_GetBuffer(swapChain, 0, &IID_ID3D11Texture2D, (void**)&backBuffer);

        ID3D11DeviceContext_CopyResource(context, (ID3D11Resource*)backBuffer, (ID3D11Resource*)resolvedTexture);

        // change to FALSE to disable vsync
        hr = IDXGISwapChain1_Present(swapChain, vsync ? 1 : 0, 0);
        if (hr == DXGI_STATUS_OCCLUDED)
        {
            // window is minimized, cannot vsync - instead sleep a bit
            if (vsync)
            {
                Sleep(10);
            }
        }
        else if (FAILED(hr))
        {
            FatalError("Failed to present swap chain! Device lost?");
        }

        frame += 1;
    }
}

