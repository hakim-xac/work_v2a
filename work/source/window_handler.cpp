#include "../include/window_handler.h"
#include "../include/resource.h"
#include "../include/HdcScoped.h"
#include <algorithm>
#include <iostream>
#include <sstream>
#include <memory>

namespace LAB
{
    //--------------------------
	
    WindowHandler::WindowHandler(HWND hwnd) :
        m_hwnd{ hwnd },
        m_hdc { GetDC(m_hwnd) }
    {
        sg_boiler_provider = CreateNamedPipeW(
            L"\\\\.\\pipe\\BoilerProvider",
            PIPE_ACCESS_OUTBOUND,
            PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE,
            1, 4, 4, 0, 0
        );

        sg_provider_transporter = CreateNamedPipeW(
            L"\\\\.\\pipe\\ProviderTransporter",
            PIPE_ACCESS_DUPLEX,
            PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE,
            1, 4, sizeof sg_pipe_message_atomic.load(), 0, 0
        );

        ConnectNamedPipe(sg_boiler_provider, 0);
        ConnectNamedPipe(sg_provider_transporter, 0);

        CreateThread(0, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(vehicleThread), 0, 0, 0);
        CreateThread(0, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(powerUnitsThread), reinterpret_cast<LPVOID>(0), 0, 0);
        CreateThread(0, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(powerUnitsThread), reinterpret_cast<LPVOID>(1), 0, 0);
        CreateThread(0, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(powerUnitsThread), reinterpret_cast<LPVOID>(2), 0, 0);
        CreateThread(0, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(powerUnitsThread), reinterpret_cast<LPVOID>(3), 0, 0);
    }

	//--------------------------
	
    WindowHandler::~WindowHandler()
    {
        ReleaseDC(m_hwnd, m_hdc);
    }
    
    //--------------------------
	
    LRESULT CALLBACK
        WindowHandler::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        switch (message)
        {
        case WM_CREATE:
            SetTimer(hWnd, 1, 20, nullptr);
            break;
        case WM_TIMER:
            InvalidateRect(hWnd, nullptr, false);
            break;
        case WM_COMMAND:
        {
            int wmId{ LOWORD(wParam) };
            switch (wmId)
            {
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
        case WM_PAINT:
        {
            LAB::HdcScoped hdc_scoped{ hWnd };
            HDC hdc{ hdc_scoped.getHDC() };
            g_drawer.draw(hdc);
        }
        break;
        case WM_DESTROY:
            m_is_running = false;
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProcW(hWnd, message, wParam, lParam);
        }
        return 0;
    }

    //--------------------------

    void WindowHandler::vehicleThread()
    {
        std::array<HANDLE, 4> transporter_boilers
        {
            CreateNamedPipeW(L"\\\\.\\pipe\\TransporterBoilers", PIPE_ACCESS_DUPLEX, PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE, 4, 4, 4, 0, 0),
            CreateNamedPipeW(L"\\\\.\\pipe\\TransporterBoilers", PIPE_ACCESS_DUPLEX, PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE, 4, 4, 4, 0, 0),
            CreateNamedPipeW(L"\\\\.\\pipe\\TransporterBoilers", PIPE_ACCESS_DUPLEX, PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE, 4, 4, 4, 0, 0),
            CreateNamedPipeW(L"\\\\.\\pipe\\TransporterBoilers", PIPE_ACCESS_DUPLEX, PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE, 4, 4, 4, 0, 0)
        };

        ConnectNamedPipe(transporter_boilers[0], 0);
        ConnectNamedPipe(transporter_boilers[1], 0);
        ConnectNamedPipe(transporter_boilers[2], 0);
        ConnectNamedPipe(transporter_boilers[3], 0);
        
        g_drawer.drawRectangle(480, 40, 499, 60, Color{ 200, 200, 200 });

        unsigned long s{};
        static_cast<void>(ReadFile(sg_provider_transporter, &sg_pipe_message_atomic, sizeof sg_pipe_message_atomic, &s, 0));

        for (int32_t i{ 480 }; i >= 60; i -= 10)
        {
            g_drawer.drawRectangle(i, 40, i + 19, 60, Color{ 200, 200, 200 });

            std::this_thread::sleep_for(std::chrono::milliseconds{ 20 });

            if (i > 60)
                g_drawer.drawRectangle(i, 40, i + 19, 60, Color{ 255, 255, 255 });
        }

        unsigned long s2{};
        while (m_is_running)
        {
            if (TransactNamedPipe(sg_provider_transporter, &s2, 4, &sg_pipe_message_atomic, sizeof sg_pipe_message_atomic, &s2, 0) == 0) [[unlikely]]
            {
                m_is_running = false;
                break;
            }

            int32_t capacity{ sg_pipe_message_atomic.load().capacity };
            int32_t boiler_number{ sg_pipe_message_atomic.load().boiler_number};

            int32_t delay{ capacity * 7 };

            for (int32_t i{ 60 }; i <= 200 + 80 * boiler_number; i += 10)
            {
                g_drawer.drawRectangle(i, 40, i + 19, 60, Color{ 200, 240, 200 });

                std::this_thread::sleep_for(std::chrono::milliseconds{ delay });

                if (i < 200 + 80 * boiler_number)
                {
                    g_drawer.drawRectangle(i, 40, i + 19, 60, Color{ 255, 255, 255 });
                }
            }

            if (TransactNamedPipe(transporter_boilers[boiler_number], &capacity, 4, &s2, 4, &s2, 0) == 0) [[unlikely]]
            {
                m_is_running = false;
                break;
            }

            for (int32_t i{ 200 + 80 * boiler_number }; i >= 60; i -= 10)
            {
                g_drawer.drawRectangle(i, 40, i + 19, 60, Color{ 200, 200, 200 });
                std::this_thread::sleep_for(std::chrono::milliseconds{ 20 });

                if (i > 60)
                    g_drawer.drawRectangle(i, 40, i + 19, 60, Color{ 255, 255, 255 });
            }
        }

        ExitProcess(0);
    }

	//--------------------------
     
    void WindowHandler::powerUnitsThread(int32_t n)
    {
        int32_t capacity{};
        int32_t pos{ 180 + n * 80 };
        unsigned long s{};
        unsigned long s2{};

        HANDLE transporter_boiler { CreateFileW(L"\\\\.\\pipe\\TransporterBoilers", GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, 0) };
        
        WriteFile(sg_boiler_provider, &n, 4, &s, 0);
        if (s == 0) [[unlikely]]
            return;

        while (m_is_running)
        {
            static_cast<void>(ReadFile(transporter_boiler, &capacity, 4, &s, 0));
            if (s == 0) [[unlikely]]
                return;

            for (int32_t i{}; i < capacity; ++i)
            {
                g_drawer.drawRectangle(pos, 85 + i * 20, pos + 59, 85 + i * 20 + 19, Color{ 255, 250, 125 });
                std::this_thread::sleep_for(std::chrono::milliseconds{ 100 });
            }

            WriteFile(transporter_boiler, &s2, 4, &s, 0);
            if (s == 0) [[unlikely]]
                return;

            for (int32_t i{ capacity - 1 }; i >= 0; --i)
            {
                g_drawer.drawRectangle(pos, 85 + i * 20, pos + 59, 85 + i * 20 + 19, Color{ 255, 255, 255 });
                std::this_thread::sleep_for(std::chrono::milliseconds{ 1500 });
            }

            WriteFile(sg_boiler_provider, &n, 4, &s, 0);
            if (s == 0) [[unlikely]]
                return;
        }

        ExitProcess(0);
    }
}