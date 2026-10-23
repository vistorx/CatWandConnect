#pragma once

// The Play session journal and the Robot clock on the robot (lib/play_journal,
// docs/adr/0013): records what the Robot core reports, with the source of the
// command being handled. Every member is used under DeviceApi's robot lock.

#include <play_journal/play_journal.h>
#include <robot_core/ports.h>

#include "log.h"

class PlayLog : public cwc::PlayObserver {
 public:
  PlayLog(cwc::Clock& uptime, cwc::Storage& storage) : clock(uptime), journal(storage, clock) {}

  // The source of the command about to reach the Robot core.
  void setSource(play_journal::Source source) { source_ = source; }

  void playSession(cwc::PlayEvent event, cwc::PlayCause cause) override {
    play_journal::What what = play_journal::whatOf(event);
    // The end of the Session duration belongs to whoever started it.
    play_journal::Source source = what == play_journal::What::Ended ? startSource_ : source_;
    if (what == play_journal::What::Started) startSource_ = source_;
    play_journal::Reason reason = play_journal::reasonOf(cause);
    const play_journal::Event& e = journal.record(what, source, reason);
    const char* reasonName = play_journal::reasonName(reason);
    logLine("[play] Play session %s%s%s, from %s (journal #%u)", play_journal::whatName(what),
            reasonName ? ": " : "", reasonName ? reasonName : "", play_journal::sourceName(source),
            unsigned(e.id));
  }

  play_journal::RobotClock clock;
  play_journal::Journal journal;

 private:
  play_journal::Source source_ = play_journal::Source::DeviceInterface;
  play_journal::Source startSource_ = play_journal::Source::DeviceInterface;
};
