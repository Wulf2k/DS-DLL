#include "Helpers.h"
#include <Windows.h>
#include <vector>
#include <sstream>
#include <stdarg.h>  // For va_start, etc.
#include <memory>    // For std::unique_ptr

#include <filesystem>

using namespace std;
using namespace tr2::sys;

std::string ComputerNameAsString()
{
	char buf[MAX_COMPUTERNAME_LENGTH + 1];
	DWORD lpnSize = MAX_COMPUTERNAME_LENGTH;
	GetComputerNameA(buf, &lpnSize);
	return string(buf);
}

std::string extract_filename(const std::string& filepath)
{
	int pos = (int)filepath.rfind("\\");
	if (pos == std::wstring::npos)
		pos = -1;
	return std::string(filepath.begin() + pos + 1, filepath.end());
}

// Returns the full path to the module including the module name, e.g. "C:\x33-dll.dll".
std::string module_path(HMODULE hModule)
{
	char buffer[512] = { 0 };
	GetModuleFileNameA(hModule, (LPSTR)&buffer, 512);
	return std::string(buffer);
}

// Returns the file name of the module including extension, e.g. "x33-dll.dll".
std::string module_name(HMODULE hModule)
{
	char buffer[512] = { 0 };
	GetModuleFileNameA(hModule, (LPSTR)&buffer, 512);
	return extract_filename(std::string(buffer));
}

// Returns the name of the base module, e.g. "client.exe".
std::string base_module_name()
{
	return module_name(NULL);
}

void* GetLibraryProcAddress(char* LibraryName, char* ProcName) 
{
	return GetProcAddress(GetModuleHandleA(LibraryName), ProcName);
}

std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems) {
	std::stringstream ss(s);
	std::string item;
	while (std::getline(ss, item, delim)) {
		elems.push_back(item);
	}
	return elems;
}

std::vector<std::string> split(const std::string &s, char delim) {
	vector<std::string> elems;
	split(s, delim, elems);
	return elems;
}

bool TryStringFindAndReplace(wstring &s,
	const wstring &toReplace,
	const wstring &replaceWith)
{
	if (s.find(toReplace) == std::string::npos)
		return false;

	s = s.replace(s.find(toReplace), toReplace.length(), replaceWith);
	return true;
}

// Source: http://stackoverflow.com/questions/2342162/stdstring-formatting-like-sprintf
std::string string_format(const std::string fmt_str, ...) 
{
	int final_n, n = ((int)fmt_str.size()) * 2;  // Reserve two times as much as the length of the fmt_str
	std::string str;
	std::unique_ptr<char[]> formatted;
	va_list ap;
	while (1) {
		formatted.reset(new char[n]); // Wrap the plain char array into the unique_ptr 
		strcpy_s(&formatted[0], n, fmt_str.c_str());
		va_start(ap, fmt_str);
		final_n = vsnprintf(&formatted[0], n, fmt_str.c_str(), ap);
		va_end(ap);
		if (final_n < 0 || final_n >= n)
			n += abs(final_n - n + 1);
		else
			break;
	}
	return std::string(formatted.get());
}

void PrintHexBytes(BYTE* bytes, size_t len)
{
	for (size_t i = 0; i < len; i++)
	{
		printf("%02X ", bytes[i]);
	}
	printf("\n");
}

void PrintHexAndAsciiBytes(BYTE* bytes, size_t len)
{
	for (size_t i = 0; i < len; i++)
	{
		printf("%02X ", bytes[i]);
	}
	printf("\n");

	for (size_t i = 0; i < len; i++)
	{
		if (isprint(bytes[i]))
		{
			printf(" %c", bytes[i]);
		}
		else
		{
			printf("  ");
		}
	}

	printf("\n");
}

void PrintHexBytes(BYTE* arr, int length, bool separator)
{
	int pos = 0;

	while (pos < length)
	{
		if ((pos % 16 == 0) && (pos != 0))
			printf("\n");

		if (separator && (pos != 0) && (pos % 4 == 0) && (pos % 16) != 0)
			printf("| ");

		printf("%02X ", arr[pos++]);
	}
}

void WriteHexBytes(FILE* file, BYTE* arr, int length, bool separator)
{
	int pos = 0;

	while (pos < length)
	{
		if ((pos % 16 == 0) && (pos != 0))
			fprintf(file, "\n");

		if (separator && (pos != 0) && (pos % 4 == 0) && (pos % 16) != 0)
			fprintf(file, "| ");

		fprintf(file, "%02X ", arr[pos++]);
	}
}