#include <windows.h>
#include <d2d1.h>
#pragma comment(lib, "d2d1")

#include "basewin.h"


/*
 - Direct2D is an immediate-mode API
 - Immediate-mode API is procedural;
    - Each time a new frame is drawn, the application directly issues the drawing commands. 
      The graphics library does not store a scene model between frames. 
      Instead, the application keeps track of the scene
*/

/*
 - D2D1 namespace contains helper functions and classes, are not 
   strictly part of the Direct2D API — you can program Direct2D without using them — but 
   they help simplify your code
 - D2D1 namespace contains:
    - A ColorF class for constructing color values
    - A Matrix3x2F for constructing transformation matrices
    - A set of functions to initialize Direct2D structures
*/

/*
 - render target is the location where the program will draw, it is typically the window (specifically, the client area of the window)
    - render target is represented by the 'ID2D1RenderTarget' interface
 - resource is an object the program uses for drawing, i.e. brush, stroke style, geometry, mesh 
    - every resource type is represented by an interface that derives from 'ID2D1Resource'
*/


template <class T> void SafeRelease(T** ppT)
{
    if (*ppT)
    {
        (*ppT)->Release();
        *ppT = NULL;
    }
}

class MainWindow : public BaseWindow<MainWindow>
{
    // 'pFactory' is a factory object to create other objects; render targets and device-independent resources, such as stroke styles and geometries
    ID2D1Factory* pFactory; 

    // Device - dependent resources, such as brushesand bitmaps, are created by the render target object
    ID2D1HwndRenderTarget* pRenderTarget; // render target pointer
    ID2D1SolidColorBrush* pBrush; // brush pointer
    D2D1_ELLIPSE ellipse;

    void    CalculateLayout();
    HRESULT CreateGraphicsResources();
    void    DiscardGraphicsResources();
    void    OnPaint();
    void    Resize();

public:

    MainWindow() : pFactory(NULL), pRenderTarget(NULL), pBrush(NULL){}

    PCWSTR  ClassName() const { return L"Circle Window Class"; }
    LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
};


// Recalculate drawing layout when the size of the window changes
void MainWindow::CalculateLayout()
{
    if (pRenderTarget != NULL)
    {
        D2D1_SIZE_F size = pRenderTarget->GetSize(); // returns the size of the render target in DIPs (not pixels)
        const float x = size.width / 2;
        const float y = size.height / 2;
        const float radius = min(x, y);
        ellipse = D2D1::Ellipse(D2D1::Point2F(x, y), radius, radius);
    }
}

// create the two resourses, i.e. render target and brush
HRESULT MainWindow::CreateGraphicsResources()
{
    HRESULT hr = S_OK;
    if (pRenderTarget == NULL)
    {
        RECT rc;
        GetClientRect(m_hwnd, &rc);

        D2D1_SIZE_U size = D2D1::SizeU(rc.right, rc.bottom);

        /*
         - 'CreateHwndRenderTarget' creates the render target
            - first param, specifies options that are common to any type of render target, pass in default options by calling the helper function 'D2D1::RenderTargetProperties'
            - second param, specifies the handle to the window plus the size of the render target, in pixels
            - third param, receives an 'ID2D1HwndRenderTarget' pointer
        */
        
        hr = pFactory->CreateHwndRenderTarget(
            D2D1::RenderTargetProperties(),
            D2D1::HwndRenderTargetProperties(m_hwnd, size),
            &pRenderTarget);

        if (SUCCEEDED(hr))
        {
            // create solid-color brush
            const D2D1_COLOR_F color = D2D1::ColorF(1.0f, 0, 0);
            hr = pRenderTarget->CreateSolidColorBrush(color, &pBrush);

            if (SUCCEEDED(hr))
            {
                CalculateLayout();
            }
        }
    }
    return hr;
}

void MainWindow::DiscardGraphicsResources()
{
    SafeRelease(&pRenderTarget);
    SafeRelease(&pBrush);
}

void MainWindow::OnPaint()
{
    HRESULT hr = CreateGraphicsResources();
    if (SUCCEEDED(hr))
    {
        // 'ID2D1RenderTarget' interface is used for all drawing operations

        PAINTSTRUCT ps;
        BeginPaint(m_hwnd, &ps);

        pRenderTarget->BeginDraw(); // signals the start of drawing 

        pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::BlanchedAlmond)); // fill the render target with a solid color 
        pRenderTarget->FillEllipse(ellipse, pBrush); // draws a filled ellipse, using the specified brush for the fill

        hr = pRenderTarget->EndDraw(); //  signals the completion of drawing for this frame

        /*
         - BeginDraw, Clear, and FillEllipse methods all have a void return type
         - if an error occurs during the execution of any of these methods, the error is signaled through the return 
           value of the EndDraw method
        */


        // Direct2D signals a lost device by returning the error code 'D2DERR_RECREATE_TARGET' from the EndDraw method
        if (FAILED(hr) || hr == D2DERR_RECREATE_TARGET)
        {
            DiscardGraphicsResources();
        }
        EndPaint(m_hwnd, &ps);
    }
}

void MainWindow::Resize()
{
    if (pRenderTarget != NULL)
    {
        RECT rc;
        GetClientRect(m_hwnd, &rc); // get the new size of the client area 

        D2D1_SIZE_U size = D2D1::SizeU(rc.right, rc.bottom);

        pRenderTarget->Resize(size); // updates the size of the render target, also specified in pixels
        CalculateLayout();
        InvalidateRect(m_hwnd, NULL, FALSE); // forces a repaint by adding the entire client area to the window's update region
    }
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow)
{
    MainWindow win;

    if (!win.Create(L"Draw Circle", WS_OVERLAPPEDWINDOW))
    {
        return 0;
    }

    ShowWindow(win.Window(), nCmdShow);

    // Run the message loop.

    MSG msg = { };
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}


// 'MainWindow::HandleMessage' is used to implement the window procedure
LRESULT MainWindow::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_CREATE:
        if (FAILED(D2D1CreateFactory(
            D2D1_FACTORY_TYPE_SINGLE_THREADED, &pFactory))) // create Direct2D factory object 
            /*
             - first param is the flag that specifies creation objects  
                - 'D2D1_FACTORY_TYPE_SINGLE_THREADED' flag means that you will not call Direct2D from multiple threads
                - to support calls from multiple threads, specify 'D2D1_FACTORY_TYPE_MULTI_THREADED'
             - second param, receives a pointer to the 'ID2D1Factory' interface 
            */
        {
            return -1;  // Fail CreateWindowEx.
        }
        return 0;

    case WM_DESTROY:
        DiscardGraphicsResources();
        SafeRelease(&pFactory);
        PostQuitMessage(0);
        return 0;

    case WM_PAINT:
        OnPaint();
        return 0;

    case WM_SIZE:
        Resize();
        return 0;
    }
    return DefWindowProc(m_hwnd, uMsg, wParam, lParam);
}