#pragma once

#ifdef WIRINGPI_MOCK

#include <fstream>
#include <filesystem>
#include <iostream>
#include <chrono>
#include <utils.h>

class gpio
{
public:
  gpio(uint16_t verbosity) 
    : verbosity_(verbosity) {}
  ~gpio()
  {
    term();
  }
  void init()
  {
    term();

    using namespace std::filesystem;
    auto exe_dir = path(get_self_path());
    if (exe_dir.empty())
    {
      std::cerr << "Unable to find executable directory" << std::endl;
      return;
    }
    auto data_dir = exe_dir.parent_path() / ".." / "data";
    if (!exists(data_dir) && !create_directories(data_dir))
    {
      std::cerr << "Unable to create data directory" << data_dir << std::endl;
      return;
    }
    auto data_path = data_dir / "gpio.csv";
    ofs_mock_gpio_.open(data_path.c_str(), std::ios_base::app);
    if (verbosity_ >= 1)
    {
      std::cout << "Mock GPIO output -> [" << data_path << "] "
                << (ofs_mock_gpio_.is_open() ? "ok" : "failed") << std::endl;
    }
  }
  void set(int pinid, bool high)
  {
    if (ofs_mock_gpio_.is_open())
    {
      // See https://stackoverflow.com/a/31258680
      using time_stamp = std::chrono::time_point<std::chrono::system_clock,
                                                 std::chrono::microseconds>;
      using namespace std::chrono;
      time_stamp ts = time_point_cast<microseconds>(system_clock::now());
      ofs_mock_gpio_ << pinid << "\t" << high << "\t" << ts.time_since_epoch().count() << std::endl;
    }
  }
  void term()
  {
    if (ofs_mock_gpio_.is_open())
    {
      ofs_mock_gpio_.close();
    }
  }

private:
  int pinid_;
  uint16_t verbosity_;
  std::ofstream ofs_mock_gpio_;
};

#else

#include <wiringPi.h>

// Suppress warning about verbosity_
#pragma GCC diagnostic ignored "-Wunused-private-field"
 
class gpio
{
public:
  gpio(uint16_t verbosity) 
    : verbosity_(verbosity) 
  {
    pins[0] = 0;
    pins[1] = 2;
  }
  ~gpio()
  {
    term();
  }
  void init()
  {
    wiringPiSetup();    // Setup the library
    pinMode(pins[0], OUTPUT); // Configure GPIO0 as an output
    pinMode(pins[1], OUTPUT); // Configure GPIO2 as an output
  }
  void set(int pinid, bool high)
  {
    digitalWrite(pins[pinid], high ? 1 : 0);
  }
  void term()
  {
    digitalWrite(pins[0], 0);
    digitalWrite(pins[1], 0);
  }
protected:
  static int pin_from_pinid(int pinid) {
    return pinid == 0 ? 0 : 2;
  }
private:
  int pins[2];
  uint16_t verbosity_;
};

#pragma GCC diagnostic warning "-Wunused-private-field"

#endif
