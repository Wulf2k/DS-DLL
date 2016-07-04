#pragma once

#include <string>
#include <vector>
#include <Windows.h>

void* GetLibraryProcAddress(char* LibraryName, char* ProcName);

std::string ComputerNameAsString();

std::string module_path(HMODULE hModule);

std::string base_module_name();
std::string module_name(HMODULE hModule);
std::string extract_filename(const std::string& filepath);

std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems);
std::vector<std::string> split(const std::string &s, char delim);

bool TryStringFindAndReplace(std::wstring &s, const std::wstring &toReplace, const std::wstring &replaceWith);
std::string string_format(const std::string fmt_str, ...);

void PrintHexBytes(BYTE* bytes, size_t len);
void PrintHexAndAsciiBytes(BYTE* bytes, size_t len);

void PrintHexBytes(BYTE* arr, int length, bool separator);
void WriteHexBytes(FILE* file, BYTE* arr, int length, bool separator);