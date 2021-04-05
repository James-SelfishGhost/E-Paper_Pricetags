#pragma once
#include "RFV3.h"

class ModeWuReset : public mode_class
{
public:
  virtual void interrupt()
  {
    if (handle_wu())
    {
      log(mode_name + " done");
      set_mode_idle();
    }
  }

  virtual void new_interval()
  {
    check_wu_timeout();
  }

  virtual void pre()
  {
    log(mode_name);
    start_wu(WU_RESET_CMD);
  }

  virtual void main()
  {
  }

  virtual void post()
  {
  }

  virtual String get_name()
  {
    return mode_name;
  }

private:
  String mode_name = "Wakeup Reset";
};

ModeWuReset modeWuReset;
