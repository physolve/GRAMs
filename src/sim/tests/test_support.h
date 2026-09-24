#pragma once

// Общие помощники тестов sim: каталог из настоящего профиля установки
// и загрузка JSON-фикстур.

#include "core/SimCatalog.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <gtest/gtest.h>

namespace simtest {

inline QJsonDocument readJson(const QString &path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        ADD_FAILURE() << "не открылся " << path.toStdString();
        return {};
    }
    QJsonParseError err;
    auto doc = QJsonDocument::fromJson(f.readAll(), &err);
    if (err.error != QJsonParseError::NoError)
        ADD_FAILURE() << path.toStdString() << ": " << err.errorString().toStdString();
    return doc;
}

inline QString profilePath(const QString &relative)
{
    return QStringLiteral(SIM_PROFILES_DIR) + u'/' + relative;
}

// Вход каталога — из profile/GRAMsPfp.json, раздел GRAM50 (как Initialize).
inline sim::CatalogInput gram50Input()
{
    sim::CatalogInput in;
    const QJsonObject stuff = readJson(QStringLiteral(GRAMS_PROFILE_JSON))
                                  .object().value("GRAM50").toObject().value("stuff").toObject();
    for (const auto &v : stuff.value("pressureSensors").toArray()) {
        const QJsonObject o = v.toObject();
        in.pressureCard.append({o.value("name").toString(), o.value("A").toDouble(),
                                o.value("B").toDouble(), o.value("R").toDouble()});
    }
    for (const auto &v : stuff.value("temperatureSensors").toArray())
        in.temperatureCard.append(v.toObject().value("name").toString());
    for (const auto &v : stuff.value("valveMap").toArray())
        in.valveCodes.append(v.toString());
    return in;
}

inline const sim::CatalogBuild &gram50()
{
    static const sim::CatalogBuild build = sim::buildCatalog(gram50Input());
    return build;
}

} // namespace simtest
