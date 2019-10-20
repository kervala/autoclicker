/*
 *  kdAmn is a deviantART Messaging Network client
 *  Copyright (C) 2013-2015  Cedric OCHS
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "common.h"
#include "utils.h"

#ifdef Q_OS_WIN

#ifdef USE_QT5
enum HBitmapFormat
{
    HBitmapNoAlpha,
    HBitmapPremultipliedAlpha,
    HBitmapAlpha
};

QPixmap qt_pixmapFromWinHBITMAP(HBITMAP bitmap, int hbitmapFormat = 0);
#endif

#include <QtWin>
#include <windows.h>
#include <tlhelp32.h>
#include <tchar.h>
#include <stdio.h>
#include <ShellAPI.h>
#include <sdkddkver.h>

#ifdef DEBUG_NEW
#define new DEBUG_NEW
#endif

/*
#define min(a,b) (((a)<(b))?(a):(b))
#define max(a,b) (((a)>(b))?(a):(b))

#include <gdiplus.h>
#include <d3d.h>
#include <d3d9.h>
#include <d3d9helper.h>

#define WIDEN2(x) L ## x
#define WIDEN(x) WIDEN2(x)
#define __WFILE__ WIDEN(__FILE__)
#define HRCHECK(__expr) {hr=(__expr);if(FAILED(hr)){wprintf(L"FAILURE 0x%08X (%i)\n\tline: %u file: '%s'\n\texpr: '" WIDEN(#__expr) L"'\n",hr, hr, __LINE__,__WFILE__);goto cleanup;}}
#define RELEASE(__p) {if(__p!=nullptr){__p->Release();__p=nullptr;}}


QPixmap grabWindow(WId window)
{
	Gdiplus::GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR gdiplusToken;
	GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, 0);
	HDC desktopdc = GetDC((HWND)window);
	HDC hdc = CreateCompatibleDC(desktopdc);
	int width = GetSystemMetrics(SM_CXSCREEN);
	int height = GetSystemMetrics(SM_CYSCREEN);
	HBITMAP mybmp = CreateCompatibleBitmap(desktopdc, width, height);
	HBITMAP oldbmp = (HBITMAP)SelectObject(hdc, mybmp);
	BitBlt(hdc, 0, 0, width, height, desktopdc, 0, 0, SRCCOPY | CAPTUREBLT);
	SelectObject(hdc, oldbmp);

	QPixmap	res = QtWin::fromHBITMAP(mybmp);

	Gdiplus::GdiplusShutdown(gdiplusToken);
	ReleaseDC((HWND)window, desktopdc);
	DeleteObject(mybmp);
	DeleteDC(hdc);

	return res;
}


HRESULT Direct3D9TakeScreenshots(UINT adapter)
{
	HRESULT hr = S_OK;
	IDirect3D9* d3d = nullptr;
	IDirect3DDevice9* device = nullptr;
	IDirect3DSurface9* surface = nullptr;
	D3DPRESENT_PARAMETERS parameters = { 0 };
	D3DDISPLAYMODE mode;
	D3DLOCKED_RECT rc;
	UINT pitch;
	SYSTEMTIME st;
	LPBYTE shots = nullptr;

	// init D3D and get screen size
	d3d = Direct3DCreate9(D3D_SDK_VERSION);
	HRCHECK(d3d->GetAdapterDisplayMode(adapter, &mode));

	parameters.Windowed = TRUE;
	parameters.BackBufferCount = 1;
	parameters.BackBufferHeight = mode.Height;
	parameters.BackBufferWidth = mode.Width;
	parameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
	parameters.hDeviceWindow = NULL;

	// create device & capture surface
	HRCHECK(d3d->CreateDevice(adapter, D3DDEVTYPE_HAL, NULL, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &parameters, &device));
	HRCHECK(device->CreateOffscreenPlainSurface(mode.Width, mode.Height, D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, &surface, nullptr));

	// compute the required buffer size
	HRCHECK(surface->LockRect(&rc, NULL, 0));
	pitch = rc.Pitch;
	HRCHECK(surface->UnlockRect());

	// allocate screenshots buffers
	shots = new BYTE[pitch * mode.Height];

	// get the data
	HRCHECK(device->GetFrontBufferData(0, surface));

	// copy it into our buffers
	HRCHECK(surface->LockRect(&rc, NULL, 0));
	CopyMemory(shots, rc.pBits, rc.Pitch * mode.Height);
	HRCHECK(surface->UnlockRect());

cleanup:
	if (shots != nullptr)
	{
		delete shots;
	}
	RELEASE(surface);
	RELEASE(device);
	RELEASE(d3d);

	return hr;
}

HRESULT SavePixelsToFile32bppPBGRA(UINT width, UINT height, UINT stride, LPBYTE pixels, LPWSTR filePath, const GUID& format)
{
	if (!filePath || !pixels)
		return E_INVALIDARG;

	HRESULT hr = S_OK;
	IWICImagingFactory* factory = nullptr;
	IWICBitmapEncoder* encoder = nullptr;
	IWICBitmapFrameEncode* frame = nullptr;
	IWICStream* stream = nullptr;
	GUID pf = GUID_WICPixelFormat32bppPBGRA;
	BOOL coInit = CoInitialize(nullptr);

	HRCHECK(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory)));
	HRCHECK(factory->CreateStream(&stream));
	HRCHECK(stream->InitializeFromFilename(filePath, GENERIC_WRITE));
	HRCHECK(factory->CreateEncoder(format, nullptr, &encoder));
	HRCHECK(encoder->Initialize(stream, WICBitmapEncoderNoCache));
	HRCHECK(encoder->CreateNewFrame(&frame, nullptr)); // we don't use options here
	HRCHECK(frame->Initialize(nullptr)); // we dont' use any options here
	HRCHECK(frame->SetSize(width, height));
	HRCHECK(frame->SetPixelFormat(&pf));
	HRCHECK(frame->WritePixels(height, stride, stride * height, pixels));
	HRCHECK(frame->Commit());
	HRCHECK(encoder->Commit());

cleanup:
	RELEASE(stream);
	RELEASE(frame);
	RELEASE(encoder);
	RELEASE(factory);
	if (coInit) CoUninitialize();
	return hr;
}

*/

void mouseLeftClickUp(const QPoint& pos)
{
	mouse_event(MOUSEEVENTF_LEFTUP, pos.x(), pos.y(), 0, 0);
}

void mouseLeftClickDown(const QPoint& pos)
{
	mouse_event(MOUSEEVENTF_LEFTDOWN, pos.x(), pos.y(), 0, 0);
}

qint16 QKeySequenceToVK(const QKeySequence& seq)
{
	QString str = seq.toString();

	if (str.isEmpty()) return 0;

	static QMap<QString, qint16> s_keyArray;

	if (s_keyArray.isEmpty())
	{
		// special characters
		s_keyArray["Space"] = ' ';
		s_keyArray["Ins"] = VK_INSERT;
		s_keyArray["Del"] = VK_DELETE;
		s_keyArray["Esc"] = VK_ESCAPE;
		s_keyArray["Tab"] = VK_TAB;
		// s_keyArray["Backtab"] = VK_BACK;
		s_keyArray["Backspace"] = VK_BACK;
		s_keyArray["Return"] = VK_RETURN;
		s_keyArray["Pause"] = VK_PAUSE;
		s_keyArray["Print"] = VK_PRINT;
		// s_keyArray["SysReq"] = VK_SYSREQ;
		s_keyArray["Home"] = VK_HOME;
		s_keyArray["End"] = VK_END;
		s_keyArray["Left"] = VK_LEFT;
		s_keyArray["Up"] = VK_UP;
		s_keyArray["Right"] = VK_RIGHT;
		s_keyArray["Down"] = VK_DOWN;
		/*
				{ Qt::Key_PageUp,       QT_TRANSLATE_NOOP("QShortcut", "PgUp") },
				{ Qt::Key_PageDown,     QT_TRANSLATE_NOOP("QShortcut", "PgDown") },
				{ Qt::Key_CapsLock,     QT_TRANSLATE_NOOP("QShortcut", "CapsLock") },
				{ Qt::Key_NumLock,      QT_TRANSLATE_NOOP("QShortcut", "NumLock") },
				{ Qt::Key_ScrollLock,   QT_TRANSLATE_NOOP("QShortcut", "ScrollLock") },
				{ Qt::Key_Menu,         QT_TRANSLATE_NOOP("QShortcut", "Menu") },
				{ Qt::Key_Help,         QT_TRANSLATE_NOOP("QShortcut", "Help") },
		*/
		// numbers
		for (int i = '0'; i <= '9'; ++i) s_keyArray[QString(i)] = i;

		// letters
		for (int i = 'A'; i <= 'Z'; ++i) s_keyArray[QString(i)] = i;

		// function keys
		for (int i = 1; i <= 24; ++i) s_keyArray[QString("F%1").arg(i)] = VK_F1 + i - 1;
	}

	QMap<QString, qint16>::iterator it = s_keyArray.find(str);

	if (it != s_keyArray.end()) return *it;

	qDebug() << "unable to find" << str;

	return -1;
}

bool isKeyPressed(qint16 key)
{
	return GetAsyncKeyState(key) & 1;
}

QPixmap grabWindow(WId window)
{
	return QPixmap();
}

static QPixmap fancyPants( ICONINFO const &icon_info )
{
	int result;

	HBITMAP h_bitmap = icon_info.hbmColor;

	/// adapted from qpixmap_win.cpp so we can have a _non_ premultiplied alpha
	/// conversion and also apply the icon mask to bitmaps with no alpha channel
	/// remaining comments are Trolltech originals


	////// get dimensions
	BITMAP bitmap;
	memset( &bitmap, 0, sizeof(BITMAP) );

	result = GetObjectW( h_bitmap, sizeof(BITMAP), &bitmap );

	if (!result) return QPixmap();

	int const w = bitmap.bmWidth;
	int const h = bitmap.bmHeight;

	//////
	BITMAPINFO info;
	memset( &info, 0, sizeof(info) );

	info.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
	info.bmiHeader.biWidth       = w;

	info.bmiHeader.biHeight      = -h;
	info.bmiHeader.biPlanes      = 1;

	info.bmiHeader.biBitCount    = 32;
	info.bmiHeader.biCompression = BI_RGB;

	info.bmiHeader.biSizeImage   = w * h * 4;

	// Get bitmap bits
	uchar *data = new uchar[info.bmiHeader.biSizeImage];

	result = GetDIBits(GetDC(0), h_bitmap, 0, h, data, &info, DIB_RGB_COLORS );

	QPixmap p;
	if (result)
	{
		// test for a completely invisible image
		// we need to do this because there is apparently no way to determine

		// if an icon's bitmaps have alpha channels or not. If they don't the
		// alpha bits are set to 0 by default and the icon becomes invisible
		// so we do this long check. I've investigated everything, the bitmaps
		// don't seem to carry the BITMAPV4HEADER as they should, that would tell
		// us what we want to know if it was there, but apparently MS are SHIT SHIT
		const int N = info.bmiHeader.biSizeImage;

		int x;
		for (x = 3; x < N; x += 4)

			if (data[x] != 0)
				break;

		if (x < N)
		{
			p = QPixmap::fromImage( QImage( data, w, h, QImage::Format_ARGB32 ) );
		}
		else
		{
			QImage image( data, w, h, QImage::Format_RGB32 );

			QImage mask = image.createHeuristicMask();
			mask.invertPixels(); //prolly efficient as is a 1bpp bitmap really

			image.setAlphaChannel( mask );
			p = QPixmap::fromImage( image );
		}

		// force the pixmap to make a deep copy of the image data
		// otherwise `delete data` will corrupt the pixmap
		QPixmap copy = p;
		copy.detach();

		p = copy;
	}

	delete [] data;

	return p;
}

static QPixmap pixmap( const HICON &icon, bool alpha = true )
{
	try
	{
		ICONINFO info;
		::GetIconInfo(icon, &info);

		QPixmap pixmap = alpha ? fancyPants( info )
#ifdef USE_QT5
				: qt_pixmapFromWinHBITMAP(info.hbmColor, HBitmapNoAlpha);
#else
				: QPixmap::fromWinHBITMAP( info.hbmColor, QPixmap::NoAlpha );
#endif

		// gah Win32 is annoying!
		::DeleteObject( info.hbmColor );
		::DeleteObject( info.hbmMask );

		::DestroyIcon( icon );

		return pixmap;
	}
	catch (...)
	{
		return QPixmap();
	}
}

QPixmap associatedIcon( const QString &path )
{
	// performance tuned using:
	// http://www.codeguru.com/Cpp/COM-Tech/shell/article.php/c4511/

	SHFILEINFOW file_info;
	memset(&file_info, 0, sizeof(file_info));
	::SHGetFileInfoW((wchar_t*)path.utf16(), FILE_ATTRIBUTE_NORMAL, &file_info, sizeof(SHFILEINFOW), SHGFI_USEFILEATTRIBUTES | SHGFI_ICON | SHGFI_LARGEICON );

	return pixmap( file_info.hIcon );
}

struct TmpWindow
{
	HWND hWnd;
	QString name;
};

static BOOL CALLBACK EnumWindowsProc(HWND hWnd, LPARAM inst)
{
	if (IsWindowVisible(hWnd) && IsWindowEnabled(hWnd))
	{
		LONG style = GetWindowLong(hWnd, GWL_STYLE);

		if (style & (WS_THICKFRAME|WS_DLGFRAME|WS_POPUP))
		{
			wchar_t WindowName[80];

			int len = GetWindowTextW(hWnd, WindowName, 80);

			if (len > 0)
			{
				QVector<TmpWindow> *windows = (QVector<TmpWindow>*)inst;

				TmpWindow window;

				window.hWnd = hWnd;
				window.name = QString::fromWCharArray(WindowName);

				windows->push_back(window);
			}
		}
	}

	return TRUE;
}

// Windows 2000 = GetModuleFileName()
// Windows XP x32 = GetProcessImageFileName()
// Windows XP x64 = GetProcessImageFileName()

typedef BOOL (WINAPI *QueryFullProcessImageNamePtr)(HANDLE hProcess, DWORD dwFlags, LPTSTR lpExeName, PDWORD lpdwSize);
typedef DWORD (WINAPI *GetProcessImageFileNamePtr)(HANDLE hProcess, LPTSTR lpImageFileName, DWORD nSize);

static QueryFullProcessImageNamePtr pQueryFullProcessImageName = NULL;
static GetProcessImageFileNamePtr pGetProcessImageFileName = NULL;

void CreateWindowsList(QAbstractItemModel *model)
{
	if (pQueryFullProcessImageName == NULL)
	{
		pQueryFullProcessImageName = (QueryFullProcessImageNamePtr) QLibrary::resolve("kernel32", "QueryFullProcessImageNameA");
	}

	if (pGetProcessImageFileName == NULL)
	{
		pGetProcessImageFileName = (GetProcessImageFileNamePtr) QLibrary::resolve("psapi", "GetProcessImageFileNameA");
	}

	HMODULE module = GetModuleHandle(NULL);
	QFileIconProvider icon;

	QVector<TmpWindow> currentWindows;

	// list hWnd
	HANDLE hThreadSnap = INVALID_HANDLE_VALUE;
	THREADENTRY32 te32;

	// Fill in the size of the structure before using it.
	te32.dwSize = sizeof(THREADENTRY32);

	// Take a snapshot of all running threads
	hThreadSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);

	// Retrieve information about the first thread,
	if ((hThreadSnap != INVALID_HANDLE_VALUE) && Thread32First(hThreadSnap, &te32))
	{
		// Now walk the thread list of the system,
		// and display information about each thread
		// associated with the specified process
		do
		{
			currentWindows.clear();

			EnumThreadWindows(te32.th32ThreadID, EnumWindowsProc, (LPARAM)&currentWindows);

			if (!currentWindows.empty() && te32.th32OwnerProcessID)
			{
				for(int i = 0; i < currentWindows.size(); ++i)
				{
					HWND hWnd = currentWindows[i].hWnd;

					// get process handle
					DWORD pidwin;
					GetWindowThreadProcessId(hWnd, &pidwin);
					HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pidwin);

					// get process path
					char szProcessPath[MAX_PATH];
					DWORD bufSize = MAX_PATH;

					QString processPath;

					if (pQueryFullProcessImageName != NULL)
					{
						if (pQueryFullProcessImageName(hProcess, 0, (LPTSTR)&szProcessPath, &bufSize) == 0)
						{
							DWORD error = GetLastError();

							qDebug() << "Error" << error;
						}
					}
					else if (pGetProcessImageFileName != NULL)
					{
						bufSize = pGetProcessImageFileName(hProcess, (LPTSTR)&szProcessPath, bufSize);
					}

					processPath = QString::fromLatin1(szProcessPath, bufSize);

					HICON hIcon = NULL;

					UINT count = ExtractIconEx(processPath.toLatin1().data(), -1, NULL, NULL, 1);

					if (count < 1) continue;

					UINT res = ExtractIconEx(processPath.toLatin1().data(), 0, &hIcon, NULL, 1);

					QPixmap pixmap = ::pixmap(hIcon);

					DestroyIcon(hIcon);

					if (pixmap.isNull()) pixmap = icon.icon(QFileIconProvider::File).pixmap(32, 32);

					if (model->insertRow(0))
					{
						QModelIndex index = model->index(0, 0);

						model->setData(index, currentWindows[i].name);
						model->setData(index, pixmap, Qt::DecorationRole);
						model->setData(index, qVariantFromValue((void*)currentWindows[i].hWnd), Qt::UserRole);
					}
				}
			}
		}
		while(Thread32Next(hThreadSnap, &te32));

		CloseHandle(hThreadSnap);

		model->sort(0);
	}

	if (model->insertRow(0))
	{
		QModelIndex index = model->index(0, 0);

		model->setData(index, QObject::tr("Whole screen"));
		model->setData(index, icon.icon(QFileIconProvider::Desktop).pixmap(32, 32), Qt::DecorationRole);
		model->setData(index, qVariantFromValue((void*)NULL), Qt::UserRole);
	}
}

bool RestoreMinimizedWindow(WId &id)
{
	if (id)
	{
		WINDOWPLACEMENT placement;
		memset(&placement, 0, sizeof(placement));

		if (GetWindowPlacement((HWND)id, &placement))
		{
			if (placement.showCmd == SW_SHOWMINIMIZED)
			{
				ShowWindow((HWND)id, SW_RESTORE);
				// time needed to restore window
				Sleep(500);

				return true;
			}
		}
	}
	else
	{
		id = QApplication::desktop()->winId();
		// time needed to hide capture dialog
		Sleep(500);
	}

	return false;
}

void MinimizeWindow(WId id)
{
	ShowWindow((HWND)id, SW_MINIMIZE);
}

bool IsUsingComposition()
{
	typedef BOOL (*voidfuncPtr)(void);

	HINSTANCE hInst = LoadLibraryA("UxTheme.dll");

	bool ret = false;

	if (hInst)
	{
		voidfuncPtr fctIsCompositionActive = (voidfuncPtr)GetProcAddress(hInst, "IsCompositionActive");

		if (fctIsCompositionActive)
		{
			// only if compositing is not activated
			if (fctIsCompositionActive())
				ret = true;
		}

		FreeLibrary(hInst);
	}

	return ret;
}

void PutForegroundWindow(WId id)
{
	SetForegroundWindow((HWND)id);
	Sleep(500);
}

typedef BOOL (WINAPI *LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);

bool IsOS64bits()
{
	bool res;

#ifdef _WIN644
	res = true;
#else
	res = false;

	// IsWow64Process is not available on all supported versions of Windows.
	// Use GetModuleHandle to get a handle to the DLL that contains the function
	// and GetProcAddress to get a pointer to the function if available.
	HMODULE module = GetModuleHandleA("kernel32");

	if (module)
	{
		LPFN_ISWOW64PROCESS fnIsWow64Process = (LPFN_ISWOW64PROCESS)GetProcAddress(module, "IsWow64Process");

		if (fnIsWow64Process)
		{
			BOOL bIsWow64 = FALSE;

			if (fnIsWow64Process(GetCurrentProcess(), &bIsWow64))
			{
				res = bIsWow64 == TRUE;
			}
		}
	}
#endif
	return res;
}

#endif
