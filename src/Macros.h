#pragma once

// yes im going this far to keep this mod at sdk 5.0.0 (check favorite mods lol)
#define IS_GEODE_THEME(var)                                          \
    auto geode = geode::Loader::get()->getLoadedMod("geode.loader"); \
    var = geode::Loader::get()->getVersion().getMinor() >= 6 ? geode->getSettingValue<std::string>("used-theme") != "Geometry Dash" : geode->getSettingValue<bool>("enable-geode-theme")