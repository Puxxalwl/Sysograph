// Sysograph — a free, original Linux system task manager.
// Copyright (C) 2025 The Sysograph Project Contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "core/i18n.h"

#include <cstdlib>
#include <cstring>
#include <string>
#include <unordered_map>

namespace sysograph {
namespace i18n {
namespace {

std::string g_lang = "en";

// Russian translations for the UI strings. English is the source
// language, so untranslated keys fall back to the key text.
const std::unordered_map<std::string, std::string>& russian() {
  static const std::unordered_map<std::string, std::string> ru = {
      {"Processes", "Процессы"},
      {"Process tree", "Дерево процессов"},
      {"Performance", "Производительность"},
      {"Users", "Пользователи"},
      {"Startup", "Автозапуск"},
      {"Services", "Службы"},
      {"Search", "Поиск"},
      {"Filter processes by name, user or command", "Фильтр процессов по имени, пользователю или команде"},
      {"Name", "Имя"},
      {"PID", "PID"},
      {"User", "Пользователь"},
      {"CPU", "Процессор"},
      {"Memory", "Память"},
      {"State", "Состояние"},
      {"Parent PID", "Родительский PID"},
      {"Command line", "Командная строка"},
      {"Threads", "Потоки"},
      {"Priority", "Приоритет"},
      {"End process", "Завершить процесс"},
      {"Force kill", "Принудительно завершить"},
      {"End the selected process? It will be asked to terminate (SIGTERM) and unsaved work may be lost.",
       "Завершить выбранный процесс? Ему будет отправлен сигнал завершения (SIGTERM), несохранённые данные могут быть потеряны."},
      {"Force kill the selected process? It will be terminated immediately (SIGKILL) and unsaved work will be lost.",
       "Принудительно завершить выбранный процесс? Он будет остановлен немедленно (SIGKILL), несохранённые данные будут потеряны."},
      {"Cancel", "Отмена"},
      {"No process selected", "Процесс не выбран"},
      {"Select a process to see details", "Выберите процесс, чтобы увидеть подробности"},
      {"Executable", "Исполняемый файл"},
      {"Working directory", "Рабочий каталог"},
      {"Open files", "Открытые файлы"},
      {"Started", "Запущен"},
      {"CPU time", "Время ЦП"},
      {"Available", "Доступно"},
      {"Not available", "Недоступно"},
      {"running", "выполняется"},
      {"sleeping", "ожидает"},
      {"disk-wait", "ждёт ввода-вывода"},
      {"zombie", "завершён (зомби)"},
      {"stopped", "остановлен"},
      {"tracing-stop", "остановлен трассировкой"},
      {"dead", "завершён"},
      {"waking", "пробуждается"},
      {"parked", "припаркован"},
      {"idle", "простаивает"},
      {"unknown", "неизвестно"},
      {"Cores", "Ядра"},
      {"Processes", "Процессы"},  // duplicate-safe (map overwrite)
      {"Running", "Активные"},
      {"Blocked", "Заблокированные"},
      {"Uptime", "Время работы"},
      {"Used", "Использовано"},
      {"Swap", "Подкачка"},
      {"Buffers", "Буферы"},
      {"Cached", "Кэш"},
      {"Disk", "Диск"},
      {"Network", "Сеть"},
      {"Receiving", "Приём"},
      {"Sending", "Отправка"},
      {"Read", "Чтение"},
      {"Write", "Запись"},
      {"Processes", "Процессы"},
      {"Startup applications", "Приложения автозапуска"},
      {"Enable", "Включить"},
      {"Disable", "Отключить"},
      {"Refresh", "Обновить"},
      {"Entry", "Элемент"},
      {"Command", "Команда"},
      {"Origin", "Источник"},
      {"Status", "Статус"},
      {"enabled", "включён"},
      {"disabled", "отключён"},
      {"user", "пользовательский"},
      {"system+override", "системный (переопределяется)"},
      {"system", "системный"},
      {"The list is read-only because no writable autostart directory exists.",
       "Список доступен только для чтения: нет доступного для записи каталога автозапуска."},
      {"Unit", "Юнит"},
      {"Load", "Загрузка"},
      {"Active", "Активность"},
      {"Sub", "Состояние юнита"},
      {"Description", "Описание"},
      {"The services list is provided by systemd. Changes to services are not offered here; use your distribution's service manager.",
       "Список служб предоставляется systemd. Изменение служб здесь не предусмотрено; используйте менеджер служб вашего дистрибутива."},
      {"systemd is not available on this system.", "systemd недоступен в этой системе."},
      {"Failed to run systemctl.", "Не удалось запустить systemctl."},
      {"Loading services…", "Загрузка служб…"},
      {"Update interval", "Интервал обновления"},
      {"1 second", "1 секунда"},
      {"2 seconds", "2 секунды"},
      {"5 seconds", "5 секунд"},
      {"About Sysograph", "О программе Sysograph"},
      {"A free system task manager for Linux", "Свободный диспетчер задач для Linux"},
      {"Process manager", "Диспетчер процессов"},
      {"Process", "Процесс"},
      {"Total memory", "Всего памяти"},
      {"Show system processes", "Показывать системные процессы"},
      {"Virtual memory", "Виртуальная память"},
      {"OK", "ОК"},
      {"Ending process failed", "Не удалось завершить процесс"},
      {"Operations", "Операции"},
      {"Total received", "Всего принято"},
      {"Total sent", "Всего отправлено"},
  };
  return ru;
}

}  // namespace

void init() {
  const char* forced = std::getenv("SYSSOGRAPH_LANG");
  const char* names[] = {"LC_ALL", "LC_MESSAGES", "LANG"};
  std::string lang = "en";
  if (forced && *forced) {
    lang = forced;
  } else {
    for (const char* var : names) {
      const char* v = std::getenv(var);
      if (v && *v) {
        lang = v;
        break;
      }
    }
  }
  // Accept ru_RU.UTF-8, ru, RU, "ru_UA", etc.
  if (lang.size() >= 2 && (lang[0] == 'r' || lang[0] == 'R') &&
      (lang[1] == 'u' || lang[1] == 'U')) {
    g_lang = "ru";
  } else {
    g_lang = "en";
  }
}

const char* tr(const char* key) {
  if (g_lang == "ru") {
    const auto& ru = russian();
    auto it = ru.find(key);
    if (it != ru.end()) return it->second.c_str();
  }
  return key;
}

const char* activeLanguage() { return g_lang.c_str(); }

}  // namespace i18n
}  // namespace sysograph
