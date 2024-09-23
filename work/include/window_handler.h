#pragma once

#include "common.h"
#include "Drawer.h"
#include "Structs.h"

//--------------------------
extern LAB::Drawer g_drawer;
//--------------------------
namespace LAB
{

	class WindowHandler final
	{
	public:

		//--------------------------
		
		explicit WindowHandler(HWND hwnd);
		
		//--------------------------
		
		~WindowHandler();

		//--------------------------
		
		static void vehicleThread();

		//--------------------------
		
		static void powerUnitsThread(int32_t n);

		//--------------------------
		
		static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

		//--------------------------

	private:

		HWND m_hwnd;
		HDC m_hdc;
		mutable std::mutex m_draw_mx;
		
		static inline std::atomic_bool m_is_running { true };
		static inline HANDLE sg_provider_transporter {};
		static inline HANDLE sg_boiler_provider {};
		static inline std::atomic<PipeMessage> sg_pipe_message_atomic {};
	};
}
