#pragma once

#include <limits>
#include <settings/Settings.hpp>

namespace hacks::tf2::skinchanger
{
struct SkinConfig;
}

namespace zerokernel
{
class Container;
class Text;
class Select;
} // namespace zerokernel

namespace zerokernel::special
{

class SkinChangerController
{
public:
    explicit SkinChangerController(Container &list);

    void update();

private:
    int editingKey();
    hacks::tf2::skinchanger::SkinConfig varsToSkin();
    void commitSkin(hacks::tf2::skinchanger::SkinConfig skin);
    void rebuildKitList(int key);
    void syncVars(int key);
    void installCallbacks();

    Container &list;

    settings::Variable<int> weapon{};
    settings::Variable<int> kit{};
    settings::Variable<float> wear{};
    settings::Variable<int> seed{};
    settings::Variable<int> quality{};
    settings::Variable<bool> festive{};
    settings::Variable<bool> australium{};
    settings::Variable<int> killstreak{};
    settings::Variable<int> sheen{};
    settings::Variable<int> unusual{};

    Text *status{ nullptr };
    Select *kit_select{ nullptr };
    int last_key{ std::numeric_limits<int>::min() };
    bool syncing{ false };
    bool sync_needed{ true };
};
} // namespace zerokernel::special
