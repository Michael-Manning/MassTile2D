#pragma once

#include <Windows.h>
#include <string>
#include <vector>
#include <filesystem>

// show an open/save file dialog. optionally allow multi item select. Supplying an owner window will cause it to act as a modal dialog
static bool FileDialog(
	wchar_t* buffer, 
	int bufflen, 
	const wchar_t* extension, 
	const wchar_t* filter, 
	bool open, 
	bool multi, 
	HWND owner = NULL, 
	const wchar_t* windowTitle = NULL, 
	std::vector<std::filesystem::path>* multiFiles = nullptr) {


	std::filesystem::path initialPath = std::filesystem::current_path();

	OPENFILENAME ofn;       // common dialog box structure
	HANDLE hf;              // file handle

	// we need to assume the buffer is totally null for this
	if (multi) {
		std::fill(buffer, buffer + bufflen, '\0');
	}

	// Initialize OPENFILENAME
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = owner;
	ofn.lpstrFile = buffer;
	// Set lpstrFile[0] to '\0' so that GetOpenFileName does not 
	// use the contents of szFile to initialize itself.
	ofn.lpstrFile[0] = '\0';
	ofn.nMaxFile = bufflen;
	ofn.lpstrFilter = filter;
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.lpstrDefExt = extension;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	if (windowTitle != NULL) {
		ofn.lpstrTitle = windowTitle;
	}
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_EXPLORER | (multi ? OFN_ALLOWMULTISELECT : 0);

	// Display the Open dialog box. 
	if (open) {
		bool res = GetOpenFileName(&ofn) == TRUE;

		// reset relative directories
		std::filesystem::current_path(initialPath);

		if (!res)
			return false;

		if (!multi) {
			return true;
		}

		wchar_t* p = ofn.lpstrFile;
		std::filesystem::path directory = p;
		p += wcslen(p) + 1;

		if (*p) {
			// Multiple files were selected
			while (*p) {
				multiFiles->push_back(directory / p); // Use filesystem path concatenation
				p += wcslen(p) + 1;
			}
		}
		else {
			multiFiles->push_back(directory);
		}
		return true;
	}
	else {
		return GetSaveFileName(&ofn) == TRUE;
	}
}

static bool openFileDialog(wchar_t* buffer, int bufflen, const wchar_t* extention, const wchar_t* filter, HWND owner = NULL, const wchar_t* windowTitle = NULL) {
	return FileDialog(buffer, bufflen, extention, filter, true, false, owner, windowTitle);
}
static std::vector<std::filesystem::path> openFileDialogMulti(const wchar_t* extention, const wchar_t* filter, HWND owner = NULL, const wchar_t* windowTitle = NULL) {
	wchar_t unusedBuffer[MAX_PATH * 100];
	std::vector<std::filesystem::path> multiFiles;
	FileDialog(unusedBuffer, MAX_PATH * 100, extention, filter, true, true, owner, windowTitle, &multiFiles);
	return multiFiles;
}
static bool saveFileDialog(wchar_t* buffer, int bufflen, const wchar_t* extention, const wchar_t* filter, HWND owner = NULL, const wchar_t* windowTitle = NULL) {
	return FileDialog(buffer, bufflen, extention, filter, false, false, owner, windowTitle);
}