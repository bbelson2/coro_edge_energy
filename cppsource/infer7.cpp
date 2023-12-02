#define USE_GENERIC_COROUTINE_RUNNER

#ifndef USE_FPM
#define USE_FPM 1
#endif

#if USE_FPM==1
#include <fpm/fixed.hpp>
#else
#include <fixed_point.h>
#endif

#include <tclap/CmdLine.h>
#include <svm.h>
// #include <common.h>
#include <vector>
#include <iostream>
#include <fstream>
#include <utils.h>
#include <gpio.h>
#include <timer.h>
#include <memory>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/udp.h>

#define TLX_BTREE_FRIENDS friend class btree_accessor
#include <tlx/container/btree.hpp>
#include <tlx/container/btree_map.hpp>
#include <uuid.h>

#if defined(_MSC_VER)
#define CORO_STD std::experimental
#elif defined(__clang__)
#define CORO_STD std::experimental
#else
#define CORO_STD std
#endif

#include <resumable.h>
#include <prefetch1.h>
#include <bounded_random.h>
#ifdef USE_GENERIC_COROUTINE_RUNNER
#include <run_coro.h>
#endif

#ifndef MEASURE_LOCALITY
#define MEASURE_LOCALITY 0
#endif

////////////////////////////////////////////////////////////////
// Data types (to be shared with transmitter)
////////////////////////////////////////////////////////////////

typedef std::int16_t data_item_base_t;
#if USE_FPM==1
using fixed_3_13 = fpm::fixed<data_item_base_t, std::int32_t, 13>;
#else
using fixed_3_13 = fix_t;
#endif
typedef fixed_3_13 data_item_t;

typedef std::vector<data_item_t> data_vector_t;

struct bpt_key_t
{
  uint8_t uid[UUID_SIZE];
};
struct bpt_key_less
{
  bool operator()(const bpt_key_t &lhs, const bpt_key_t &rhs) const
  {
    return memcmp(lhs.uid, rhs.uid, sizeof(lhs.uid)) < 0;
  }
};

struct datagram_t
{
  bpt_key_t sensor_id;
  uint32_t seq_id;
  data_item_t data[2];
};

inline uint32_t svm_len_from_datagram_bytes(uint32_t datagram_size)
{
  // there are also 2 items in the header
  return ((datagram_size - sizeof(datagram_t)) / sizeof(data_item_t)) + 2; 
}

typedef int result_t;

#define MIN_SV_LEN 2
#define XSTR(s) STR(s)
#define STR(s) #s

// Execution models and patterns for selecting model
#define EXEC_MODEL_SEQ 0
#define EXEC_MODEL_CORO 1

#define EXEC_PATTERN_SEQ 0
#define EXEC_PATTERN_CORO 1
#define EXEC_PATTERN_BOTH 2

////////////////////////////////////////////////////////////////
// Trace helper
////////////////////////////////////////////////////////////////

#if USE_FPM==1  
void dump_fp_vector(const std::vector<data_item_t>& v, std::ostream& os, const char* label) {
  dump_vector(v, os, label);
}
#else
void dump_fp_vector(const std::vector<data_item_t>& v, std::ostream& os, const char* label) {
  dump_vector(fix_int_to_float(v), os, label);
}
#endif        

void dump_guid_vector(std::vector<bpt_key_t> &sensor_ids, std::ostream& os, const char* label)
{
  std::vector<std::string> sensor_guids(sensor_ids.size());
  std::transform(sensor_ids.begin(), sensor_ids.end(), sensor_guids.begin(),
    [](const bpt_key_t& key){return uuid::tostring(key.uid);});
  dump_vector(sensor_guids, os, label);
}

////////////////////////////////////////////////////////////////
// Input simulator
////////////////////////////////////////////////////////////////

typedef uint16_t clear_cache_t;

// This is placed in global space to prevent the optimiser
// from optimising out the whole of clear_cache()
std::vector<std::vector<clear_cache_t>> cached_buf;

void clear_cache()
{
  std::vector<std::vector<clear_cache_t>> &buf = cached_buf;
  size_t outer = 64000;
  size_t inner = 1000;
  for (size_t i = 0; i < outer; i++)
  {
    std::vector<clear_cache_t> inner_buf(inner);
    for (size_t j = 0; j < inner; j++)
    {
      inner_buf[j] = (clear_cache_t)j;
    }
    buf.push_back(inner_buf);
  }
  buf.clear();
}

////////////////////////////////////////////////////////////////
// Input simulator
////////////////////////////////////////////////////////////////

class input_receiver {
public:
  virtual bool get_next_input(std::vector<data_item_t>& buffer)=0;
  virtual void reset()=0;
  virtual bool stop_requested() const=0;
  virtual ~input_receiver() {}
};

class input_simulator : public input_receiver {
public:
  input_simulator(const std::vector<bpt_key_t>& sensor_ids, 
    uint32_t sample_count, uint32_t datagram_size,
    rnd_bounds bounds)
    : sensor_ids_(sensor_ids), sample_count_(sample_count),
      datagram_size_(datagram_size), 
      svm_len_(svm_len_from_datagram_bytes(datagram_size_)),
      distribution_(bounds)
  {
    sensor_indices_.resize(sensor_ids_.size());
    std::iota(sensor_indices_.begin(), sensor_indices_.end(), 0);
  }
  virtual ~input_simulator() {}
  virtual void reset()
  {
    if (!engine_initialised_)
    {
      engine_.seed(5489);
      engine_initialised_ = true;
    }
    std::shuffle(sensor_indices_.begin(), sensor_indices_.end(), shuffler_);
    current_seq_id_ = 0;
    current_sensor_index_index_ = 0;
  }
  virtual bool get_next_input(std::vector<data_item_t>& buffer)
  {
    if (current_sensor_index_index_ == sensor_ids_.size())
    {
      current_sensor_index_index_ = 0;
      current_seq_id_++;
      std::shuffle(sensor_indices_.begin(), sensor_indices_.end(), shuffler_);
    }
    if (current_seq_id_ == sample_count_)
    {
      return false;
    }

    // Identify source
    uint32_t current_sensor_index = sensor_indices_[current_sensor_index_index_];

    // Prepare buffer
    buffer.resize(datagram_size_ / sizeof(data_item_t));
    datagram_t* pdata = (datagram_t*)buffer.data();

    // Copy header
    pdata->sensor_id = sensor_ids_[current_sensor_index];
    pdata->seq_id = current_seq_id_;
    
    // Fill data
    auto rand_ampl = [&]() { return distribution_(engine_); };
    std::generate(pdata->data, pdata->data + svm_len_, rand_ampl);

    // Move to next record...
    current_sensor_index_index_++;
    return true;
  }
  virtual bool stop_requested() const { return false; }
private:
  // Parameters
  std::vector<bpt_key_t> sensor_ids_; // Could be a reference?
  uint32_t sample_count_; 
  uint32_t datagram_size_;
  uint32_t svm_len_;
  // Machinery
  static bool engine_initialised_;
  static std::mt19937 engine_; // Mersenne twister MT19937
  static std::default_random_engine shuffler_;
  bounded_distribution<data_item_t> distribution_;
  // Simulated dataset & cursors
  std::vector<uint32_t> sensor_indices_;
  uint32_t current_seq_id_;
  uint32_t current_sensor_index_index_;
};

bool input_simulator::engine_initialised_ = false;
std::default_random_engine input_simulator::shuffler_;
std::mt19937 input_simulator::engine_; // Mersenne twister MT19937

////////////////////////////////////////////////////////////////
// B+Tree for storing per-sensor data
////////////////////////////////////////////////////////////////

// Key is a numeric GUID
// Data is a row number in  the storage area (weights vector)
typedef uint32_t bpt_data_t;

template <typename Key, typename Value, int mult_factor, int div_factor>
struct btree_modified_traits {
  typedef tlx::btree_default_traits<Key, std::pair<Key, Value> > bpt_base_traits;
  static const bool self_verify = bpt_base_traits::self_verify;
  static const bool debug = bpt_base_traits::debug;
  static const int leaf_slots = (int)((bpt_base_traits::leaf_slots * mult_factor) / div_factor);
  static const int inner_slots = (int)((bpt_base_traits::inner_slots * mult_factor) / div_factor);
  static const size_t binsearch_threshold = bpt_base_traits::binsearch_threshold;
};

#ifndef NODE_COUNT_MULT_FACTOR
#define NODE_COUNT_MULT_FACTOR 1
#endif

#ifndef NODE_COUNT_DIV_FACTOR
#define NODE_COUNT_DIV_FACTOR 1
#endif

typedef btree_modified_traits<bpt_key_t, bpt_data_t, 
    // 1, 1> bpt_traits;
    NODE_COUNT_MULT_FACTOR, NODE_COUNT_DIV_FACTOR> bpt_traits;

// typedef tlx::btree_default_traits<bpt_key_t, std::pair<bpt_key_t, bpt_data_t> > bpt_traits;

typedef tlx::btree_map<bpt_key_t, bpt_data_t, bpt_key_less, bpt_traits> bpt_map_t;
// typedef tlx::btree_map<bpt_key_t, bpt_data_t, bpt_key_less> bpt_map_t;
typedef bpt_map_t::btree_impl bpt_tree_t;

////////////////////////////////////////////////////////////////
// Run-time settings and command line parser
////////////////////////////////////////////////////////////////

struct run_time_settings_t
{
  // Run-time
  uint16_t verbosity;
  // Data size
  uint32_t sensor_count;
  uint32_t sample_count;
  uint32_t datagram_size;
  uint32_t repeats;
  uint16_t task_count;
  // Canned data
  std::string weights_file;
  // Simulator
  bool simulate_weights;
  rnd_bounds weights_bounds;

  // Transmitted/received data
  bool simulate_amplitudes;

  // Canned data
  std::string data_source;
  // Simulator
  rnd_bounds amplitude_bounds;

  // Report
  bool skip_header;
  std::string report_file;
  std::string perf_file;
  // Cached
  uint32_t sv_len;
  uint32_t x_len;
  uint32_t w_len;

  // Execution
  int exec_pattern;
  int exec_model; // Current model
  uint32_t delay_ms; // Initial delay in ms
  uint32_t between_ms; // Wait between operations in ms

  void validate()
  {
    if (sensor_count == 0)
    {
      throw std::domain_error("sensor_count must be positive");
    }
    if (sample_count == 0)
    {
      throw std::domain_error("sample_count must be positive");
    }

    if (!simulate_weights && weights_file.empty())
    {
      throw std::domain_error("if simulate_weights is false, weights_file must be provided");
    }
    if (simulate_weights && !weights_bounds.is_valid())
    {
      throw std::domain_error("weights.min may not be greater than weights.max; weights.granularity must be non-zero.");
    }
    if (task_count == 0 || task_count >= 17)
    {
      throw std::domain_error("task_count must be a positive integer less than 17");
    }
    if (datagram_size < sizeof(datagram_t))
    {
      throw std::domain_error(std::string("datagram_size is less than header size (") + std::to_string(sizeof(datagram_t)) + ")");
    }
    sv_len = svm_len_from_datagram_bytes(datagram_size);
    if (sv_len < MIN_SV_LEN)
    {
      throw std::domain_error("support vector length must be at least " XSTR(MIN_SV_LEN));
    }
    x_len = datagram_size / sizeof(data_item_t);
    w_len = sv_len + 1;

    if (!simulate_amplitudes && data_source.empty())
    {
      throw std::domain_error("if simulate is false, data_source must be provided");
    }
    if (simulate_amplitudes && !amplitude_bounds.is_valid())
    {
      throw std::domain_error("amplitudes.min may not be greater than amplitudes.max; amplitudes.granularity must be non-zero.");
    }
  }

  void dump(std::ostream &os)
  {
    os << "verbosity"
       << "\t" << verbosity << std::endl;
    os << "sensor_count"
       << "\t" << sensor_count << std::endl;
    os << "sample_count"
       << "\t" << sample_count << std::endl;
    os << "datagram_size"
       << "\t" << datagram_size << std::endl;
    os << "task_count"
       << "\t" << task_count << std::endl;
    os << "simulate_weights"
       << "\t" << simulate_weights << std::endl;
    os << "weights_file"
       << "\t" << weights_file << std::endl;
    os << "weights_bounds"
       << "\t" << weights_bounds << std::endl;

    os << "data_source"
       << "\t" << data_source << std::endl;
    os << "amplitude_bounds"
       << "\t" << amplitude_bounds << std::endl;

    os << "skip_header"
       << "\t" << skip_header << std::endl;
    os << "report_file"
       << "\t" << report_file << std::endl;
    os << "perf_file"
       << "\t" << perf_file << std::endl;

    os << "sv_len"
       << "\t" << sv_len << std::endl;
    os << "x_len"
       << "\t" << x_len << std::endl;
    os << "w_len"
       << "\t" << w_len << std::endl;

    os << "exec_pattern"
       << "\t" << exec_pattern << std::endl;
    os << "exec_model"
       << "\t" << exec_model << std::endl;
    os << "delay_ms"
       << "\t" << delay_ms << std::endl;
    os << "between_ms"
       << "\t" << between_ms << std::endl;

    os << "repeats"
       << "\t" << repeats << std::endl;
  }
};

/**
 * @brief Parses the command line arguments into a run-time
 * settings structure
 *
 * @param argc The command line argument count
 * @param argv The command line arguments
 * @param rt run-time settings to be populated by the parser
 * @return true if parsing succeeds
 * @return false on parsing error
 */
bool parse_cmd_line(int argc, char **argv, run_time_settings_t &rt)
{
  try
  {
    // Used: abcdefgijkmnqrstuvwxy
    // Unused: p
    // Available: loz
    TCLAP::CmdLine cmd("Multi-sensor SVM Inference self-contained test utility", ' ', "0.4");

    TCLAP::ValueArg<uint16_t> verbosity_arg("v", "verbosity", "Verbosity level (0 is quiet)", false, 0, "non-negative integer");
    TCLAP::ValueArg<uint16_t> task_count_arg("t", "task_count", "Task count (1-16)", false, 1, "non-negative integer below 17");
    TCLAP::ValueArg<uint32_t> sensor_count_arg("s", "sensors", "Maximum number of sensors supported", true, 0, "positive integer");
    TCLAP::ValueArg<uint32_t> sample_count_arg("c", "samples", "Number of samples per sensor", true, 0, "positive integer");
    TCLAP::ValueArg<uint32_t> datagram_size_arg("d", "datagram", "Datagram size in bytes", true, 0, "positive integer");

    // TCLAP::ValueArg<uint32_t> port_num_arg("p", "port", "Port number of sensors supported", false, 8080, "positive integer");

    TCLAP::SwitchArg simulate_weights_arg("i", "sim_weights", "Simulate weights", false);
    TCLAP::ValueArg<std::string> weights_file_arg("w", "weights_file", "Path to weights file", false, "", "valid file path (relative or absolute)");
    TCLAP::ValueArg<uint32_t> weights_granularity_arg("g", "weights_div", "Divider for simulated weights", false, 1024, "positive integer");
    TCLAP::ValueArg<float> weights_min_arg("m", "weights_min", "Minimum simulated weight", false, -1.0, "real number");
    TCLAP::ValueArg<float> weights_max_arg("n", "weights_max", "Maximum simulated weight", false, 1.0, "real number");

    TCLAP::ValueArg<uint32_t> delay_arg("b", "delay", "Initial delay (ms)", false, 0, "non-negative integer");
    TCLAP::ValueArg<uint32_t> between_arg("e", "between", "Wait between operations (ms)", false, 100, "non-negative integer");
    TCLAP::ValueArg<uint32_t> repeats_arg("a", "repeats", "Number of times data set is repeated", false, 1, "non-negative integer");
    TCLAP::ValueArg<std::string> data_source_arg("u", "source", "Path to data source file", false, "", "valid file path (relative or absolute)");
    TCLAP::ValueArg<uint32_t> amplitude_granularity_arg("j", "ampl_div", "Divider for simulated amplitudes", false, 1024, "positive integer");
    TCLAP::ValueArg<float> amplitude_min_arg("q", "ampl_min", "Minimum simulated amplitude", false, -1.0, "real number");
    TCLAP::ValueArg<float> amplitude_max_arg("x", "ampl_max", "Maximum simulated amplitude", false, 1.0, "real number");

    TCLAP::SwitchArg skip_header_arg("k", "skip_header", "Skip report header line", false);
    TCLAP::ValueArg<std::string> report_file_arg("y", "report_file", "Path to report file", false, "", "valid file path (relative or absolute), - for cout");
    TCLAP::ValueArg<std::string> perf_file_arg("f", "perf_file", "Path to report file for perf data", false, "", "valid file path (relative or absolute), - for cout");

    cmd.add(verbosity_arg);
    cmd.add(task_count_arg);
    cmd.add(sensor_count_arg);
    cmd.add(sample_count_arg);
    cmd.add(datagram_size_arg);

    // cmd.add(port_num_arg);

    cmd.add(simulate_weights_arg);
    cmd.add(weights_file_arg);
    cmd.add(weights_granularity_arg);
    cmd.add(weights_min_arg);
    cmd.add(weights_max_arg);

    cmd.add(delay_arg);
    cmd.add(between_arg);
    cmd.add(repeats_arg);
    cmd.add(data_source_arg);
    cmd.add(amplitude_granularity_arg);
    cmd.add(amplitude_min_arg);
    cmd.add(amplitude_max_arg);

    cmd.add(skip_header_arg);
    cmd.add(report_file_arg);
    cmd.add(perf_file_arg);

    cmd.parse(argc, argv);

    rt.verbosity = verbosity_arg.getValue();
    rt.task_count = task_count_arg.getValue();
    rt.sensor_count = sensor_count_arg.getValue();
    rt.sample_count = sample_count_arg.getValue();
    rt.datagram_size = datagram_size_arg.getValue();
    // rt.port_num = port_num_arg.getValue();

    rt.simulate_weights = simulate_weights_arg.getValue();
    rt.weights_file = weights_file_arg.getValue();
    rt.weights_bounds = {weights_min_arg.getValue(), 
      weights_max_arg.getValue(), weights_granularity_arg.getValue()};

    // Transmitted data
    rt.delay_ms = delay_arg.getValue();
    rt.between_ms = between_arg.getValue();
    rt.repeats = repeats_arg.getValue();
    rt.data_source = data_source_arg.getValue();
    rt.amplitude_bounds = {amplitude_min_arg.getValue(), 
      amplitude_max_arg.getValue(), amplitude_granularity_arg.getValue()};

    rt.skip_header = skip_header_arg.getValue();
    rt.report_file = report_file_arg.getValue();
    rt.perf_file = perf_file_arg.getValue();

    rt.simulate_amplitudes = rt.simulate_weights;

    rt.exec_pattern = EXEC_PATTERN_BOTH;
    rt.exec_model = EXEC_MODEL_SEQ;

    rt.validate();

    return true;
  }
  catch (const TCLAP::ArgException &e)
  {
    std::cerr << "error: " << e.error() << " for arg " << e.argId() << std::endl;
    return false;
  }
  catch (const std::exception &ex)
  {
    std::cerr << "validation error: " << ex.what() << std::endl;
    return false;
  }
}

////////////////////////////////////////////////////////////////
// Data buffer
////////////////////////////////////////////////////////////////

/*
D = datagram size in data bytes
U = datagram unit count = D/(sizeof(data_item_t) = D/2)
The shared input buffer [U x N] looks like this:

Note that sensor id (k) & sequence id (s) are 32-bit, whereas
data points (x) are 16-bit.

-----------------------------------------------------------------
[0...nk-1 nk nk+1 nk+2     nk+3     nk+4          U-1
-----------------------------------------------------------------
[k0,      s0,     x00,     x01,     x02,     ..., x0(U-nk+3) ]
[k1,      s1,     x10,     x11,     x12,     ..., x1(U-nk+3) ]
[k2,      s2,     x20,     x21,     x22,     ..., x2(U-nk+3) ]
[...                                                         ]
-----------------------------------------------------------------

Each row of width U contains nk cells for a sensor ID,
two cells for a sequence number and U-(nk+2) cells of data.
The row is passed as the payload of a single datagram; thus
recv() can write the content directly to the next available
row of the buffer.

The buffer can also include a magic value for s:
(unsigned)(-1). This delivers a single commmand integer (s0)
as payload.

datagram_t can be used to access a record, e.g.:

  datagram_t* p = (datagram_t*)((char*)vector.data()
                         + (rt.datagram_size * row_index))

*/

////////////////////////////////////////////////////////////////
// Weights
////////////////////////////////////////////////////////////////

/*
The ID in columns 0-1 is passed to a lookup function which
executes some pointer chasing to retrieve the address of
the weights for the specific sensor.

This will be a prime beneficiary of prefetching.
*/

void create_sensor_ids(const run_time_settings_t &rt, std::vector<bpt_key_t> &sensor_ids)
{
  sensor_ids.clear();
  for (uint32_t i = 0; i < rt.sensor_count; i++)
  {
    bpt_key_t key;
    uuid::generate_uuid_v4_num(key.uid);
    sensor_ids.push_back(key);
  }
}

void create_sensor_btree(const run_time_settings_t &rt, std::vector<bpt_key_t> &sensor_ids,
  bpt_map_t& map)
{
  map.clear();
  // i is the data -> row number in weights array
  for (uint32_t i = 0; i < rt.sensor_count; i++) 
  {
    map[sensor_ids[i]] = i;
  }
}

// The weights are stored in a big array (like a file), and the tree contains
// an offset in the leaf node.
// This seems more realistic. It also creates one more "pointer" to chase.

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
void populate_weights_simulated(const run_time_settings_t &rt, const std::vector<bpt_key_t> &sensor_ids, std::vector<data_item_t> &weights)
{
  std::mt19937 engine; // Mersenne twister MT19937
  bounded_distribution<data_item_t> distribution(rt.weights_bounds);
  engine.seed(5432);

  auto rand_weights = [&]() { return distribution(engine); };
  weights.resize((rt.sv_len + 1) * rt.sensor_count);
  std::generate(weights.begin(), weights.end(), rand_weights);
  if (rt.verbosity >= 3)
  {
    dump_fp_vector(weights, std::cout, "weights");
  }
}

void populate_weights_stored(const run_time_settings_t &rt, const std::vector<bpt_key_t> &sensor_ids, std::vector<data_item_t> &weights)
{
  // TODO
}
#pragma GCC diagnostic pop

void populate_weights(const run_time_settings_t &rt, const std::vector<bpt_key_t> &sensor_ids, std::vector<data_item_t> &weights)
{
  if (rt.simulate_weights)
  {
    populate_weights_simulated(rt, sensor_ids, weights);
  }
  else
  {
    populate_weights_stored(rt, sensor_ids, weights);
  }
}

////////////////////////////////////////////////////////////////
// Runtime data
////////////////////////////////////////////////////////////////

class runtime_data
{
public:
  runtime_data(const run_time_settings_t &rt_)
      : rt(rt_)
  {
    #if MEASURE_LOCALITY==1
    locality_filestream.open("locality.csv", 
          std::ofstream::out | std::ofstream::app);
    #endif
  }

  // Fixed input data
  const run_time_settings_t &rt;
  std::vector<bpt_key_t> source_sensor_ids;
  bpt_map_t weights_map;

  // Weights - calculated once only
  std::vector<data_item_t> weights;

  // Dynamic data
  std::vector<data_vector_t> sensor_data;
  std::vector<id_t> seq_ids;
  std::vector<std::vector<result_t> > results;

  // Output
  std::ofstream report_filestream;
  std::ofstream perf_filestream;
  #if MEASURE_LOCALITY==1
  std::ofstream locality_filestream;
  #endif

  void initialise()
  {
    // Prepare work areas
    create_sensor_ids(rt, source_sensor_ids);
    create_sensor_btree(rt, source_sensor_ids, weights_map);

    // The sequence ID of each sensor tells us how many samples
    // have been received for that sensor so far
    seq_ids.resize(rt.sensor_count);
    std::fill(seq_ids.begin(), seq_ids.end(), 0);

    // The input data for each sensor is held in a continuous 
    // block of sample_count rows, each of width sv_len
    sensor_data.resize(rt.sensor_count);
    for (auto& v: sensor_data) {
      v.resize(rt.sample_count * rt.sv_len);
    }

    // There is one result for each sample of each sensor
    results.resize(rt.sensor_count);
    for (auto& v : results) {
      v.resize(rt.sample_count);
      std::fill(v.begin(), v.end(), false);
    }

    // Populate weights from storage or simulation
    populate_weights(rt, source_sensor_ids, weights);
  }

  inline const data_vector_t& resolve_x_vec(uint32_t sensor_index) const
  {
    return sensor_data[sensor_index];
  }
  inline const data_item_t *resolve_w(bpt_data_t sensor_index) const
  {
    return weights.data() + (rt.w_len * sensor_index);
  }
  inline std::vector<result_t>& resolve_results_vec(uint32_t sensor_index)
  {
    return results[sensor_index];
  }
  inline bool check_sensor_index(bpt_data_t sensor_index) const
  {
    if (sensor_index >= rt.sensor_count)
    {
      std::cerr << "Internal error: sensor id " << sensor_index << " is out of range" << std::endl;
      return false;
    }
    return true;
  }
  inline bool update_seq_id(bpt_data_t sensor_index, uint32_t seq_id, bool update)
  {
    uint32_t current_id = seq_ids[sensor_index];
    if (seq_id != current_id)
    {
      std::cerr << "Data error: sensor index " << sensor_index
                << " got seq_id " << seq_id
                << "; expected " << current_id << std::endl;
      return false;
    }
    if (update) {
      seq_ids[sensor_index]++;
    }
    return true;
  }
  bool save_input_data(const std::vector<data_item_t>& buffer)
  {
    const datagram_t *row_ptr = (const datagram_t *)buffer.data();
    bool ok = false;

    // Find sensor index from UUID
    auto itF = weights_map.find(row_ptr->sensor_id);
    uint32_t sensor_index = itF == weights_map.end() ? (uint32_t)-1 : itF->second;

    // Check data integrity & Match sequence IDs
    ok = check_sensor_index(sensor_index) 
      && update_seq_id(sensor_index, row_ptr->seq_id, true);
    
    if (ok)
    {
      // Identify the target block
      data_vector_t& vec = sensor_data[sensor_index];
      // Copy the SVM into the correct row of the block
      std::copy(row_ptr->data, row_ptr->data + rt.sv_len, vec.data() + (row_ptr->seq_id * rt.sv_len));
    }
    return ok;
  } 
  void reset_seq_ids() {
    std::fill(seq_ids.begin(), seq_ids.end(), 0);
  }
};


////////////////////////////////////////////////////////////////
// Performance event monitoring 
////////////////////////////////////////////////////////////////

#include <perf/perf_rec.h>
#include <perf/pe_summaries.h>

pem_test pem_summaries[EMI_COUNT];
long long pem_data[PEM_MAX_EVENTS];
int pem_statistic_count = 0;
intmax_t pem_duration;

void pem_init(int which_setup)
{
  pem_statistic_count = pem_setup(which_setup);
  ::pe_summaries_init(pem_statistic_count, pem_summaries);
}

template <typename FP_T>
void perf_record(size_t which_summary, FP_T fn) {
  ::perf_record(true, pem_summaries + which_summary, pem_duration, pem_data, fn);
}

void perf_report(std::ostream* os, int which_summary)
{
  if (os) {
    const pem_test* p = pem_summaries + which_summary;
    const char** names = pe_summary_get_names();
    *os << setw(9) << names[which_summary] << " pe stats : ";
    for (int i = 0; i < pem_statistic_count; i++) {
      *os << p->descriptors[i].name << "," << p->extract_value(i) << ",";
    }
    *os << std::endl;
  }
}

////////////////////////////////////////////////////////////////
// Locality
////////////////////////////////////////////////////////////////

#if MEASURE_LOCALITY==1
// This code records the addresses of consecutive nodes found in the tree
// and calculates the locality of the jumps between them
std::vector<long long> locality_jumps;
void record_locality(const void* n, const void* nprev)
{ 
  const char* pnp = static_cast<const char*>(nprev);
  const char* pn = static_cast<const char*>(n);
  locality_jumps.push_back(pn - pnp);
}
void clear_locality()
{
  locality_jumps.clear();
}
void report_locality(runtime_data& rt_data)
{
  std::ostream& os = rt_data.locality_filestream;
  for (size_t i = 0; i < locality_jumps.size(); i++)
  {
    if (i > 0) {
      os << ",";
    }
    os << locality_jumps[i];
  }
  os << std::endl;
}
#endif

////////////////////////////////////////////////////////////////
// SVM processing (coroutine)
////////////////////////////////////////////////////////////////

const char *x_next, *w_next;
char* result_next;

template <typename PREFETCHER_T>
static resumable infer_sensor_coro(const PREFETCHER_T &prefetcher, runtime_data &rt_data,
                                   size_t coroutine_index)
{
  bpt_data_t sensor_index = (bpt_data_t)coroutine_index;
  co_await CORO_STD::suspend_always{};

  const data_item_t *x, *w;

  size_t weights_size = rt_data.rt.w_len * sizeof(data_item_t);
  size_t weights_line_count = to_pf_line_count(weights_size);

  // Resolve weights & bias for this sensor
  w = rt_data.resolve_w(sensor_index);
  w_next = prefetcher.prefetch(reinterpret_cast<const char*>(w), weights_line_count);
  co_await CORO_STD::suspend_always{};

  // Inspect w data
  data_item_t bias = w[0];
  w++;

  // Get sensor data base
  const data_vector_t& x_vec = rt_data.resolve_x_vec(sensor_index);
  x = x_vec.data();
  auto row_len = rt_data.rt.sv_len;
  auto sample_count = x_vec.size() / row_len;
  size_t data_size = row_len * sizeof(data_item_t);
  size_t data_line_count = to_pf_line_count(data_size);

  // Get result base
  size_t results_size = sample_count * sizeof(result_t);
  size_t results_line_count = to_pf_line_count(results_size);
  std::vector<result_t>& results = rt_data.resolve_results_vec(sensor_index);
  result_t* result_ptr = results.data();
  result_next = prefetcher.prefetchw(reinterpret_cast<char*>(result_ptr), results_line_count);
  // TODO? do the output prefetch on a per-item basis inside the loop?

  for (uint32_t sample = 0; sample < sample_count; sample++, x += row_len, result_ptr++)
  {
    x_next = prefetcher.prefetch(reinterpret_cast<const char*>(x), data_line_count);
    co_await CORO_STD::suspend_always{};
    *result_ptr = svm_infer(w, x, bias, row_len) ? 1 : 0;
  }
}

#ifndef USE_GENERIC_COROUTINE_RUNNER
void run_infer_coroutine(runtime_data &rt_data)
{
  prefetch_true prefetcher;

  std::vector<resumable> tasks;
  std::vector<bool> done(rt_data.rt.task_count, false);
  size_t incomplete = rt_data.rt.sensor_count;

  for (size_t b = 0; b < (size_t)rt_data.rt.task_count; b++)
  {
    tasks.push_back(infer_sensor_coro(prefetcher, rt_data, b));
  }

  size_t next = (size_t)rt_data.rt.task_count;
  while (incomplete > 0)
  {
    for (size_t c = 0; c < tasks.size(); c++)
    {
      if (done[c])
      {
        continue;
      }
      resumable &t = tasks[c];
      if (t.is_complete())
      {
        if (next < rt_data.rt.sensor_count)
        {
          tasks[c] = infer_sensor_coro(prefetcher, rt_data, next);
          next++;
        }
        else
        {
          done[c] = true;
        }
        incomplete--;
      }
      else
      {
        t.resume();
      }
    }
  }
}
#endif

////////////////////////////////////////////////////////////////
// SVM processing (sequential)
////////////////////////////////////////////////////////////////

void infer_sensor_sequential(runtime_data &rt_data, uint32_t sensor_index)
{
  const data_item_t *x, *w;

  // Resolve weights & bias for this sensor
  w = rt_data.resolve_w(sensor_index);

  // Inspect w data
  data_item_t bias = w[0];
  w++;

  // Get sensor data base
  const data_vector_t& x_vec = rt_data.resolve_x_vec(sensor_index);
  x = x_vec.data();
  auto row_len = rt_data.rt.sv_len;
  auto sample_count = x_vec.size() / row_len;

  // Get result base
  std::vector<result_t>& results = rt_data.resolve_results_vec(sensor_index);
  result_t* result_ptr = results.data();

  for (uint32_t sample = 0; sample < sample_count; sample++, x += row_len, result_ptr++)
  {
    *result_ptr = svm_infer(w, x, bias, row_len) ? 1 : 0;
  }
}

void run_infer_sequential(runtime_data &rt_data)
{
  for (uint32_t i = 0; i < rt_data.rt.sensor_count; i++)
  {
    infer_sensor_sequential(rt_data, i);
  }
}

////////////////////////////////////////////////////////////////
// Reporting
////////////////////////////////////////////////////////////////

const char *model_names[] = {
    "sequential",
    "coroutine ",
    0};

std::ostream &get_output_stream(runtime_data &rt_data)
{
  if (rt_data.report_filestream.is_open())
  {
    return rt_data.report_filestream;
  }
  return std::cout;
}

std::string sep = ",";

void init_report_file(runtime_data &rt_data)
{
  if (!rt_data.rt.report_file.empty() && (rt_data.rt.report_file != "-"))
  {
    rt_data.report_filestream.open(rt_data.rt.report_file, std::ofstream::out | std::ofstream::app);
  }
}

void report_header(runtime_data &rt_data)
{
  if (rt_data.rt.exec_pattern == EXEC_PATTERN_BOTH)
  {
    get_output_stream(rt_data) << "sensors,samples,datagram,seq0,coro,seq1,ratio0,ratio1" << std::endl;
  }
  else
  {
    get_output_stream(rt_data) << "sensors,samples,datagram,model,time" << std::endl;
  }
}

void report_one(runtime_data &rt_data, int exec_model, NanoTimer::timeres_t process_time)
{
  get_output_stream(rt_data)
      << rt_data.rt.sensor_count << sep
      << rt_data.rt.sample_count << sep
      << rt_data.rt.datagram_size << sep
      << model_names[exec_model] << sep
      << process_time << std::endl;
}

void report_three(runtime_data &rt_data, const NanoTimer::timeres_t* spans, 
  size_t span_count, const float* ratios)
{
  if (span_count != 3) {
    return;
  }
  get_output_stream(rt_data)
      << rt_data.rt.sensor_count << sep
      << rt_data.rt.sample_count << sep
      << rt_data.rt.datagram_size << sep
      << spans[0] << sep
      << spans[1] << sep
      << spans[2] << sep
      << ratios[0] << sep
      << ratios[1] << std::endl;
}

void report_tree(const bpt_map_t& weights_map, std::ostream& os)
{
  os << "sizeof(bpt_key_t)=" << sizeof(bpt_key_t) << std::endl;
  os << "sizeof(void*)=" << sizeof(void*) << std::endl;
  os << "B+Tree traits: " << std::endl 
     << "  leaf_slots         =" << bpt_traits::leaf_slots << std::endl 
     << "  inner_slots        =" << bpt_traits::inner_slots << std::endl 
     << "  binsearch_threshold=" << bpt_traits::binsearch_threshold << std::endl;
  auto& stats = weights_map.get_stats();
  os << "B+Tree: " << std::endl 
     << "  size       =" << stats.size << std::endl 
     << "  nodes      =" << stats.nodes() << std::endl 
     << "  leaves     =" << stats.leaves << std::endl
     << "  inner_nodes=" << stats.leaves << std::endl
     << "  inner_slots=" << stats.inner_slots << std::endl 
     << "  leaf_slots =" << stats.leaf_slots << std::endl
     << "  avgfill    =" << stats.avgfill_leaves() << std::endl
     << std::endl;
}

std::ostream &get_perf_stream(runtime_data &rt_data)
{
  if (rt_data.perf_filestream.is_open())
  {
    return rt_data.perf_filestream;
  }
  return std::cout;
}

void init_perf_file(runtime_data &rt_data)
{
  if (!rt_data.rt.perf_file.empty() && (rt_data.rt.perf_file != "-"))
  {
    rt_data.perf_filestream.open(rt_data.rt.perf_file, std::ofstream::out | std::ofstream::trunc);
  }
}

void perf_header(runtime_data &rt_data)
{
  if (rt_data.rt.perf_file.empty()) {
    return;
  }
  get_perf_stream(rt_data) << "repeat,step,model,cpu_cycles,instructions,d_cache_reads,d_cache_misses" << std::endl;
}

void perf_line(runtime_data &rt_data, uint32_t iRepeat, int iModel, int exec_model)
{
  if (rt_data.rt.perf_file.empty()) {
    return;
  }
  std::ostream& os = get_perf_stream(rt_data);
  os << iRepeat << sep << iModel << sep << exec_model;
  const pem_test* p = pem_summaries + iModel;
  for (int i = 0; i < pem_statistic_count; i++) {
    os << sep << p->extract_value(i);
  }
  os << std::endl;
}

void sys_wait_us(unsigned long wait_us) 
{
  // Only use this on the Pi
  #ifndef WIRINGPI_MOCK
  struct timespec req = { 0, (__syscall_slong_t)wait_us * 1000 };
  clock_nanosleep(CLOCK_MONOTONIC, 0, &req, NULL); 
  #else
  wait_us++;        
  #endif
}

/**
 * @brief Entry point
 *
 * @param argc The command line argument count
 * @param argv The command line arguments
 * @return int 0 for successful execution, non-zero for error
 */
int main(int argc, char **argv)
{
  struct run_time_settings_t rt;

  if (!parse_cmd_line(argc, argv, rt))
  {
    // cmd line error or just show usage
    return -1;
  }

  if (rt.delay_ms > 0)
  {
    wait_us(1000 * rt.delay_ms);
  }

  if (rt.verbosity >= 2)
  {
    rt.dump(std::cout);
  }

  if (!rt.simulate_weights)
  {
    std::cerr << "Only simulated weights are supported at this time\r\n";
    return 1;
  }

  // Init perf subsystem
  pem_init(0);

  // Init global time
  NanoTimer timer;

  // Init GPIOs
  gpio the_gpio(rt.verbosity);
  the_gpio.init();
  the_gpio.set(0, false);
  the_gpio.set(1, false);

  // Prepare work areas
  runtime_data rt_data(rt);
  rt_data.initialise();
  init_report_file(rt_data);
  init_perf_file(rt_data);

  if (rt.verbosity >= 2) 
  {
    report_tree(rt_data.weights_map, std::cout);
  }
  if (rt.verbosity >= 4) 
  {
    // TODO dump tree    
  }

  // Prepare to receive input
  std::unique_ptr<input_receiver> receiver;
  if (rt.simulate_amplitudes) 
  {
    receiver = std::make_unique<input_simulator>(
      rt_data.source_sensor_ids, 
      rt.sample_count, 
      rt.datagram_size, 
      rt.amplitude_bounds);
  }
  else
  {
    std::cerr << "Only simulated input is supported at this time\r\n";
    return 1;
  }

  // Performance data
  // NanoTimer::timeres_t last_coro_time = 0;
  float ratio_totals[2] = { 0.0, 0.0 };
  long ratio_count = 0;

  if (!rt.skip_header && (rt.verbosity > 0))
  {
    report_header(rt_data);
  }
  perf_header(rt_data); // Ignored if perf_file.empty()

  #ifdef USE_GENERIC_COROUTINE_RUNNER
  prefetch_true prefetcher;
  coroutine_runner<prefetch_true, runtime_data, std::resumable> runner_with_prefetch(prefetcher, rt_data);
  #endif

  // Run rt.repeats times
  for (uint32_t iRepeat = 0; iRepeat < rt.repeats; iRepeat++) 
  {
    receiver->reset();
    rt_data.reset_seq_ids();

    // Collect and organise input
    std::vector<data_item_t> input_buffer;
    while (receiver->get_next_input(input_buffer))
    {
      if (!rt_data.save_input_data(input_buffer)) {
        std::cerr << "Faulty input received\r\n";
        return 2;
      }
    }
    if (receiver->stop_requested())
    {
      break;
    }

    // Infer all rows
    if (rt.exec_pattern == EXEC_PATTERN_BOTH)
    {
      ::clear_cache();
      NanoTimer::timeres_t spans[3];
      // Run 3 models as follows:
      //  EXEC_MODEL_SEQ, EXEC_MODEL_CORO, EXEC_MODEL_SEQ
      for (int iModel = 0; iModel < 3; iModel++)
      {
        // Wait 1/10th second to clarify any hysteresis on the power use
        sys_wait_us(1000 * rt.between_ms);

        int exec_model = (iModel == 1) ? EXEC_MODEL_CORO : EXEC_MODEL_SEQ;
        the_gpio.set(exec_model, true);
        auto started_at = timer.get_timestamp();
        //start perf_record
        perf_record(iModel, [&exec_model, &rt_data, &runner_with_prefetch]()
        {
          if (exec_model == EXEC_MODEL_SEQ)
          {
            run_infer_sequential(rt_data);
          }
          else
          {
            #ifndef USE_GENERIC_COROUTINE_RUNNER
            run_infer_coroutine(rt_data);
            #else
            runner_with_prefetch.run(rt_data.rt.task_count, rt_data.rt.sensor_count, infer_sensor_coro);
            #endif
          }
        });
        //end perf_record
        auto finished_at = timer.get_timestamp();
        the_gpio.set(exec_model, false);
        spans[iModel] = finished_at - started_at;
        perf_line(rt_data, iRepeat, iModel, exec_model);

        if (rt.verbosity > 1)
        {
          for(size_t i = 0; i < rt_data.results.size(); i++)
          {
            dump_vector(rt_data.results[i], std::cout, "results " + std::to_string(i));
          }
        }
        if (rt.verbosity > 1)
        {
          perf_report(&std::cout, iModel);
        }

        if (iModel == 2) {
          float ratios[2] = { (float)spans[0]/(float)spans[1], (float)spans[2]/(float)spans[1] };
          if (rt.verbosity > 0)
          {
            report_three(rt_data, spans, 3, ratios);
          }
          ratio_count++;
          ratio_totals[0] += ratios[0];
          ratio_totals[1] += ratios[1];
        }
      }
      sys_wait_us(1000 * rt.between_ms);
    }
    else
    {
      the_gpio.set(rt.exec_model, true);
      auto started_at = timer.get_timestamp();
      //start perf_record
      if (rt.exec_model == EXEC_MODEL_SEQ)
      {
        run_infer_sequential(rt_data);
      }
      else
      {
        runner_with_prefetch.run(rt_data.rt.task_count, rt_data.rt.sensor_count, infer_sensor_coro);
      }
      //end perf_record
      auto finished_at = timer.get_timestamp();
      the_gpio.set(rt.exec_model, false);
      if (rt.verbosity > 0)
      {
        report_one(rt_data, rt.exec_model, finished_at - started_at);
      }
      if (rt.verbosity > 1)
      {
        for (const auto& v : rt_data.results) {
          dump_vector(v, std::cout, "results");
        }
      }
    }
  }

  if (rt.verbosity >= 1)
  {
    if (rt.exec_pattern == EXEC_PATTERN_BOTH) {
    }
    std::cout << "Server closing now" << std::endl;
    std::cout << "sensors," << rt.sensor_count
              << ",samples," << rt.sample_count 
              << ",datagram," << rt.datagram_size
              << ",tasks," << rt.task_count
              << ",ratio0," << ratio_totals[0]/(float)ratio_count 
              << ",ratio1," << ratio_totals[1]/(float)ratio_count 
              << std::endl;
  }
  return 0;
}
