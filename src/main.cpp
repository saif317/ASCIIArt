#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <iostream>
#include <Windows.h>
#include <cstring>
#include <utility>
#include <limits>
#include <fstream>
#include <filesystem>

HWND hWnd = GetConsoleWindow();
HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
SMALL_RECT rectWindow = { 0,0,1,1 };
COORD coord = { 120,40 };
CONSOLE_FONT_INFOEX cfi;

void WindowToMaterial(std::string sFileName) {
	hWnd = GetConsoleWindow();
	HDC hWindowDC = GetDC(hWnd);

	HDC hwindowCompatibleDC = CreateCompatibleDC(hWindowDC);

	SetStretchBltMode(hwindowCompatibleDC, COLORONCOLOR);

	HBITMAP hbwindow = CreateCompatibleBitmap(hWindowDC, coord.X * 3, coord.Y * 3);

	BITMAPINFOHEADER bi;
	bi.biSize = sizeof(BITMAPINFOHEADER);
	bi.biWidth = coord.X * 3;
	bi.biHeight = -coord.Y * 3;
	bi.biPlanes = 1;
	bi.biBitCount = 32;
	bi.biCompression = BI_RGB;
	bi.biSizeImage = 0;
	bi.biXPelsPerMeter = 0;
	bi.biYPelsPerMeter = 0;
	bi.biClrUsed = 0;
	bi.biClrImportant = 0;

	SelectObject(hwindowCompatibleDC, hbwindow);

	StretchBlt(hwindowCompatibleDC, 0, 0, coord.X * 3, coord.Y * 3, hWindowDC, 0, 3, coord.X * 3, (coord.Y * 3) + 3, SRCCOPY);

	cv::Mat img;
	img.create(coord.Y * 3, coord.X * 3, CV_8UC4);

	GetDIBits(hWindowDC, hbwindow, 0, coord.Y * 3, img.data, (BITMAPINFO*)&bi, DIB_RGB_COLORS);

	DeleteObject(hbwindow);
	DeleteDC(hwindowCompatibleDC);

	size_t i = sFileName.size() - 1;
	std::string sSaveLocation;
	do {
		sSaveLocation += sFileName.at(i);
		i--;
	} while (sFileName.at(i) != '/');
	std::reverse(sSaveLocation.begin(), sSaveLocation.end());

	wchar_t EXEPath[MAX_PATH];
	GetModuleFileName(NULL, EXEPath, MAX_PATH);

	for (int i = 0; i < wcslen(EXEPath); i++)
	{
		if (EXEPath[i] == '\\')
			EXEPath[i] = '/';
	}

	i = wcslen(EXEPath) - 1;
	std::string sWorkingDirectory;
	do {
		EXEPath[i] = '\0';
		i--;
	} while (EXEPath[i] != '/');

	SetCurrentDirectory(EXEPath);

	system("mkdir images");
	sSaveLocation = "images/" + sSaveLocation;
	cv::imwrite(sSaveLocation, img);
}

void FullScreen(cv::Mat image, int fontSize) {
	rectWindow = { 0, 0, 1, 1 };
	SetConsoleWindowInfo(hConsole, TRUE, &rectWindow);

	coord = { (short)image.cols,(short)image.rows };
	SetConsoleScreenBufferSize(hConsole, coord);

	SetConsoleActiveScreenBuffer(hConsole);

	cfi.dwFontSize.X = fontSize;
	cfi.dwFontSize.Y = fontSize;
	SetCurrentConsoleFontEx(hConsole, false, &cfi);

	rectWindow = { 0, 3, coord.X - 1, coord.Y - 1 };
	SetConsoleWindowInfo(hConsole, TRUE, &rectWindow);

	SetWindowLongPtr(hWnd, GWL_STYLE, WS_POPUP);
	ShowWindow(hWnd, SW_MAXIMIZE);
}

void Fit(cv::Mat& image, int fontSize) {
	RECT desktop;
	GetWindowRect(GetDesktopWindow(), &desktop);

	if (image.cols > desktop.right / fontSize || image.rows > desktop.bottom / fontSize) {
		float fResizeFactorX = float(image.cols) / (float(desktop.right) / (float)fontSize);
		cv::resize(image, image, cv::Size(image.cols / fResizeFactorX, image.rows), 0, 0, cv::InterpolationFlags::INTER_LINEAR_EXACT);

		float fResizeFactorY = float(image.rows) / (float(desktop.bottom) / (float)fontSize);
		cv::resize(image, image, cv::Size(image.cols, image.rows / fResizeFactorY), 0, 0, cv::InterpolationFlags::INTER_LINEAR_EXACT);
	}
}

std::string SelectImage()
{
	OPENFILENAME ofn;
	wchar_t szFile[260];
	std::string sFileName;

	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.lpstrFile = szFile;
	ofn.lpstrFile[0] = '\0';
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFilter = L"All\0*.*\0jpg\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

	std::cout << "Select Image: " << std::endl;

	if (GetOpenFileName(&ofn) == TRUE)
		for (int i = 0; i < wcslen(ofn.lpstrFile); i++)
		{
			if (ofn.lpstrFile[i] == '\\')
				sFileName += '/';
			else
				sFileName += ofn.lpstrFile[i];
		}

	system("cls");
	std::cout << " Image location: " << sFileName << std::endl;
	return sFileName;
}

cv::Mat Draw() {
	cv::Mat image;
	const char* const ASCII = " .:-=+*#%@";
	std::vector<char> cArt;

	std::string sImageLocation = SelectImage();

	char cMode;
	std::cout << "Select ASCII art type" << std::endl
		<< "1 - Gray Scale" << std::endl
		<< "2 - Matrix" << std::endl
		<< "3 - Inverted Grey Scale" << std::endl << std::endl
		<< "Enter type: ";
	std::cin >> cMode;

	int nFontSize;
	std::cout << std::endl << "Enter Font Size - Minimum is 3px: ";
	std::cin >> nFontSize;

	image = cv::imread(sImageLocation, cv::IMREAD_GRAYSCALE);

	Fit(image, nFontSize);

	FullScreen(image, nFontSize);
	system("cls");

	CHAR_INFO* buffer = new CHAR_INFO[image.cols * image.rows];
	memset(buffer, 0, image.cols * image.rows);

	if (!image.empty())
	{
		for (int y = 0; y < image.rows; y++)
			for (int x = 0; x < image.cols; x++)
				cArt.push_back(ASCII[int(roundf(float(image.at<uchar>(y, x)) * 10.0f / 255.0f))]);

		for (int x = 0; x < cArt.size(); x++)
			buffer[x].Char.AsciiChar = cArt.at(x);

		if (cMode == '1')
			for (int x = 0; x < cArt.size(); x++)
				buffer[x].Attributes = 0x000F;

		if (cMode == '2')
			for (int x = 0; x < cArt.size(); x++)
				buffer[x].Attributes = 0x000A;

		if (cMode == '3')
			for (int x = 0; x < cArt.size(); x++)
				buffer[x].Attributes = 0x00F0;
	}

	WriteConsoleOutputA(hConsole, buffer, coord, { 0,0 }, &rectWindow);
	delete[] buffer;

	WindowToMaterial(sImageLocation);

	image.release();

	return image;
}

void ConstructConsole() {
	SetConsoleTitle(TEXT("ASCII Art"));

	rectWindow = { 0, 0, 1, 1 };
	SetConsoleWindowInfo(hConsole, TRUE, &rectWindow);

	SetConsoleScreenBufferSize(hConsole, coord);

	SetConsoleActiveScreenBuffer(hConsole);

	cfi.cbSize = sizeof(cfi);
	cfi.nFont = 0;
	cfi.dwFontSize.X = 8;
	cfi.dwFontSize.Y = 16;
	cfi.FontFamily = FF_DONTCARE;
	cfi.FontWeight = FW_NORMAL;
	wcscpy_s(cfi.FaceName, L"Consolas");
	SetCurrentConsoleFontEx(hConsole, false, &cfi);

	rectWindow = { 0, 0, 119, 39 };
	SetConsoleWindowInfo(hConsole, TRUE, &rectWindow);

	SetWindowLongPtr(hWnd, GWL_STYLE, WS_TILEDWINDOW);
	ShowWindow(hWnd, SW_SHOWNORMAL);
}

int main()
{
	ConstructConsole();
	Draw();
	system("pause");
	system("cls");

	ConstructConsole();

	std::cout << "Your converted image has been saved to the images folder" << std::endl;
	system("pause");

	CloseHandle(hConsole);

	return 0;
}