#pragma once

// map is faster for small data set
#include <cassert>
#include <map>
#include <unordered_map>

namespace md {

// returns whether it's a new market data
class MdArbitrator {
public:
  bool record_order_or_trade(uint16_t channel_id, uint64_t appl_seq_num) {
    if (appl_seq_num_recorder_[channel_id] >= appl_seq_num) {
      return false;
    }
    appl_seq_num_recorder_[channel_id] = appl_seq_num;
    return true;
  }

  // for snapshot
  bool record_snapshot(const std::string &stock, int64_t exchange_time) {
    if (exchange_time_recorder_[stock] >= exchange_time) {
      return false;
    }
    exchange_time_recorder_[stock] = exchange_time;
    return true;
  }

private:
  // key: channel id, value: last seen appl_seq_num
  // appl_seq_num shall be unique for each channel
  // Assumption: we shall not receive out-of-order appl_seq_num
  // shared by order and trade
  std::map<uint16_t, uint64_t> appl_seq_num_recorder_;

  // key: stock id, value: exchange timestamp
  // stock id shall be unique in each snapshot
  // Assumption: exchange time shall be the same for all stocks in a snapshot
  std::unordered_map<std::string, int64_t> exchange_time_recorder_;
};
} // namespace md
