#pragma once

#include <array>
#include <string>

#include <borealis.hpp>

#include "view/rubik_label.hpp"

class AgathaActivity : public brls::Activity {
public:
    static constexpr size_t kBookmarkCount = 5;

    CONTENT_FROM_XML_RES("activity/agatha.xml");

    void onContentAvailable() override;
    void onResume() override;

private:
    struct CardViews {
        brls::Box* card = nullptr;
        RubikLabel* letter = nullptr;
        RubikLabel* name = nullptr;
        RubikLabel* domain = nullptr;
    };

    void buildCards();
    void buildActions();
    void refreshCard(size_t index);

    void openUrl(const std::string& url);
    void openYouTubeHome();
    void openYouTubeVideo(const std::string& videoId);
    void promptUrl();
    void editBookmark(size_t index);
    void resetBookmark(size_t index);
    void setStatus(const std::string& text);

    std::array<CardViews, kBookmarkCount> cards_{};

    BRLS_BIND(brls::Box, cardsBox, "agatha/cards");
    BRLS_BIND(brls::Box, actionsBox, "agatha/actions");
    BRLS_BIND(brls::Label, statusLabel, "agatha/status");
};
