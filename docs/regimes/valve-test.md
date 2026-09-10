# Режим: Тест клапанов (`ValveTestWorker`)

> Статус: **Реализован (код)**
> Доказательство: `src/actions/ValveTestWorker.cpp` — полная state machine (318 строк), все методы рабочие. С Day7 biodaq работает на Windows — доступна верификация на реальном стенде (в процессе).

---

## 1. Назначение

Последовательно открывает и закрывает заданные клапаны по конфигурируемому сценарию с контролем паузы, выдержки и безопасности. Используется для верификации работоспособности клапанного оборудования перед экспериментом.

## 2. Клапаны

Конфигурируется из QML через `RegimeTaskTree.setValveTestSteps()`. Шаги задаются как `QList<ValveStepConfig>`:

| Действие | Клапан(ы) | Момент |
|---|---|---|
| Открыть | `ValveStepConfig::valveNames` | Конец `StepPauseBefore` |
| Закрыть | `ValveStepConfig::valveNames` | Конец `StepDwelling` |

Последовательность шагов: `GlobalPauseBefore → (StepPauseBefore → open → StepDwelling → close → StepPauseAfter) × N шагов → GlobalPauseAfter`.

## 3. Условие завершения

Режим завершается после прохождения всех шагов (`m_currentStep >= m_cfg.steps.size()`) всех повторов (`m_currentRepeat >= m_cfg.totalRepeats`). Таймаут на шаг: `dwellSec` из `ValveStepConfig`. Общего таймаута нет — только явный конец сценария.

## 4. Поведение при ошибке и срабатывании security

- **Security violation** (`checkSecurity()` → false): закрываются все открытые клапаны текущего шага (`closeValves`), повтор помечается ошибкой (`markRepeatAsError`), логируется `kSecurityViolation`. Источник: `ValveTestWorker.cpp:checkSecurity()`.
- **Valve open failure** (`setValveFromAction` вернул false): логируется `kValveBlocked`, выполнение шага продолжается.
- **Pause**: таймер останавливается, `RegimeManager` переводится в `Paused`.

## 5. Использование NodePressure / QuartileManager

Не использует NodePressure / QuartileManager напрямую. Работает только через `ValveControl::setValveFromAction` и `Security::checkSecurity`.

## 6. Физический смысл

Вспомогательный режим, не часть алгоритма Сивертса. Проверяет механическую исправность клапанов и корректность конфигурации `ControlDO` перед проведением эксперимента.

## 7. Параметры из профиля / настроек

| Параметр | Источник | Назначение |
|---|---|---|
| `ValveStepConfig::valveNames` | QML → `setValveTestSteps()` | Имена клапанов шага |
| `ValveStepConfig::pauseBeforeSec` | QML | Пауза перед открытием |
| `ValveStepConfig::dwellSec` | QML | Выдержка в открытом состоянии |
| `ValveStepConfig::pauseAfterSec` | QML | Пауза после закрытия |
| `totalRepeats` | QML | Количество повторов сценария |
| `globalPauseBefore/AfterSec` | QML | Глобальная пауза до/после всех шагов |

## 8. Текущий статус

**Статус: Реализован (код)**

Доказательства:
- `src/actions/ValveTestWorker.cpp:startRepeat` (строки ~55–75) — инициализация повтора, state machine запущена
- `src/actions/ValveTestWorker.cpp:openCurrentStepValves` — реальный вызов `setValveFromAction`
- `src/actions/ValveTestWorker.cpp:checkSecurity` — проверка через `Security::checkValvePressure`
- `src/actions/ValveTestWorker.h:State` (строки 87–94) — все состояния state machine определены

Что осталось сделать:
- [ ] Верифицировать на железе (реальный стенд под Windows / Linux + biodaq) → перевести в `Проверено на железе`

## 9. Журнал изменений

| Дата | Что изменилось | Новый статус |
|---|---|---|
| 2026-06-22 | Создан документ (dry run скилла regime-docs) | Реализован (код) |
