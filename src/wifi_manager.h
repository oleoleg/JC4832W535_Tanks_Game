//объявления Wi-Fi и веб-сервера

#pragma once

// Подключиться к Wi-Fi (с веб-порталом при необходимости)
bool wifiConnect();

// Запустить веб-сервер для загрузки файлов
void wifiStartServer();

// Обработать Wi-Fi (вызывать в loop)
void wifiLoop();