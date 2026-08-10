#ifndef RETRONODE_LABEL_H
#define RETRONODE_LABEL_H

#include "control.h"
#include <string>
#include <SDL3/SDL.h>

namespace RetroNode {

class RN_API Label : public Control {
    RN_CLASS(Label, Control)

public:
    std::string text;
    SDL_Color text_color = { 255, 255, 255, 255 };
    uint32_t font_texture_id = 0;

    Label();
    virtual ~Label() = default;

    virtual void get_property_list(std::vector<PropertyInfo>& out_list) const override;
    virtual Variant get(const StringName& p_name) const override;
    virtual bool set(const StringName& p_name, const Variant& p_value) override;

    void set_text(const std::string& p_text) { text = p_text; }
    const std::string& get_text() const { return text; }

    virtual void _ready() override;
    virtual void _process(float delta) override;
};

} // namespace RetroNode

#endif // RETRONODE_LABEL_H
