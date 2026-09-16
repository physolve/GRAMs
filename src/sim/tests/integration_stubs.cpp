// Заглушки для сборки интеграционного теста без квартилей и PostgreSQL.
//
// ValveControl ссылается на ReactionQuartile (ручной клапан камеры R5) и на
// GramStateDB (запись снимка — ветка if(false)). В сценарии «Вакуума» ни то,
// ни другое не вызывается; тянуть ради них квартили, графики и QCustomPlot в
// тест незачем. Если вызов всё-таки случится — тест упадёт громко.

#include "addon/ReactionQuartile.h"
#include "db/GramStateDB.h"

#include <cstdlib>
#include <iostream>

namespace {
[[noreturn]] void unexpected(const char *what)
{
    std::cerr << "integration_stubs: неожиданный вызов " << what << std::endl;
    std::abort();
}
} // namespace

void ReactionQuartile::setChamberStatus(bool) { unexpected("ReactionQuartile::setChamberStatus"); }
void ReactionQuartile::updateChamberToQuartile() { unexpected("ReactionQuartile::updateChamberToQuartile"); }
bool GramStateDB::writeTimeStamp() { unexpected("GramStateDB::writeTimeStamp"); }
