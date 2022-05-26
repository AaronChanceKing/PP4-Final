// Simple basecode showing how to create a window and attatch a vulkansurface
#define GATEWARE_ENABLE_CORE // All libraries need this
#define GATEWARE_ENABLE_SYSTEM // Graphics libs require system level libraries
#define GATEWARE_ENABLE_GRAPHICS // Enables all Graphics Libraries
// Ignore some GRAPHICS libraries we aren't going to use
#define GATEWARE_DISABLE_GDIRECTX11SURFACE // we have another template for this
#define GATEWARE_DISABLE_GDIRECTX12SURFACE // we have another template for this
#define GATEWARE_DISABLE_GRASTERSURFACE // we have another template for this
#define GATEWARE_DISABLE_GOPENGLSURFACE // we have another template for this
#define GATEWARE_ENABLE_MATH
#define GATEWARE_ENABLE_INPUT 
// With what we want & what we don't defined we can include the API
#include "../Gateware/Gateware.h"
#include "renderer.h"
#include <ShlObj.h>
#include <shobjidl.h>
#include "level_Renderer.h"
// open some namespaces to compact the code a bit
using namespace GW;
using namespace CORE;
using namespace SYSTEM;
using namespace GRAPHICS;
std::string level = "Farm";
bool end = false;
std::string loadFile();
// lets pop a window and use Vulkan to clear to a red screen
int main()
{
	while (!end)
	{
		GWindow win;
		GEventResponder msgs;
		GVulkanSurface vulkan;
		if (+win.Create(0, 0, 800, 600, GWindowStyle::WINDOWEDBORDERED))
		{
			win.SetWindowName("Aaron King---Vulkan---Final");
			VkClearValue clrAndDepth[2];
			clrAndDepth[0].color = { {0.25f, 0.15f, 0.15f, 1} };
			clrAndDepth[1].depthStencil = { 1.0f, 0u };
			msgs.Create([&](const GW::GEvent& e) {
				GW::SYSTEM::GWindow::Events q;
				if (+e.Read(q) && q == GWindow::Events::RESIZE)
					clrAndDepth[0].color.float32[2] += 0.01f; // disable
				});
			win.Register(msgs);
#ifndef NDEBUG
			const char* debugLayers[] = {
				"VK_LAYER_KHRONOS_validation", // standard validation layer
				//"VK_LAYER_LUNARG_standard_validation", // add if not on MacOS
				//"VK_LAYER_RENDERDOC_Capture" // add this if you have installed RenderDoc
			};
			if (+vulkan.Create(win, GW::GRAPHICS::DEPTH_BUFFER_SUPPORT,
				sizeof(debugLayers) / sizeof(debugLayers[0]),
				debugLayers, 0, nullptr, 0, nullptr, false))
#else
			if (+vulkan.Create(win, GW::GRAPHICS::DEPTH_BUFFER_SUPPORT))
#endif
			{

				Renderer renderer(win, vulkan, level);

				while (+win.ProcessWindowEvents())
				{
					if (+vulkan.StartFrame(2, clrAndDepth))
					{
						if (!renderer.UpdateCamera())
						{
							std::string newLevel = loadFile();
							if (newLevel != "")
							{
								level = newLevel;
							}
							break;
						}
						renderer.Render();
						vulkan.EndFrame(true);
					}
				}

				end = renderer.end;

			}
		}
	}
	return 0; // that's all folks
}

std::string loadFile()
{
	IFileOpenDialog* pFileOpen;
	PWSTR pszFilePath = NULL;

	// Create the FileOpenDialog object.
	HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL,
		IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen));
	if (SUCCEEDED(hr))
	{
		// Show the Open dialog box.
		hr = pFileOpen->Show(NULL);

		// Get the file name from the dialog box.
		if (SUCCEEDED(hr))
		{
			IShellItem* pItem;
			hr = pFileOpen->GetResult(&pItem);
			if (SUCCEEDED(hr))
			{
				//Show only the name of the file without the relitive path
				hr = pItem->GetDisplayName(SIGDN_NORMALDISPLAY, &pszFilePath);

				pItem->Release();
			}
		}
		pFileOpen->Release();
	}

	if (pszFilePath != NULL)
	{
		//Parse the value into usable string
		std::wstring ws(pszFilePath);
		std::string str(ws.begin(), ws.end());

		return str;
	}
	else return "";
}
