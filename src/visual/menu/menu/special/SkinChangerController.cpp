/*
 * SkinChangerController - populates and drives the skinchanger menu tab.
 */

#include <menu/special/SkinChangerController.hpp>
#include <menu/object/Text.hpp>
#include <menu/object/input/Select.hpp>
#include <menu/object/input/Checkbox.hpp>
#include <menu/object/input/Slider.hpp>
#include <menu/object/container/LabeledObject.hpp>
#include <menu/object/container/Container.hpp>
#include <hacks/SkinChanger.hpp>
#include <common.hpp>

namespace zerokernel::special
{

namespace skinchanger = hacks::tf2::skinchanger;

static const std::pair<int, const char *> weapon_choices[] = {
    { 0, "Held weapon (auto)" },        { -1, "All weapons" },
    { 200, "Scout - Scattergun" },      { 209, "Scout/Engi - Pistol" },
    { 190, "Scout - Bat" },             { 205, "Soldier - Rocket Launcher" },
    { 199, "Shotgun (multi-class)" },   { 196, "Soldier - Shovel" },
    { 208, "Pyro - Flame Thrower" },    { 192, "Pyro - Fire Axe" },
    { 206, "Demo - Grenade Launcher" }, { 207, "Demo - Stickybomb Launcher" },
    { 191, "Demo - Bottle" },           { 202, "Heavy - Minigun" },
    { 197, "Engi - Wrench" },           { 211, "Medic - Medi Gun" },
    { 198, "Medic - Bonesaw" },         { 201, "Sniper - Sniper Rifle" },
    { 203, "Sniper - SMG" },            { 193, "Sniper - Kukri" },
    { 210, "Spy - Revolver" },          { 194, "Spy - Knife" },
};

static const std::pair<int, const char *> quality_choices[] = {
    { -1, "Auto" },       { 0, "Normal" },  { 1, "Genuine" },   { 3, "Vintage" },
    { 5, "Unusual" },     { 6, "Unique" },  { 11, "Strange" },  { 13, "Haunted" },
    { 14, "Collector's" }, { 15, "Decorated" },
};

static const std::pair<int, const char *> killstreak_choices[] = {
    { 0, "None" }, { 1, "Basic" }, { 2, "Specialized" }, { 3, "Professional" },
};

static const std::pair<int, const char *> sheen_choices[] = {
    { 0, "Off" },              { 1, "Team shine" },       { 2, "Deadly daffodil" },
    { 3, "Manndarin" },        { 4, "Mean green" },       { 5, "Agonizing emerald" },
    { 6, "Villainous violet" }, { 7, "Hot rod" },
};

static const std::pair<int, const char *> unusual_choices[] = {
    { 0, "None" }, { 1, "Hot" }, { 2, "Isotope" }, { 3, "Cool" }, { 4, "Energy Orb" },
};

SkinChangerController::SkinChangerController(Container &list) : list(list)
{
    // Populate cached strings so Selects render before the first sync.
    syncing    = true;
    weapon     = 0;
    kit        = 0;
    wear       = 0.0f;
    seed       = 0;
    quality    = -1;
    festive    = false;
    australium = false;
    killstreak = 0;
    sheen      = 0;
    unusual    = 0;
    syncing    = false;

    installCallbacks();

    auto text = [this](const std::string &str)
    {
        auto t = std::make_unique<Text>();
        t->set(str);
        auto *ptr = t.get();
        this->list.addObject(std::move(t));
        return ptr;
    };
    auto labeled = [this](const char *title, std::unique_ptr<BaseMenuObject> obj)
    {
        auto lo = std::make_unique<LabeledObject>();
        lo->setObject(std::move(obj));
        lo->setLabel(title);
        lo->bb.width.setFill();
        this->list.addObject(std::move(lo));
    };
    auto select = [](settings::IVariable &var, const std::pair<int, const char *> *opts, size_t count)
    {
        auto sel = std::make_unique<Select>(var);
        for (size_t i = 0; i < count; ++i)
            sel->options.push_back(Select::option{ opts[i].second, std::to_string(opts[i].first), std::nullopt });
        sel->resize(170, -1);
        return sel;
    };

    status = text("Editing: -");
    labeled("Weapon", select(weapon, weapon_choices, std::size(weapon_choices)));

    auto kits = select(kit, nullptr, 0);
    kit_select = kits.get();
    labeled("Paint kit", std::move(kits));

    auto wear_slider = std::make_unique<Slider<float>>(wear);
    wear_slider->min = 0.0f;
    wear_slider->max = 1.0f;
    wear_slider->resize(170, -1);
    labeled("Wear", std::move(wear_slider));

    auto seed_slider  = std::make_unique<Slider<int>>(seed);
    seed_slider->min  = 0;
    seed_slider->max  = 100;
    seed_slider->resize(170, -1);
    labeled("Seed", std::move(seed_slider));

    labeled("Quality", select(quality, quality_choices, std::size(quality_choices)));
    labeled("Killstreak", select(killstreak, killstreak_choices, std::size(killstreak_choices)));
    labeled("Sheen", select(sheen, sheen_choices, std::size(sheen_choices)));
    labeled("Unusual effect", select(unusual, unusual_choices, std::size(unusual_choices)));
    labeled("Festive", std::make_unique<Checkbox>(festive));
    labeled("Australium", std::make_unique<Checkbox>(australium));

    text("Right-click a slider to type a value.");
    text("Presets: skinchanger_save/load <name>");

    list.onMove();
    list.recursiveSizeUpdate();
    list.reorder_needed = true;
}

int SkinChangerController::editingKey()
{
    const int target = *weapon;
    if (target == 0)
    {
        const int held = skinchanger::ActiveSkinKey();
        return held >= 0 ? held : skinchanger::defaults_key;
    }
    return target;
}

skinchanger::SkinConfig SkinChangerController::varsToSkin()
{
    skinchanger::SkinConfig s;
    s.paintkit   = *kit;
    s.wear       = *wear;
    s.seed       = *seed;
    s.quality    = *quality;
    s.festive    = *festive;
    s.australium = *australium;
    s.killstreak = *killstreak;
    s.sheen      = *sheen;
    s.unusual    = *unusual;
    return s;
}

void SkinChangerController::commitSkin(skinchanger::SkinConfig skin)
{
    skinchanger::SetSkinConfig(editingKey(), skin);
}

void SkinChangerController::rebuildKitList(int key)
{
    if (!kit_select)
        return;
    kit_select->options.clear();
    std::vector<const char *> names;
    std::vector<int> ids;
    skinchanger::get_kits(key, names, ids);
    for (size_t i = 0; i < ids.size(); ++i)
        kit_select->options.push_back(Select::option{ names[i], std::to_string(ids[i]), std::nullopt });
}

void SkinChangerController::syncVars(int key)
{
    syncing              = true;
    const auto s         = skinchanger::GetSkinConfig(key);
    kit        = s.paintkit;
    wear       = s.wear;
    seed       = s.seed;
    quality    = s.quality;
    festive    = s.festive;
    australium = s.australium;
    killstreak = s.killstreak;
    sheen      = s.sheen;
    unusual    = s.unusual;
    syncing    = false;

    if (status)
        status->set(key == skinchanger::defaults_key ? "Editing: All weapons (defaults)"
                                                     : format("Editing: ", skinchanger::weapon_label(key), "  #", key));
}

void SkinChangerController::update()
{
    const int key = editingKey();
    bool dirty    = skinchanger::ConsumeMenuDirty() || sync_needed;
    sync_needed   = false;
    if (key != last_key)
    {
        rebuildKitList(key);
        dirty = true;
    }
    if (dirty)
    {
        last_key = key;
        syncVars(key);
    }
}

void SkinChangerController::installCallbacks()
{
    weapon.installChangeCallback(
        [this](settings::VariableBase<int> &, int)
        {
            if (!syncing)
                sync_needed = true;
        });
    kit.installChangeCallback(
        [this](settings::VariableBase<int> &, int after)
        {
            if (syncing)
                return;
            auto s     = varsToSkin();
            s.paintkit = after;
            commitSkin(s);
        });
    wear.installChangeCallback(
        [this](settings::VariableBase<float> &, float after)
        {
            if (syncing)
                return;
            auto s = varsToSkin();
            s.wear = after;
            commitSkin(s);
        });
    seed.installChangeCallback(
        [this](settings::VariableBase<int> &, int after)
        {
            if (syncing)
                return;
            auto s = varsToSkin();
            s.seed = after;
            commitSkin(s);
        });
    quality.installChangeCallback(
        [this](settings::VariableBase<int> &, int after)
        {
            if (syncing)
                return;
            auto s    = varsToSkin();
            s.quality = after;
            commitSkin(s);
        });
    festive.installChangeCallback(
        [this](settings::VariableBase<bool> &, bool after)
        {
            if (syncing)
                return;
            auto s    = varsToSkin();
            s.festive = after;
            commitSkin(s);
        });
    australium.installChangeCallback(
        [this](settings::VariableBase<bool> &, bool after)
        {
            if (syncing)
                return;
            auto s       = varsToSkin();
            s.australium = after;
            commitSkin(s);
        });
    killstreak.installChangeCallback(
        [this](settings::VariableBase<int> &, int after)
        {
            if (syncing)
                return;
            auto s       = varsToSkin();
            s.killstreak = after;
            commitSkin(s);
        });
    sheen.installChangeCallback(
        [this](settings::VariableBase<int> &, int after)
        {
            if (syncing)
                return;
            auto s  = varsToSkin();
            s.sheen = after;
            commitSkin(s);
        });
    unusual.installChangeCallback(
        [this](settings::VariableBase<int> &, int after)
        {
            if (syncing)
                return;
            auto s    = varsToSkin();
            s.unusual = after;
            commitSkin(s);
        });
}
} // namespace zerokernel::special
