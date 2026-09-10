# Skill: Сборка Qt проекта

Этот skill описывает как собирать проект с Qt 6.11 из любого терминала, включая `claude -p`.

## Критично: инициализация MSVC

`cl.exe` требует окружения MSVC. При запуске из обычного PowerShell или через `claude -p`
стандартные заголовки (errno.h, iostream и т.д.) недоступны — сборка падает.

**Всегда** используй паттерн `cmd /c`:

```powershell
$vcvars = "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
cmd /c "`"$vcvars`" && cmake --build build --parallel"
```

## Перед сборкой проверь

1. Папка `build/` существует и сконфигурирована (`CMakeCache.txt` есть)
2. `CMAKE_PREFIX_PATH` указывает на Qt 6.11.x (не старее)
3. `Qt6::TaskTree` доступен (Qt 6.11+)

## Полный цикл: конфигурация + сборка

```powershell
$vcvars = "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"

# Конфигурация (только если build/ пуст или нужно переконфигурировать)
cmd /c "`"$vcvars`" && cmake -B build -S . -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH=C:/Qt/6.11.1/msvc2022_64 -DEigen3_DIR=F:/testFilterLib/eigen/build"

# Сборка
cmd /c "`"$vcvars`" && cmake --build build --parallel"
```

## Диагностика ошибок

| Ошибка | Причина | Решение |
|---|---|---|
| `errno.h: No such file or directory` | MSVC-окружение не инициализировано | Использовать `cmd /c "vcvars64.bat && ..."` |
| `Could not find Qt6::TaskTree` | Qt без компонента TaskTree или Qt < 6.11 | Переустановить Qt 6.11+ с TaskTree |
| `cl.exe not found` | Ninja не видит компилятор | То же — нет vcvars |

После успешной сборки выведи `BUILD OK` и время сборки (из вывода ninja).
