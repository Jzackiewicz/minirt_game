#include "SessionProgress.hpp"

namespace {
SessionProgress g_session_progress;
}

const SessionProgress &get_session_progress() { return g_session_progress; }

void set_session_progress(const SessionProgress &progress) { g_session_progress = progress; }

void clear_session_progress() { g_session_progress = SessionProgress{}; }

