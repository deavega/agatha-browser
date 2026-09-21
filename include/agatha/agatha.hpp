#pragma once

#include <string>

namespace agatha {

// How the user entered the YouTube part of the app from the Agatha home screen.
// Used to rebuild the right screens after NewPipe's player (which restarts the UI) closes.
enum class YouTubeEntry { None, Browse, DeepLink };

YouTubeEntry youtube_entry();
void set_youtube_entry(YouTubeEntry entry);

// Returns the 11-character video id for youtube.com / youtu.be / shorts / embed links, or "".
std::string youtube_video_id(const std::string& url);

// Screenshots and video capture (Switch, application mode only).
void enable_capture();

}  // namespace agatha
