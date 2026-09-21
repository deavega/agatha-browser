// Agatha Browser home screen: bookmark cards (system web browser) + YouTube via NewPipe.
#include "activity/agatha_activity.hpp"

#include <sys/stat.h>

#include <cctype>
#include <fstream>
#include <vector>

#include "activity/main_activity.hpp"
#include "activity/stream_detail_activity.hpp"
#include "agatha/agatha.hpp"
#include "newpipe/log.hpp"
#include "newpipe/models.hpp"

#if defined(__SWITCH__)
#include <switch.h>
#endif

namespace {

// ---------- Look ----------
const NVGcolor kCard      = nvgRGB(27, 26, 42);
const NVGcolor kText      = nvgRGB(230, 228, 240);
const NVGcolor kSubtle    = nvgRGB(150, 146, 170);
const NVGcolor kBadge[AgathaActivity::kBookmarkCount] = {
    nvgRGB(124, 92, 255), nvgRGB(238, 92, 120), nvgRGB(230, 165, 40),
    nvgRGB(70, 180, 95),  nvgRGB(70, 125, 240),
};
constexpr float kCardW = 196, kCardH = 190, kCardGap = 18, kCardRadius = 22;
constexpr float kActW = 240, kActH = 58, kActGap = 18;

// ---------- Bookmarks: sdmc:/config/AgathaBrowser/bookmarks.txt, "name|url" per line ----------
struct Bookmark {
    std::string name;
    std::string url;
};

const Bookmark kDefaults[AgathaActivity::kBookmarkCount] = {
    {"detik.com", "https://m.detik.com"},
    {"Kompas", "https://www.kompas.com"},
    {"CNBC Indonesia", "https://www.cnbcindonesia.com"},
    {"Bisnis.com", "https://www.bisnis.com"},
    {"Kontan", "https://www.kontan.co.id"},
};

#if defined(__SWITCH__)
const char* kConfigDir  = "sdmc:/config/AgathaBrowser";
const char* kConfigFile = "sdmc:/config/AgathaBrowser/bookmarks.txt";
#else
const char* kConfigDir  = "agatha_config";
const char* kConfigFile = "agatha_config/bookmarks.txt";
#endif

Bookmark g_bookmarks[AgathaActivity::kBookmarkCount];

void load_bookmarks() {
    for (size_t i = 0; i < AgathaActivity::kBookmarkCount; i++) g_bookmarks[i] = kDefaults[i];
    std::ifstream in(kConfigFile);
    std::string line;
    size_t i = 0;
    while (i < AgathaActivity::kBookmarkCount && std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        const auto sep = line.find('|');
        if (sep == std::string::npos || sep == 0 || sep + 1 >= line.size()) continue;
        g_bookmarks[i++] = {line.substr(0, sep), line.substr(sep + 1)};
    }
}

bool save_bookmarks() {
#if defined(__SWITCH__)
    mkdir("sdmc:/config", 0777);
#endif
    mkdir(kConfigDir, 0777);
    std::ofstream out(kConfigFile, std::ios::trunc);
    if (!out) return false;
    for (const auto& b : g_bookmarks) out << b.name << '|' << b.url << '\n';
    return static_cast<bool>(out);
}

std::string sanitize(std::string s) {
    for (auto& c : s)
        if (c == '|' || c == '\n' || c == '\r') c = '/';
    return s;
}

std::string normalize_url(std::string url) {
    if (url.rfind("http://", 0) != 0 && url.rfind("https://", 0) != 0) url = "https://" + url;
    return url;
}

// "https://www.kompas.com/news" -> "kompas.com"
std::string domain_of(const std::string& url) {
    size_t start = url.find("://");
    start        = (start == std::string::npos) ? 0 : start + 3;
    if (url.compare(start, 4, "www.") == 0) start += 4;
    else if (url.compare(start, 2, "m.") == 0) start += 2;
    const size_t end = url.find_first_of("/?#:", start);
    return url.substr(start, end == std::string::npos ? std::string::npos : end - start);
}

// First character of the name (UTF-8 aware), uppercased when ASCII.
std::string initial_of(const std::string& name) {
    if (name.empty()) return "?";
    const auto c = static_cast<unsigned char>(name[0]);
    size_t len   = 1;
    if (c >= 0xF0) len = 4;
    else if (c >= 0xE0) len = 3;
    else if (c >= 0xC0) len = 2;
    std::string s = name.substr(0, len);
    if (len == 1) s[0] = static_cast<char>(std::toupper(c));
    return s;
}

// ---------- System web browser (Switch web applet) ----------
bool application_mode() {
#if defined(__SWITCH__)
    const AppletType t = appletGetAppletType();
    return t == AppletType_Application || t == AppletType_SystemApplication;
#else
    return false;
#endif
}

// Opens the page; returns true on success and fills lastUrl with where the user ended up.
bool show_web_page(const std::string& url, std::string& lastUrl, std::string& error) {
#if defined(__SWITCH__)
    WebCommonConfig cfg;
    WebCommonReply reply;
    Result rc = webPageCreate(&cfg, url.c_str());
    if (R_SUCCEEDED(rc)) {
        webConfigSetWhitelist(&cfg, "^http*");
        rc = webConfigShow(&cfg, &reply);
    }
    if (R_FAILED(rc)) {
        char buf[64];
        snprintf(buf, sizeof(buf), "Browser error: 0x%x", rc);
        error = buf;
        return false;
    }
    char last[1024] = {0};
    size_t len      = 0;
    if (R_SUCCEEDED(webReplyGetLastUrl(&reply, last, sizeof(last), &len))) lastUrl = last;
    return true;
#else
    (void)url;
    (void)lastUrl;
    error = "The web browser is only available on Switch.";
    return false;
#endif
}

// ---------- View builders ----------
RubikLabel* make_label(const std::string& text, float size, NVGcolor color, float width = 0) {
    auto* label = new RubikLabel();
    label->setText(text);
    label->setFontSize(size);
    label->setTextColor(color);
    label->setHorizontalAlign(brls::HorizontalAlign::CENTER);
    if (width > 0) {
        label->setWidth(width);
        label->setSingleLine(true);  // long names end with "..."
    }
    return label;
}

brls::Box* make_tile(float w, float h, float radius) {
    auto* box = new brls::Box(brls::Axis::COLUMN);
    box->setWidth(w);
    box->setHeight(h);
    box->setCornerRadius(radius);
    box->setHighlightCornerRadius(radius);
    box->setBackgroundColor(kCard);
    box->setJustifyContent(brls::JustifyContent::CENTER);
    box->setAlignItems(brls::AlignItems::CENTER);
    box->setFocusable(true);
    box->addGestureRecognizer(new brls::TapGestureRecognizer(box));
    return box;
}

// After a system applet closes, ignore the button press that closed it.
void swallow_input() {
    brls::Application::blockInputs();
    brls::delay(300, []() { brls::Application::unblockInputs(); });
}

agatha::YouTubeEntry g_entry = agatha::YouTubeEntry::None;

}  // namespace

// ---------- agatha:: helpers ----------
namespace agatha {

YouTubeEntry youtube_entry() { return g_entry; }
void set_youtube_entry(YouTubeEntry entry) { g_entry = entry; }

std::string youtube_video_id(const std::string& url) {
    if (url.find("youtube.com") == std::string::npos && url.find("youtu.be") == std::string::npos) return "";
    auto grab = [&url](size_t pos) {
        std::string id;
        while (pos < url.size() && id.size() < 11) {
            const char c = url[pos++];
            if (std::isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_') id += c;
            else break;
        }
        return id.size() == 11 ? id : std::string();
    };
    for (const char* key : {"?v=", "&v=", "youtu.be/", "/shorts/", "/embed/", "/live/"}) {
        const size_t p = url.find(key);
        if (p != std::string::npos) {
            std::string id = grab(p + std::char_traits<char>::length(key));
            if (!id.empty()) return id;
        }
    }
    return "";
}

void enable_capture() {
#if defined(__SWITCH__)
    if (!application_mode()) return;  // capture belongs to the running application (title override)
    appletSetScreenShotPermission(AppletScreenShotPermission_Enable);
    if (R_SUCCEEDED(appletInitializeGamePlayRecording())) appletSetGamePlayRecordingState(true);
#endif
}

}  // namespace agatha

// ---------- Activity ----------
void AgathaActivity::onContentAvailable() {
    load_bookmarks();
    this->buildCards();
    this->buildActions();

    if (!application_mode())
        this->setStatus("Applet mode: the web browser needs application mode (hold R while launching a game).");

    if (!this->cards_.empty() && this->cards_[0].card) brls::Application::giveFocus(this->cards_[0].card);
}

void AgathaActivity::onResume() {
    // Back on the home screen: whatever YouTube screen we opened has been closed.
    agatha::set_youtube_entry(agatha::YouTubeEntry::None);
}

void AgathaActivity::buildCards() {
    if (!this->cardsBox) return;
    for (size_t i = 0; i < kBookmarkCount; i++) {
        CardViews v;
        v.card = make_tile(kCardW, kCardH, kCardRadius);
        if (i + 1 < kBookmarkCount) v.card->setMarginRight(kCardGap);

        auto* badge = new brls::Box(brls::Axis::ROW);
        badge->setWidth(68);
        badge->setHeight(68);
        badge->setCornerRadius(34);
        badge->setBackgroundColor(kBadge[i]);
        badge->setJustifyContent(brls::JustifyContent::CENTER);
        badge->setAlignItems(brls::AlignItems::CENTER);
        v.letter = make_label("", 32, nvgRGB(255, 255, 255));
        badge->addView(v.letter);
        v.card->addView(badge);

        v.name = make_label("", 22, kText, kCardW - 24);
        v.name->setMarginTop(18);
        v.card->addView(v.name);

        v.domain = make_label("", 16, kSubtle, kCardW - 24);
        v.domain->setMarginTop(6);
        v.card->addView(v.domain);

        v.card->registerClickAction([this, i](brls::View*) {
            this->openUrl(g_bookmarks[i].url);
            return true;
        });
        v.card->registerAction("Edit", brls::BUTTON_X, [this, i](brls::View*) {
            this->editBookmark(i);
            return true;
        });
        v.card->registerAction("Reset", brls::BUTTON_BACK, [this, i](brls::View*) {
            this->resetBookmark(i);
            return true;
        });

        this->cardsBox->addView(v.card);
        this->cards_[i] = v;
        this->refreshCard(i);
    }
}

void AgathaActivity::buildActions() {
    if (!this->actionsBox) return;
    struct Action {
        const char* label;
        std::function<void()> run;
    };
    const std::vector<Action> actions = {
        {"Enter URL", [this]() { this->promptUrl(); }},
        {"YouTube", [this]() { this->openYouTubeHome(); }},
        {"Exit", []() { brls::Application::quit(); }},
    };
    for (size_t j = 0; j < actions.size(); j++) {
        auto* tile = make_tile(kActW, kActH, kActH / 2);
        if (j + 1 < actions.size()) tile->setMarginRight(kActGap);
        tile->addView(make_label(actions[j].label, 24, kText));
        auto run = actions[j].run;
        tile->registerClickAction([run](brls::View*) {
            run();
            return true;
        });
        this->actionsBox->addView(tile);
    }
}

void AgathaActivity::refreshCard(size_t i) {
    const CardViews& v = this->cards_[i];
    if (!v.card) return;
    v.letter->setText(initial_of(g_bookmarks[i].name));
    v.name->setText(g_bookmarks[i].name);
    v.domain->setText(domain_of(g_bookmarks[i].url));
}

void AgathaActivity::setStatus(const std::string& text) {
    if (this->statusLabel) this->statusLabel->setText(text);
}

void AgathaActivity::openUrl(const std::string& url) {
    this->setStatus("");

    // YouTube can't run in the system browser (blank page), so it goes to the built-in player.
    const std::string videoId = agatha::youtube_video_id(url);
    if (!videoId.empty()) {
        this->openYouTubeVideo(videoId);
        return;
    }
    if (url.find("youtube.com") != std::string::npos || url.find("youtu.be") != std::string::npos) {
        this->openYouTubeHome();
        return;
    }

    if (!application_mode()) {
        this->setStatus("The web browser needs application mode: hold R while launching a game.");
        return;
    }

    std::string lastUrl, error;
    newpipe::logf("agatha: open %s", url.c_str());
    const bool ok = show_web_page(url, lastUrl, error);
    swallow_input();
    if (!ok) {
        this->setStatus(error);
        return;
    }

    // Tapped a YouTube link inside an article? Open that video directly.
    const std::string lastVideo = agatha::youtube_video_id(lastUrl);
    if (!lastVideo.empty()) this->openYouTubeVideo(lastVideo);
}

void AgathaActivity::openYouTubeHome() {
    agatha::set_youtube_entry(agatha::YouTubeEntry::Browse);
    brls::Application::pushActivity(new MainActivity());
}

void AgathaActivity::openYouTubeVideo(const std::string& videoId) {
    newpipe::StreamItem item;
    item.id  = videoId;
    item.url = "https://www.youtube.com/watch?v=" + videoId;
    agatha::set_youtube_entry(agatha::YouTubeEntry::DeepLink);
    brls::Application::pushActivity(new StreamDetailActivity(item));
}

void AgathaActivity::promptUrl() {
    brls::Application::getImeManager()->openForText(
        [this](std::string text) {
            if (text.empty()) return;
            const std::string url = normalize_url(sanitize(text));
            brls::sync([this, url]() { this->openUrl(url); });
        },
        "Enter a URL", "", 500, "https://");
}

void AgathaActivity::editBookmark(size_t i) {
    brls::Application::getImeManager()->openForText(
        [this, i](std::string name) {
            if (name.empty()) return;
            name = sanitize(name);
            // Second keyboard (URL) opens after the first one has fully closed.
            brls::sync([this, i, name]() {
                brls::Application::getImeManager()->openForText(
                    [this, i, name](std::string url) {
                        if (url.empty()) return;
                        g_bookmarks[i] = {name, normalize_url(sanitize(url))};
                        this->refreshCard(i);
                        this->setStatus(save_bookmarks() ? "Saved \"" + name + "\""
                                                         : std::string("Could not save to ") + kConfigFile);
                    },
                    "Bookmark URL", "", 500, g_bookmarks[i].url);
            });
        },
        "Bookmark name", "", 32, g_bookmarks[i].name);
}

void AgathaActivity::resetBookmark(size_t i) {
    g_bookmarks[i] = kDefaults[i];
    this->refreshCard(i);
    this->setStatus(save_bookmarks() ? "Bookmark " + std::to_string(i + 1) + " reset to default"
                                     : std::string("Could not save to ") + kConfigFile);
}
