#include "snapshot.h"

#include "utils.h"

using namespace md;

SnapshotWrapper::SnapshotWrapper(const u_char *raw_data) {
  const int max_level_num = 10;
  bid_book_.resize(max_level_num + 1);
  ask_book_.resize(max_level_num + 1);

  header_ = reinterpret_cast<const SnapshotHeader *>(raw_data);

  security_id = bytes_to_str(header_->security_id, 8);
  orig_time = header_->orig_time();
  total_trade_num = header_->total_trade_num();
  total_trade_volume = header_->total_trade_volume();
  total_trade_value = header_->total_trade_value();
}

void SnapshotWrapper::init_md_entries() {
  assert(raw_data != nullptr);
  const u_char *entries =
      reinterpret_cast<const u_char *>(header_) + sizeof(SnapshotHeader);
  for (uint32_t i = 0; i < header_->md_entry_num(); i++) {
    const auto *md_entry = reinterpret_cast<const MarketDataEntry *>(entries);
    // Debug
    // std::cout << "start process " << i << "th (total " <<
    // header->md_entry_num()
    //           << ") md entry with type: " << md_entry->be_md_entry_type <<
    //           '\n';
    int offset = parse_extended_fields(*md_entry);
    entries += offset;
  }
}

int SnapshotWrapper::parse_extended_fields(const MarketDataEntry &md_entry) {
  switch (md_entry.md_entry_type()) {
  case MdEntryType::Open:
    open_price = md_entry.price();
    break;

  case MdEntryType::Latest:
    latest_trade_price = md_entry.price();
    break;

  case MdEntryType::Buy:
    bid_book_[md_entry.price_level()].price = md_entry.price();
    bid_book_[md_entry.price_level()].quantity = md_entry.quantity();
    break;

  case MdEntryType::Sell:
    ask_book_[md_entry.price_level()].price = md_entry.price();
    ask_book_[md_entry.price_level()].quantity = md_entry.quantity();
    break;

  default:
    // ignored
    break;
  }
  // ignore per-order quantity information
  return sizeof(MarketDataEntry) + md_entry.number_of_quantity_awared_orders() *
                                       sizeof(md_entry.quantity());
}
