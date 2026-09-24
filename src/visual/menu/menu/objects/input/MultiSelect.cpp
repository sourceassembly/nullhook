#include <menu/object/input/MultiSelect.hpp>
#include <menu/Message.hpp>
#include <menu/menu/special/SettingsManagerList.hpp>
#include <settings/Manager.hpp>

namespace zerokernel_multiselect
{
static settings::RVariable<rgba_t> color_border{ "zk.style.input.multiselect.border", "446498ff" };
static settings::RVariable<int> default_width{ "zk.style.input.multiselect.width", "84" };
static settings::RVariable<int> default_height{ "zk.style.input.multiselect.height", "14" };
}

bool zerokernel::MultiSelect::onLeftMouseClick()
{
    openModal();
    BaseMenuObject::onLeftMouseClick();
    return true;
}

void zerokernel::MultiSelect::render()
{
    renderBorder(*zerokernel_multiselect::color_border);
    std::string shown;
    int on = 0;
    for (auto &p : options)
    {
        if (!p.variable || p.variable->toString() != "true")
            continue;
        if (on)
            shown += ", ";
        shown += p.name;
        ++on;
    }
    if (!on)
        shown = "None";
    else
    {
        float x, y;
        resource::font::base.stringSize(shown, &x, &y);
        if (x + 5 > bb.getBorderBox().width)
            shown = std::to_string(on) + " selected";
    }
    text.set(shown);
    text.render();
    BaseMenuObject::render();
}

zerokernel::MultiSelect::MultiSelect() : BaseMenuObject{}
{
    resize(*zerokernel_multiselect::default_width, *zerokernel_multiselect::default_height);
    text.setParent(this);
    text.bb.width.setFill();
    text.bb.height.setFill();
    text.bb.setPadding(0, 0, 5, 0);
}

void zerokernel::MultiSelect::openModal()
{
    auto object = std::make_unique<ModalSelect>();
    object->stay_open = true;
    object->setParent(Menu::instance->wm.get());
    object->move(bb.getBorderBox().x, bb.getBorderBox().y);
    object->addMessageHandler(*this);
    int size = bb.getBorderBox().width;
    for (auto &p : options)
    {
        object->addOption(p.name, p.name, p.tooltip, p.variable);
        float x, y;
        resource::font::base.stringSize(p.name, &x, &y);
        x += 28;
        if (x > size)
            size = x;
    }
    object->resize(size, -1);
    Menu::instance->addModalObject(std::move(object));
}

void zerokernel::MultiSelect::handleMessage(zerokernel::Message &msg, bool is_relayed)
{
    if (!is_relayed && msg.name == "OptionSelected")
    {
        const std::string name = (std::string) msg.kv["value"];
        for (auto &p : options)
        {
            if (p.name != name || !p.variable)
                continue;
            auto *b = dynamic_cast<settings::Variable<bool> *>(p.variable);
            if (b)
                b->flip();
            break;
        }
    }
    BaseMenuObject::handleMessage(msg, is_relayed);
}

void zerokernel::MultiSelect::loadFromXml(const tinyxml2::XMLElement *data)
{
    BaseMenuObject::loadFromXml(data);

    auto child = data->FirstChildElement(nullptr);
    while (child != nullptr)
    {
        if (!strcmp("Option", child->Name()))
        {
            const char *name    = nullptr;
            const char *target  = nullptr;
            const char *tooltip = nullptr;
            if (child->QueryStringAttribute("name", &name) || child->QueryStringAttribute("target", &target))
            {
                child = child->NextSiblingElement(nullptr);
                continue;
            }
            auto *var = settings::Manager::instance().lookup(target);
            if (!var || var->getType() != settings::VariableType::BOOL)
            {
                printf("WARNING: MultiSelect option '%s': missing bool setting '%s'\n", name, target);
                child = child->NextSiblingElement(nullptr);
                continue;
            }
            zerokernel::special::SettingsManagerList::markVariable(target);
            auto has_tooltip = !(child->QueryStringAttribute("tooltip", &tooltip));
            options.push_back(option{ name, var, has_tooltip ? std::optional<std::string>(tooltip) : std::nullopt });
        }
        child = child->NextSiblingElement(nullptr);
    }
}

void zerokernel::MultiSelect::onMove()
{
    BaseMenuObject::onMove();
    text.onParentMove();
}
