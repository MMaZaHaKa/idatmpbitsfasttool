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

void inline SetColor(WORD wAttributes) { SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), wAttributes); }

#define CW_R() SetColor(FOREGROUND_RED)                          
#define CW_G() SetColor(FOREGROUND_GREEN)                  
#define CW_B() SetColor(FOREGROUND_BLUE)                    
#define CW_Y() SetColor(FOREGROUND_RED | FOREGROUND_GREEN)      
#define CW_C() SetColor(FOREGROUND_GREEN | FOREGROUND_BLUE)        
#define CW_M() SetColor(FOREGROUND_RED | FOREGROUND_BLUE)         
#define CW_W() SetColor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE)
#define CW_K() SetColor(0)                                         

//strhexlen(hexPart)#define EXIT // no while
#define M1_FLAGS_NAME // спросить букву байта флага
#define LOG_MASK_POS // доп инфа битов, байтов индексы
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
    // Пропуск префиксов "0x" или "0X" на любом месте строки
    while (*str == '+' || *str == '-' || *str == '~' || *str == ' ' || *str == '\t' || (*str == '0' && (str[1] == 'x' || str[1] == 'X'))) {
        // Если встречен префикс "0x" или "0X", пропустить
        if (*str == '0' && (str[1] == 'x' || str[1] == 'X')) {
            str += 2;  // Пропустить "0x" или "0X"
        }
        else {
            str++;  // Пропускать символы +, -, ~, пробелы
        }
    }

    // Подсчет валидных символов в диапазоне 0-9, A-F
    while (*str) {
        char c = toupper((unsigned char)*str);  // Преобразуем в верхний регистр
        if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F')) {
            length++;
        }
        else {
            break;  // Если встречен невалидный символ, завершить
        }
        str++;
    }

    return length;
}


#ifdef M1_FLAGS_NAME
void PrintBits(unsigned long long value, unsigned long long maskOn, unsigned long long maskOff, char fChar, char* buffer = NULL, int numBits = 64)
#else
void PrintBits(unsigned long long value, unsigned long long maskOn, unsigned long long maskOff, char* buffer = NULL, int numBits = 64)
#endif
{
#ifdef M1_FLAGS_NAME
    char flagChar = fChar;
    int totalFlags = numBits / 8;
    if (flagChar) { flagChar += totalFlags - (numBits % 8 == 0 ? 1 : 0); } // Начинаем с конца
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
            flagChar--; // Двигаемся назад
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

#ifdef LOG_MASK_POS
    // Печать позиций битов меньшинства по байтам (byte0 = LSB). Формат: [byteN, b1,b2,...]
    int totalBytes = (numBits + 7) / 8;
    int anyPrinted = 0;
    if (buffer) pos += sprintf(buffer + pos, "\n");
    printf("%s ", (onCount < offCount) ? "ON" : "OFF");
    if (buffer) pos += sprintf(buffer + pos, "%s ", (onCount > offCount) ? "ON" : "OFF");

    char fch = fChar;
    if (fch) { fch += totalBytes - (numBits % 8 == 0 ? 1 : 0); }
    //for (int b = 0; b < totalBytes; ++b)
    for (int b = totalBytes - 1; b >= 0; --b)
    {
        int start = b * 8;                 // глобальный индекс младшего бита в байте
        int end = start + 7;               // глобальный индекс старшего бита в байте
        if (end >= numBits) end = numBits - 1;
        if (start > end) continue;

        int ones = 0, zeros = 0;
        for (int k = start; k <= end; ++k) {
            if ((value >> k) & 1ULL) ++ones; else ++zeros;
        }
        //if (ones == zeros) continue; // нет меньшинства пропускаем
        if (ones == zeros) { if (fch) fch--; continue; }

        int minority = (zeros > ones) ? 1 : 0;

        int idxs[8]; int idxc = 0;
        for (int k = start; k <= end; ++k) {
            if (((value >> k) & 1ULL) == (unsigned)minority) idxs[idxc++] = k - start; // bitInByte 0..7
        }
        if (idxc == 0) continue;

        // сортируем (максимум 8 элементов простой сорт)
        for (int i = 0; i < idxc - 1; ++i)
            for (int j = i + 1; j < idxc; ++j)
                if (idxs[i] > idxs[j]) { int t = idxs[i]; idxs[i] = idxs[j]; idxs[j] = t; }

        if (anyPrinted) { printf(", "); if (buffer) pos += sprintf(buffer + pos, ", "); }
#ifdef M1_FLAGS_NAME
        if (fch) {
            printf("(%c)", fch);
            if (buffer) pos += sprintf(buffer + pos, "(%c)", fch);
            fch--;
        }
#endif
        printf("[byte%d, ", b);
        if (buffer) pos += sprintf(buffer + pos, "[byte%d, ", b);
        for (int i = 0; i < idxc; ++i) {
            if (i) { printf(","); if (buffer) pos += sprintf(buffer + pos, ","); }
            printf("%d", idxs[i]);
            if (buffer) pos += sprintf(buffer + pos, "%d", idxs[i]);
        }
        printf("]");
        if (buffer) pos += sprintf(buffer + pos, "]");
        anyPrinted = 1;
    }

    printf("\n");
    if (buffer) pos += sprintf(buffer + pos, "\n");
#endif

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
        //printf("Enter HEX value [BITS MODE] (or 'exit'): ");
        printf("Enter DEC or HEX(0x) value ");
        CW_M();
        printf("[BITS MODE]");
        CW_W();
        printf(" (or 'exit'): ");

        scanf("%29s", input);
        if (!strcmp(input, "exit")) { return; }

#ifdef M1_FLAGS_NAME
        printf("Enter flag character (or press Enter to skip): ");
        while (getchar() != '\n'); // Очищаем буфер после ввода символа
        flagName = getchar();
        if (flagName == '\r' || flagName == '\n') { flagName = '\0'; }
#endif

        int isNegated = (input[0] == '~');
        char* numStr = isNegated ? input + 1 : input;

        int base = 10;
        if (strstr(numStr, "0x") == numStr || strstr(numStr, "0X") == numStr) {
            numStr += 2;
            base = 16;
        }
        else if (*numStr == 'x' || *numStr == 'X') {
            numStr++;
            base = 16;
        }
        //else {
        //    // Check for any hex digit A-F to switch base
        //    for (char* p = numStr; *p; ++p) {
        //        if ((toupper(*p) >= 'A' && toupper(*p) <= 'F')) {
        //            base = 16;
        //            break;
        //        }
        //    }
        //}

        //int numBits = strhexlen(numStr) * 4; // no round 2
        int numBits = ((strhexlen(numStr) + 1) / 2 * 2) * 4; // 1char 4bit parse ~0x0200000000040000ui64; is 64bits, 0XFF is 8bits, 0x0FF is 12bits
        value = strtoull(numStr, NULL, base);

        if (isNegated) {
            maskOn = ~value;
            maskOff = value;
            value = maskOn;
        }
        else {
            maskOn = value;
            maskOff = ~value;
        }

        //printf("Bits [copied<--]: ");
        printf("Bits [copied<--] (%llu, 0x%llX): ", value, value);
#ifdef M1_FLAGS_NAME
        PrintBits(value, maskOn, maskOff, flagName, bitBuffer, numBits);
#else
        PrintBits(value, maskOn, maskOff, bitBuffer, numBits);
#endif
        if (*bitBuffer) { copyToClipboard(bitBuffer); }
        else { printf("Error!\n"); }
    }
}

// Разбирает и вычисляет выражение с флагами (8|1&4^2)
unsigned long long parseFlagExpression(const char* expr)
{
    unsigned long long result = 0;
    char buffer[256];
    strcpy(buffer, expr);

    char* token = strtok(buffer, "|&^");
    unsigned long long values[64];
    char operators[64];
    int count = 0;

    while (token)
    {
        int isNegated = (token[0] == '~');
        char* numPart = isNegated ? token + 1 : token;
        unsigned long long value = strtoull(numPart, NULL, 0);
        if (isNegated)
            value = ~value;
        values[count++] = value;
        token = strtok(NULL, "|&^");
    }

    // Восстанавливаем операторы
    int opIndex = 0;
    for (int i = 0; expr[i]; i++)
    {
        if (expr[i] == '|' || expr[i] == '&' || expr[i] == '^')
            operators[opIndex++] = expr[i];
    }

    // Вычисляем результат по порядку операций
    result = values[0];
    for (int i = 0; i < count - 1; i++)
    {
        if (operators[i] == '|')
            result |= values[i + 1];
        else if (operators[i] == '&')
            result &= values[i + 1];
        else if (operators[i] == '^')
            result ^= values[i + 1];
    }

    return result;
}

void Mode2(bool reverse)
{
    char input[256];
    char resultBuffer[256];
    unsigned long long value;

    while (1)
    {
        if (reverse) { printf("Enter flags expression (e.g. 8|1&4^2) (or 'exit'): "); }
        //else { printf("Enter DEC or HEX(0x) value [FLAGS MODE] (or 'exit'): "); }
        else {
            printf("Enter DEC or HEX(0x) value ");
            CW_M();
            printf("[FLAGS MODE]");
            CW_W();
            printf(" (or 'exit'): ");
        }

        scanf("%255s", input);
        if (!strcmp(input, "exit")) { return; }

        if (reverse)
        { // |&^~ OR AND XOR NOT
            unsigned long long result = parseFlagExpression(input);
            sprintf(resultBuffer, "0x%llX (%llu)", result, result);
            printf("Numeric result: %s\n", resultBuffer);
            copyToClipboard(resultBuffer);
        }
        else
        {
            // Опционально обрабатываем знак отрицания
            int isNegated = (input[0] == '~');
            char* hexPart = isNegated ? input + 1 : input;
            int base = 10;

            // Убираем префикс "0x" или "0X" (а также "x" или "X")
            if (strstr(hexPart, "0x") == hexPart || strstr(hexPart, "0X") == hexPart)
            {
                hexPart += 2;
                base = 16;
            }
            else if (*hexPart == 'x' || *hexPart == 'X')
            {
                ++hexPart;
                base = 16;
            }

            // Преобразуем строку в число
            value = strtoull(hexPart, NULL, base);

            // Если был указан знак отрицания, инвертируем число
            if (isNegated) { value = ~value; }

            // Построение строки с флагами: ищем установленные биты в числе.
            // Проходим от старшего бита (63-й) к младшему, чтобы выводить большие значения первыми.
            resultBuffer[0] = '\0';
            char hexBuffer[256] = "";
            char decBuffer[256] = "";
            bool first = true;
            for (int i = 63; i >= 0; i--) {
                unsigned long long bitVal = 1ULL << i;
                if (value & bitVal) {
                    char hexTmp[32];
                    char decTmp[32];
                    sprintf(hexTmp, "0x%llX", bitVal);
                    sprintf(decTmp, "%llu", bitVal);
                    if (!first) { strcat(hexBuffer, "|"); strcat(decBuffer, "|"); }
                    strcat(hexBuffer, hexTmp);
                    strcat(decBuffer, decTmp);
                    first = false;
                }
            }
            // Если ни один бит не установлен, выводим 0.
            if (first) { strcpy(hexBuffer, "0"); strcpy(decBuffer, "0"); }

            sprintf(resultBuffer, "%s   %s", hexBuffer, decBuffer);
            printf("Flags [copied<--] (%llu, 0x%llX): %s\n", value, value, resultBuffer);
            copyToClipboard(resultBuffer);
        }
    }
}

int main()
{
    // mode0 // ptr conv
    bool shift = (GetAsyncKeyState(VK_SHIFT) & 0x8000); // bits table
    bool lalt = (GetAsyncKeyState(VK_RMENU) & 0x8000); // ALT dec/dex flags extractor
    bool ctrl = (GetAsyncKeyState(VK_CONTROL) & 0x8000); // ALT dec/dex flags extractor
    bool prev = (GetAsyncKeyState(VK_OEM_COMMA) & 0x8000);
    bool next = (GetAsyncKeyState(VK_OEM_PERIOD) & 0x8000);
    if (ctrl) { Mode2(shift); }
    else if (shift) { Mode1(); }
    else { Mode0(); }
    return 0;
}

