#pragma once

#include <QVector>

// Порт дискретных выходов клапанов для ValveControl.
//
// Подтверждения положения у клапанов нет: «чтение» — это перечитывание
// регистра-защёлки выхода (AdvantechDO::refresh). Демо-реализация
// (sim::SimValveEcho) отдаёт последнюю записанную маску — ту же семантику.
class IDoPort
{
public:
    virtual ~IDoPort() = default;

    virtual bool write(const QVector<bool> &states) = 0;   // бит = клапан по valveMap
    virtual bool refresh() = 0;                            // false — чтение не удалось
    virtual QVector<bool> data() = 0;                      // последнее прочитанное
};
