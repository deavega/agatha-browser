#include "view/rubik_label.hpp"

RubikLabel::RubikLabel() {
    const int rubik = brls::Application::getFont("rubik");
    if (rubik != brls::FONT_INVALID) {
        this->font = rubik;
    }
}

brls::View* RubikLabel::create() { return new RubikLabel(); }
