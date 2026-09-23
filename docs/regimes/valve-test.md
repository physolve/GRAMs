# Режим: Тест клапанов (`ValveTestWorker`)

> Статус: **Реализован (код)**
> Доказательство: `src/actions/workers/ValveTestWorker.cpp` — полная state machine, все методы рабочие. С Day7 biodaq работает на Windows — доступна верификация на реальном стенде (в процессе).

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

- **Security закрыл клапан шага** (с 23.09.2026, основной путь для S4/R4). Security закрывает S4/R4 сам на такте `softEvent` — по давлению выше 1,8 бар, по прогнозу равновесия через К15x, — и `ValveControl` сообщает об этом сигналом `securityClosed`. Если клапан открыт этим режимом (`ValveSource::Regime`) и входит в текущий шаг, воркер пишет **одно** событие `kSecurityViolation` (`step N: …`) и вызывает `abortCurrentStep`: клапаны шага закрываются, повтор помечается ошибкой, **следующий шаг не открывается** (D2). Закрытие посреди команды открытия откладывается до её конца (`m_commanding`). Закрытие по концу режима (`origin == regime_end`) нарушением не считается: задача в этот момент уже отменена. Источник: `ValveTestWorker.cpp:onSecurityClosed:112`, `abortCurrentStep:163`.
- **Security violation на такте воркера** (`checkSecurity()` → false; `ValveTestWorker.cpp:checkSecurity:323`): тот же путь `abortCurrentStep`. Для S4/R4 срабатывает редко — Security закрывает их раньше, на такте `softEvent`; остаётся для AR4 (порог сброса).
- **Недостоверное давление** у открытого клапана: одно событие `kWarning` за эпизод (`onSecurityWarning:102`), клапан не закрывается (D6).
- **«Стоп» во время выдержки**: клапан шага закрывается — S4/R4 Security закрывает по концу режима (`unauthorized_open`, `regime_end`; `RegimeTaskTree::setValveControl:41` → `ValveControl::setRegimeActive`). До 23.09.2026 S4 оставался открытым: у воркера нет обработки отмены.
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
- `src/actions/workers/ValveTestWorker.cpp:startRepeat:45` — инициализация повтора, state machine запущена
- `src/actions/workers/ValveTestWorker.cpp:openCurrentStepValves:87` — реальный вызов `setValveFromAction`
- `src/actions/workers/ValveTestWorker.cpp:onSecurityClosed:112` — реакция на закрытие Security (Т3)
- `src/actions/workers/ValveTestWorker.cpp:checkSecurity:323` — проверка через `Security::checkPressure`
- `src/actions/workers/ValveTestWorker.h:State:93` — все состояния state machine определены
- Тесты: `SimValves.ValveTest*`, `SimSecurityRange.ValveTestStepClosedBySecurityOnce`, `SimSecurityRange.StoppedValveTestLeavesRangeValveClosed` (через настоящий `RegimeTaskTree`)

Что осталось сделать:
- [ ] Верифицировать на железе (реальный стенд под Windows / Linux + biodaq) → перевести в `Проверено на железе`

## 9. Журнал изменений

| Дата | Что изменилось | Новый статус |
|---|---|---|
| 2026-06-22 | Создан документ (dry run скилла regime-docs) | Реализован (код) |
| 2026-09-17 | D1: закрытый клапан больше не «нарушение»; D2: авария шага не открывает следующий шаг (`abortCurrentStep`) | Реализован (код) — без изменений |
| 2026-09-23 | Security сам закрывает S4/R4 (Т1/Т2): воркер получает `securityClosed` и прерывает шаг одним `security_violation`; «Стоп» во время выдержки больше не оставляет S4 открытым; предупреждения о недостоверном давлении — `kWarning` с такта Security | Реализован (код) — без изменений |
