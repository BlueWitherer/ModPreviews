#pragma once

// bindings breaks forced me to update sdk. you win this time.
#define IS_GEODE_THEME(var)                                         \
    auto gode = geode::Loader::get()->getLoadedMod("geode.loader"); \
    var = gode->getSettingValue<std::string>("used-theme") != "Geometry Dash"