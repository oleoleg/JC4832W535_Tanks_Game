#pragma once
#include <Arduino.h>

bool sdInit();
bool sdIsReady();

// Список файлов с размером и датой (HTML)
String sdListFiles();

// Информация о свободном месте (HTML)
String sdFreeSpace();

// Удалить файл по имени
bool sdDeleteFile(const String &filename);