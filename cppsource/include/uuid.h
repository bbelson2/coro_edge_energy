#pragma once
#include <random>
#include <sstream>

#define UUID_CHARS 36
#define UUID_SIZE 16

// https://stackoverflow.com/a/60198074
namespace uuid
{
  // static std::random_device              rd;
  // static std::mt19937                    gen(rd());
  static std::mt19937 gen(1234);
  static std::uniform_int_distribution<> dis(0, 15);
  static std::uniform_int_distribution<> dis2(8, 11);

  std::string generate_uuid_v4()
  {
    std::stringstream ss;
    int i;
    ss << std::hex;
    for (i = 0; i < 8; i++)
    {
      ss << dis(gen);
    }
    ss << "-";
    for (i = 0; i < 4; i++)
    {
      ss << dis(gen);
    }
    ss << "-4";
    for (i = 0; i < 3; i++)
    {
      ss << dis(gen);
    }
    ss << "-";
    ss << dis2(gen);
    for (i = 0; i < 3; i++)
    {
      ss << dis(gen);
    }
    ss << "-";
    for (i = 0; i < 12; i++)
    {
      ss << dis(gen);
    };
    return ss.str();
  }

  void generate_uuid_v4_num(uint8_t *key)
  {
    int i;
    for (i = 0; i < 6; i++)
    {
      key[i] = (dis(gen) << 4) | dis(gen);
    }
    key[i++] = (4 << 4) | dis(gen);
    key[i++] = (dis(gen) << 4) | dis(gen);
    key[i++] = (dis2(gen) << 4) | dis(gen);
    for (; i < 16; i++)
    {
      key[i] = (dis(gen) << 4) | dis(gen);
    }
  }

  std::string tostring(const unsigned char *key)
  {
    const char *v = "0123456789abcdef";
    const bool dash[] = {0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0};
    std::string res;
    for (int i = 0; i < 16; i++)
    {
      if (dash[i])
        res += "-";
      auto ch = key[i];
      res += v[(ch & 0xf0) >> 4];
      res += v[ch & 0x0f];
    }
    return res;
  }
}

#include <string.h>

// https://stackoverflow.com/a/58467162
namespace uuid_simple
{
  std::string get_uuid()
  {
    // static std::random_device dev;
    // static std::mt19937 rng(dev());
    static std::mt19937 rng(4321); // repeatable

    std::uniform_int_distribution<int> dist(0, 15);

    const char *v = "0123456789abcdef";
    const bool dash[] = {0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0};

    std::string res;
    for (int i = 0; i < 16; i++)
    {
      if (dash[i])
        res += "-";
      res += v[dist(rng)];
      res += v[dist(rng)];
    }
    return res;
  }
  void get_uuid(char *buf36)
  {
    memcpy(buf36, get_uuid().c_str(), UUID_CHARS);
  }
}

