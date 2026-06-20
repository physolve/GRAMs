# Skill: Сборка Qt проекта

Этот skill описывает как собирать проект с Qt 6.11.

Перед сборкой всегда проверяй:
1. Что CMAKE_PREFIX_PATH указывает на Qt 6.11.x, не более старую версию
2. Что модуль Qt6::TaskTree доступен (find_package result)
3. Что нет конфликтующих ANTHROPIC_API_KEY в env (это не нужно для сборки)

При ошибках CMake:
- "Could not find Qt6::TaskTree" → Qt установлен без компонента TaskTree, нужно переустановить с галочкой TaskTree
- "No module named TaskTree" → используется Qt < 6.11

После успешной сборки — запусти приложение и сообщи об успехе.
