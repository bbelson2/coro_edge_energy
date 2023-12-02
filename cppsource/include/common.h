#ifndef COMMON_H
#define COMMON_H

#ifndef FPM_FIXED_HPP
#error svm.h requires fpm/fixed.hpp
#endif

// Number of frequencies in a datagram
//#define ENV_FREQ_COUNT 510
// Datagram has a prepended numeric sensor ID and sequence number, so size is:
//#define DATAGRAM_ITEM_COUNT (ENV_FREQ_COUNT + 2)

#define MIN_SV_LEN 252

#define XSTR(s) STR(s)
#define STR(s) #s

// Data type used by transmitter and receiver
typedef std::int16_t data_item_base_t;
using fixed_3_13 = fpm::fixed<data_item_base_t, std::int32_t, 13>;
typedef fixed_3_13 data_item_t;
typedef std::uint32_t id_t;

struct x_row_t {
  id_t sensor_id;
  id_t seq_id;
  data_item_t data[2];
};

// Command frames contain { SENSOR_ID_CMD, cmd, 0* }
#define SENSOR_ID_CMD           ((id_t)-1)
#define CMD_ID_RESET_SEQUENCES  ((id_t)0)
#define CMD_ID_STOP_SERVER      ((id_t)1)
#define CMD_ID_USE_SEQ          ((id_t)2)
#define CMD_ID_USE_CORO         ((id_t)3)
#define CMD_ID_USE_BOTH         ((id_t)4)

// Execution models and patterns for selecting model
#define EXEC_MODEL_SEQ    0
#define EXEC_MODEL_CORO   1

#define EXEC_PATTERN_SEQ  0
#define EXEC_PATTERN_CORO 1
#define EXEC_PATTERN_BOTH 2

#endif // COMMON_H

