#pragma once

#include <borealis.hpp>

// A brls::Label drawn with the Rubik font (loaded in main.cpp as "rubik").
// Falls back to the default font if Rubik could not be loaded.
class RubikLabel : public brls::Label {
public:
    RubikLabel();
    static brls::View* create();
};
