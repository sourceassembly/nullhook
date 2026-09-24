#pragma once

#include <settings/Settings.hpp>
#include <menu/Menu.hpp>
#include <menu/object/container/ModalSelect.hpp>

namespace zerokernel
{

class MultiSelect : public BaseMenuObject
{
public:
    struct option
    {
        std::string name;
        settings::IVariable *variable{ nullptr };
        std::optional<std::string> tooltip;
    };

    ~MultiSelect() override = default;

    MultiSelect();

    void render() override;
    void onMove() override;
    void handleMessage(Message &msg, bool is_relayed) override;
    void loadFromXml(const tinyxml2::XMLElement *data) override;
    bool onLeftMouseClick() override;

    void openModal();

    Text text{};
    std::vector<option> options{};
};
}
