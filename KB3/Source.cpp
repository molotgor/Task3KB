#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <string>
#include <iostream>
#include <filesystem>
#include <windows.h>
#include <vector>

using namespace std;

DWORD align(DWORD size, DWORD align, DWORD addr) {
    if (!(size % align))
        return addr + size;
    return addr + (size / align + 1) * align;
}

bool AddSection(char* filepath, char* sectionName, DWORD sizeOfSection) {
    HANDLE file = CreateFile(filepath, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE)
        return false;
    DWORD fileSize = GetFileSize(file, NULL);
    BYTE* pByte = new BYTE[fileSize];
    DWORD dw;
    ReadFile(file, pByte, fileSize, &dw, NULL);

    PIMAGE_DOS_HEADER dos = (PIMAGE_DOS_HEADER)pByte;
    if (dos->e_magic != IMAGE_DOS_SIGNATURE)
        return false;
    PIMAGE_FILE_HEADER FH = (PIMAGE_FILE_HEADER)(pByte + dos->e_lfanew + sizeof(DWORD));
    PIMAGE_OPTIONAL_HEADER OH = (PIMAGE_OPTIONAL_HEADER)(pByte + dos->e_lfanew + sizeof(DWORD) + sizeof(IMAGE_FILE_HEADER));
    PIMAGE_SECTION_HEADER SH = (PIMAGE_SECTION_HEADER)(pByte + dos->e_lfanew + sizeof(IMAGE_NT_HEADERS));

    ZeroMemory(&SH[FH->NumberOfSections], sizeof(IMAGE_SECTION_HEADER));
    CopyMemory(&SH[FH->NumberOfSections].Name, sectionName, 8);

    SH[FH->NumberOfSections].Misc.VirtualSize = align(sizeOfSection, OH->SectionAlignment, 0);
    SH[FH->NumberOfSections].VirtualAddress = align(SH[FH->NumberOfSections - 1].Misc.VirtualSize, OH->SectionAlignment, SH[FH->NumberOfSections - 1].VirtualAddress);
    SH[FH->NumberOfSections].SizeOfRawData = align(sizeOfSection, OH->FileAlignment, 0);
    SH[FH->NumberOfSections].PointerToRawData = align(SH[FH->NumberOfSections - 1].SizeOfRawData, OH->FileAlignment, SH[FH->NumberOfSections - 1].PointerToRawData);
    SH[FH->NumberOfSections].Characteristics = 0xE00000E0;

    SetFilePointer(file, SH[FH->NumberOfSections].PointerToRawData + SH[FH->NumberOfSections].SizeOfRawData, NULL, FILE_BEGIN);
    SetEndOfFile(file);
    OH->SizeOfImage = SH[FH->NumberOfSections].VirtualAddress + SH[FH->NumberOfSections].Misc.VirtualSize;
    FH->NumberOfSections += 1;
    SetFilePointer(file, 0, NULL, FILE_BEGIN);
    WriteFile(file, pByte, fileSize, &dw, NULL);
    CloseHandle(file);
    return true;
}

bool AddCode(char* filepath) {
    HANDLE file = CreateFile(filepath, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE)
        return false;
    DWORD filesize = GetFileSize(file, NULL);
    BYTE* pByte = new BYTE[filesize];
    DWORD dw;
    ReadFile(file, pByte, filesize, &dw, NULL);
    PIMAGE_DOS_HEADER dos = (PIMAGE_DOS_HEADER)pByte;
    PIMAGE_NT_HEADERS nt = (PIMAGE_NT_HEADERS)(pByte + dos->e_lfanew);

    PIMAGE_SECTION_HEADER first = IMAGE_FIRST_SECTION(nt);
    PIMAGE_SECTION_HEADER last = first + (nt->FileHeader.NumberOfSections - 1);

    SetFilePointer(file, last->PointerToRawData, NULL, FILE_BEGIN);
    char str[] = "TEST TEXT";
    WriteFile(file, str, strlen(str), &dw, 0);
    CloseHandle(file);
    return TRUE;
}

string getFirstExeFileInFolder(string folder) {
    string search_path = folder + "/*.*";
    WIN32_FIND_DATA fd;
    HANDLE hFind = ::FindFirstFile(search_path.c_str(), &fd);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                string filename = fd.cFileName;
                if (filename.substr(filename.find_last_of(".") + 1) == "exe") {
                    return folder + "\\" + filename;
                }
            }
        } while (::FindNextFile(hFind, &fd));
        ::FindClose(hFind);
    }
    return "";
}

void main()
{
    char url[] = "C:\\Users\\Public";
    string exeurl = getFirstExeFileInFolder(url);
    char ext[] = ".TEST";
    char* path = new char[exeurl.length() + 1];
    strcpy(path, exeurl.c_str());
    if (AddSection(path, ext, 400)) {
        printf("Section added!\n");

        if (AddCode(path)) {
            printf("Code written!\n");
        }
        else
            printf("Error writting code!\n");
    }
    else
        printf("Error adding section!\n");
}