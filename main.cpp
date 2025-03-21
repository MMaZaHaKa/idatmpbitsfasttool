#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

// 0x20100000 1st bytes *.elf binary
// 0x20000000 + poiner -> physical address
#define P_PCSX2_BASE 0x20000000
#define P_ELF_BASE  0x00100000 // P_PCSX2_BASE + P_ELF_BASE = 1st byte elf program
#define IDATRANSLATE(p) (((uintptr_t)p) + P_PCSX2_BASE) // IDA -> PCSX2
#define PCSXTRANSLATE(p) (((uintptr_t)p) - P_PCSX2_BASE) // PCSX2 -> IDA
#define PCSX2POINTER(p) (((uintptr_t)p) + P_PCSX2_BASE) // VIRTUAL -> PHYSICAL

//strhexlen(hexPart)#define EXIT // no while
#define M1_FLAGS_NAME
#define BASECOL (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE)

void copyToClipboard(const char* text) {
    if (OpenClipboard(NULL)) {
        EmptyClipboard();
        size_t len = strlen(text) + 1;
        HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, len);
        if (hMem) {
            memcpy(GlobalLock(hMem), text, len);
            GlobalUnlock(hMem);
            SetClipboardData(CF_TEXT, hMem);
        }
        CloseClipboard();
    }
}

void Mode0()
{
    char input[20];
    unsigned long long address;
    int choice;
#ifndef EXIT
    while (1)
#endif
    {
        printf("Enter HEX pointer (or 'exit'): ");
        scanf("%19s", input);
        if (!strcmp(input, "exit")) { return; }

        address = strtoull(input, NULL, 16);

        printf("Enter action: 0 - IDA2PCSX, 1 - PCSX2IDA: ");
        scanf("%d", &choice);

        if (choice) // PCSX2 -> IDA
        {
            address = PCSXTRANSLATE(address);
            printf("[out:IDA](copied): 0x%llX\n", address);
        }
        else // IDA -> PCSX2
        {
            address = IDATRANSLATE(address);
            printf("[out:PCSX2](copied): 0x%llX\n", address);
        }
        char buffer[20];
        sprintf(buffer, "0x%llX", address);
        copyToClipboard(buffer);
    }
}

size_t strhexlen(const char* str) // ~0x200000000040000ui64;   :15
{
    size_t length = 0;
    if (!str) { return 0; }
    // ������� ��������� "0x" ��� "0X" �� ����� ����� ������
    while (*str == '+' || *str == '-' || *str == '~' || *str == ' ' || *str == '\t' || (*str == '0' && (str[1] == 'x' || str[1] == 'X'))) {
        // ���� �������� ������� "0x" ��� "0X", ����������
        if (*str == '0' && (str[1] == 'x' || str[1] == 'X')) {
            str += 2;  // ���������� "0x" ��� "0X"
        }
        else {
            str++;  // ���������� ������� +, -, ~, �������
        }
    }

    // ������� �������� �������� � ��������� 0-9, A-F
    while (*str) {
        char c = toupper((unsigned char)*str);  // ����������� � ������� �������
        if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F')) {
            length++;
        }
        else {
            break;  // ���� �������� ���������� ������, ���������
        }
        str++;
    }

    return length;
}


#ifdef M1_FLAGS_NAME
void PrintBits(unsigned long long value, unsigned long long maskOn, unsigned long long maskOff, char flagChar, char* buffer = NULL, int numBits = 64)
#else
void PrintBits(unsigned long long value, unsigned long long maskOn, unsigned long long maskOff, char* buffer = NULL, int numBits = 64)
#endif
{
#ifdef M1_FLAGS_NAME
    int totalFlags = numBits / 8;
    if (flagChar) { flagChar += totalFlags - (numBits % 8 == 0 ? 1 : 0); } // �������� � �����
#endif

    int onCount = 0;
    int offCount = 0;
    for (int i = 0; i < numBits; ++i) {
        unsigned long long isOn = (maskOn >> i) & 1;
        unsigned long long isOff = (maskOff >> i) & 1;

        if (isOn) { onCount++; }
        if (isOff) { offCount++; }
    }

    int pos = 0;
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    for (int i = numBits - 1; i >= 0; --i) {
        unsigned long long bit = (value >> i) & 1;
        unsigned long long isOn = (maskOn >> i) & 1;
        unsigned long long isOff = (maskOff >> i) & 1;

#ifdef M1_FLAGS_NAME
        if (((i % 8 == 7) || (numBits % 8 != 0 && i == numBits - 1)) && flagChar)
        {
            SetConsoleTextAttribute(hConsole, FOREGROUND_BLUE | FOREGROUND_INTENSITY);
            printf("(%c)", flagChar);
            if (buffer) pos += sprintf(buffer + pos, "(%c)", flagChar);
            flagChar--; // ��������� �����
        }
#endif

        if ((isOn && (offCount < onCount)) || (isOff && (offCount > onCount))) { SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY); } // bg
        else if ((isOn && (offCount > onCount)) || (isOff && (offCount < onCount))) { SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN | FOREGROUND_INTENSITY); } // focus
        else { SetConsoleTextAttribute(hConsole, BASECOL); }

        printf("%d", bit);
        if (buffer) pos += sprintf(buffer + pos, "%d", bit);

        if (i % 8 == 0) {
            printf(" ");
            if (buffer) pos += sprintf(buffer + pos, " ");
        }
    }
    printf("\n");
    if (buffer) { buffer[pos] = '\0'; }
    SetConsoleTextAttribute(hConsole, BASECOL);
}

void Mode1()
{ // 0xFDFFFFFFFFFBFFFFui64;
    // ~0x200000000040000ui64;
    char bitBuffer[80];
    char input[30];
#ifdef M1_FLAGS_NAME
    char flagName = '\0';
#endif
    unsigned long long value, maskOn = 0, maskOff = 0;
#ifndef EXIT
    while (1)
#endif
    {
        printf("Enter HEX value [BITS MODE] (or 'exit'): ");
        scanf("%29s", input);
        if (!strcmp(input, "exit")) { return; }

#ifdef M1_FLAGS_NAME
        printf("Enter flag character (or press Enter to skip): ");
        while (getchar() != '\n'); // ������� ����� ����� ����� �������
        flagName = getchar();
        if (flagName == '\r' || flagName == '\n') { flagName = '\0'; }
#endif

        int isNegated = (input[0] == '~');
        char* hexPart = isNegated ? input + 1 : input;

        if (strstr(hexPart, "0x") == hexPart || strstr(hexPart, "0X") == hexPart) {
            hexPart += 2;
        }

        //int numBits = strhexlen(hexPart) * 4; // no round 2
        int numBits = ((strhexlen(hexPart) + 1) / 2 * 2) * 4; // 1char 4bit parse ~0x0200000000040000ui64; is 64bits, 0XFF is 8bits, 0x0FF is 12bits
        value = strtoull(hexPart, NULL, 16);

        if (isNegated) {
            maskOn = ~value;
            maskOff = value;
            value = maskOn;
        }
        else {
            maskOn = value;
            maskOff = ~value;
        }

        printf("Bits [copied<--]: ");
#ifdef M1_FLAGS_NAME
        PrintBits(value, maskOn, maskOff, flagName, bitBuffer, numBits);
#else
        PrintBits(value, maskOn, maskOff, bitBuffer, numBits);
#endif
        copyToClipboard(bitBuffer);
    }
}

int main()
{
    bool bIsMode1 = (GetAsyncKeyState(VK_SHIFT) & 0x8000) || (GetAsyncKeyState(VK_CONTROL) & 0x8000);
    if (bIsMode1) { Mode1(); }
    else { Mode0(); }
    return 0;
}

