#if defined(__SWITCH__)
#include <switch.h>
#endif

#include <borealis.hpp>
#include <cstdlib>
#include <exception>
#include <string>

#include "activity/agatha_activity.hpp"
#include "activity/main_activity.hpp"
#include "agatha/agatha.hpp"
#include "newpipe/auth_store.hpp"
#include "newpipe/image_loader.hpp"
#include "newpipe/i18n.hpp"
#include "newpipe/log.hpp"
#include "newpipe/runtime.hpp"
#include "newpipe/settings_store.hpp"
#if defined(__SWITCH__)
#include "newpipe/switch_player.hpp"
#endif
#include "tab/home_tab.hpp"
#include "tab/search_tab.hpp"
#include "tab/settings_tab.hpp"
#include "tab/subscriptions_tab.hpp"
#include "tab/library_tab.hpp"
#include "view/auto_tab_frame.hpp"
#include "view/rubik_label.hpp"
#include "view/svg_image.hpp"

namespace {

void configure_theme() {
    brls::Application::getPlatform()->setThemeVariant(brls::ThemeVariant::DARK);

    brls::Theme::getDarkTheme().addColor("color/newpipe", nvgRGB(244, 67, 54));
    brls::Theme::getDarkTheme().addColor("color/newpipe_bg", nvgRGB(18, 18, 18));
    brls::Theme::getDarkTheme().addColor("color/newpipe_card", nvgRGB(28, 28, 28));
    brls::Theme::getDarkTheme().addColor("color/grey_1", nvgRGB(28, 28, 28));
    brls::Theme::getDarkTheme().addColor("color/grey_2", nvgRGB(36, 38, 42));
    brls::Theme::getDarkTheme().addColor("color/grey_3", nvgRGBA(160, 160, 160, 160));

    brls::Theme::getLightTheme().addColor("color/newpipe", nvgRGB(216, 67, 21));
    brls::Theme::getLightTheme().addColor("color/newpipe_bg", nvgRGB(248, 248, 248));
    brls::Theme::getLightTheme().addColor("color/newpipe_card", nvgRGB(255, 255, 255));
    brls::Theme::getLightTheme().addColor("color/grey_1", nvgRGB(255, 255, 255));
    brls::Theme::getLightTheme().addColor("color/grey_2", nvgRGB(235, 236, 238));
    brls::Theme::getLightTheme().addColor("color/grey_3", nvgRGBA(200, 200, 200, 16));

    // Agatha Browser: purple focus highlight everywhere
    brls::Theme::getDarkTheme().addColor("brls/highlight/color1", nvgRGB(124, 92, 255));
    brls::Theme::getDarkTheme().addColor("brls/highlight/color2", nvgRGB(176, 156, 255));

    brls::getStyle().addMetric("brls/tab_frame/sidebar_width", 160);
}

void register_views() {
    brls::Application::registerXMLView("AutoTabFrame", AutoTabFrame::create);
    brls::Application::registerXMLView("SVGImage", SVGImage::create);
    brls::Application::registerXMLView("RubikLabel", RubikLabel::create);
    brls::Application::registerXMLView("HomeTab", HomeTab::create);
    brls::Application::registerXMLView("SearchTab", SearchTab::create);
    brls::Application::registerXMLView("SubscriptionsTab", SubscriptionsTab::create);
    brls::Application::registerXMLView("LibraryTab", LibraryTab::create);
    brls::Application::registerXMLView("SettingsTab", SettingsTab::create);
}

void load_rubik_font() {
#ifdef USE_LIBROMFS
    auto& font = romfs::get("font/Rubik-Regular.ttf");
    if (font.valid() && brls::Application::loadFontFromMemory("rubik", (void*) font.data(), font.size(), false)) {
        return;
    }
#else
    if (brls::Application::loadFontFromFile("rubik", std::string(BRLS_RESOURCES) + "font/Rubik-Regular.ttf")) {
        return;
    }
#endif
    newpipe::log_line("main: Rubik font not loaded, using default font");
}

bool run_borealis_ui() {
    bool image_loader_started = false;
    newpipe::log_line("main: borealis init begin");

    if (!brls::Application::init()) {
        newpipe::log_line("main: Application::init failed");
        return false;
    }

    newpipe::log_line("main: createWindow");
    brls::Application::createWindow(newpipe::tr("app/title"));
    brls::Application::setGlobalQuit(false);
    configure_theme();

    newpipe::log_line("main: register XML views");
    register_views();

    newpipe::log_line("main: start ImageLoader");
    newpipe::ImageLoader::instance().start();
    image_loader_started = true;

    load_rubik_font();

    // Agatha home is always the bottom screen. After NewPipe's player closes (the UI is
    // restarted for playback), put the user back where they came from.
    newpipe::log_line("main: push AgathaActivity");
    brls::Application::pushActivity(new AgathaActivity(), brls::TransitionAnimation::NONE);
    if (agatha::youtube_entry() == agatha::YouTubeEntry::Browse) {
        newpipe::log_line("main: resume MainActivity");
        brls::Application::pushActivity(new MainActivity(), brls::TransitionAnimation::NONE);
    } else {
        agatha::set_youtube_entry(agatha::YouTubeEntry::None);
    }

    const std::string playback_error = newpipe::take_last_playback_error();
    if (!playback_error.empty()) {
        brls::sync([playback_error]() {
            auto* dialog =
                new brls::Dialog(newpipe::tr("app/playback_failed", playback_error));
            dialog->addButton(newpipe::tr("hints/ok"), [dialog]() { dialog->close(); });
            dialog->setCancelable(true);
            dialog->open();
        });
    }

    newpipe::log_line("main: enter mainLoop");
    try {
        while (brls::Application::mainLoop()) {
        }
    } catch (...) {
        if (image_loader_started) {
            newpipe::ImageLoader::instance().stop();
        }
        throw;
    }
    newpipe::log_line("main: mainLoop exit");

    if (image_loader_started) {
        newpipe::ImageLoader::instance().stop();
    }
    return true;
}

}  // namespace

int main(int argc, char* argv[]) {
    (void) argc;
    (void) argv;

    newpipe::init_log();
    newpipe::log_line("main: start");
    agatha::enable_capture();
    try {
        std::string auth_error;
        if (!newpipe::AuthStore::instance().load(&auth_error) && !auth_error.empty()) {
            newpipe::logf("main: auth load failed error=%s", auth_error.c_str());
        }
        std::string settings_error;
        if (!newpipe::SettingsStore::instance().load(&settings_error) && !settings_error.empty()) {
            newpipe::logf("main: settings load failed error=%s", settings_error.c_str());
        }
        brls::Logger::setLogLevel(brls::LogLevel::LOG_DEBUG);
        while (true) {
            brls::Platform::APP_LOCALE_DEFAULT = newpipe::locale_from_setting(
                newpipe::SettingsStore::instance().settings().language);
            newpipe::clear_pending_playback();
            if (!run_borealis_ui()) {
                newpipe::shutdown_log();
                return EXIT_FAILURE;
            }

#if defined(__SWITCH__)
            const auto pending_playback = newpipe::take_pending_playback();
            if (!pending_playback.has_value()) {
                break;
            }

            newpipe::logf("main: launch player title=%s", pending_playback->title.c_str());
            std::string playback_error;
            if (!newpipe::run_switch_player(*pending_playback, playback_error)) {
                newpipe::logf("main: player failed error=%s", playback_error.c_str());
                newpipe::set_last_playback_error(playback_error.empty() ? "unknown error" : playback_error);
            } else {
                newpipe::log_line("main: player finished");
            }
            continue;
#else
            break;
#endif
        }

        newpipe::shutdown_log();
        return EXIT_SUCCESS;
    } catch (const std::exception& ex) {
        newpipe::logf("main: exception: %s", ex.what());
    } catch (...) {
        newpipe::log_line("main: unknown exception");
    }

    newpipe::shutdown_log();
    return EXIT_FAILURE;
}
