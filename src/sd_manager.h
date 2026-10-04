#pragma once
#include <Arduino.h>

bool sdInit();
bool sdIsReady();

String sdFreeSpace();

// HTML-список содержимого папки (папки + файлы)
String sdListItems(const String& path);

// Создать папку (полный путь, начинается с "/")
bool sdCreateDir(const String& fullPath);

// Удалить файл или пустую папку
bool sdDeletePath(const String& fullPath);

// Проверка пути (нет "..", нет "\")
bool sdIsValidPath(const String& path);