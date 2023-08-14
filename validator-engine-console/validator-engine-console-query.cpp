#include "validator-engine-console-query.h"
#include "auto/tl/ton_api.h"
#include "td/utils/StringBuilder.h"
#include "validator-engine-console.h"
#include "terminal/terminal.h"
#include "td/utils/filesystem.h"
#include "overlay/overlays.h"
#include "ton/ton-tl.hpp"

#include <cctype>
#include <fstream>

Tokenizer::Tokenizer(td::BufferSlice data) : data_(std::move(data)) {
  remaining_ = data_.as_slice();
}

void Tokenizer::skipspc() {
  while (remaining_.size() > 0 && std::isspace(remaining_[0])) {
    remaining_.remove_prefix(1);
  }
}

bool Tokenizer::endl() {
  skipspc();
  return remaining_.size() == 0;
}

td::Result<td::Slice> Tokenizer::get_raw_token() {
  skipspc();
  if (remaining_.size() == 0) {
    return td::Status::Error("failed to parse token: EOL");
  }
  size_t idx = 0;
  while (idx < remaining_.size() && !std::isspace(remaining_[idx])) {
    idx++;
  }
  auto r = remaining_.copy().truncate(idx);
  remaining_.remove_prefix(idx);
  return r;
}

td::Result<td::Slice> Tokenizer::peek_raw_token() {
  skipspc();
  if (remaining_.size() == 0) {
    return td::Status::Error("failed to parse token: EOL");
  }
  size_t idx = 0;
  while (idx < remaining_.size() && !std::isspace(remaining_[idx])) {
    idx++;
  }
  auto r = remaining_.copy().truncate(idx);
  return r;
}

void Query::start_up() {
  std::cout << " Query::start_up()\n";
  auto R = [&]() -> td::Status {
    TRY_STATUS(run());
    TRY_STATUS(send());
    return td::Status::OK();
  }();
  if (R.is_error()) {
    handle_error(std::move(R));
  }
}

void Query::handle_error(td::Status error) {
  td::TerminalIO::out() << "Failed " << name() << " query: " << error << "\n";
  td::actor::send_closure(console_, &ValidatorEngineConsole::got_result, false);
  stop();
}

void Query::receive_wrap(td::BufferSlice R) {
  auto S = receive(std::move(R));
  if (S.is_error()) {
    handle_error(std::move(S));
  } else {
    td::actor::send_closure(console_, &ValidatorEngineConsole::got_result, true);
    stop();
  }
}

td::Status GetOverlaysStatsQuery::run() {
  return td::Status::OK();
}

td::Status GetOverlaysStatsQuery::send() {
  std::cout << "GetOverlaysStatsQuery::send()\n";
  // auto b = ton::create_serialize_tl_object<ton::ton_api::engine_validator_getOverlaysStats>();
  // td::actor::send_closure(console_, &ValidatorEngineConsole::envelope_send_query, std::move(b), create_promise());
  auto name = tokenizer_.get_token<std::string>().move_as_ok();
  td::actor::send_closure(console_, &ValidatorEngineConsole::envelope_send_query, td::BufferSlice(name.c_str(), name.length()), create_promise());
  return td::Status::OK();
}

td::Status GetOverlaysStatsQuery::receive(td::BufferSlice data) {
  TRY_RESULT_PREFIX(f, ton::fetch_tl_object<ton::ton_api::engine_validator_overlaysStats>(data.as_slice(), true),
                    "received incorrect answer: ");
  for (auto &s : f->overlays_) {
    td::StringBuilder sb;
    sb << "overlay_id: " << s->overlay_id_ << " adnl_id: " << s->adnl_id_ << " scope: " << s->scope_ << "\n";
    sb << "  nodes:\n";
    
    td::uint32 overlay_t_out_bytes = 0;
    td::uint32 overlay_t_out_pckts = 0;
    td::uint32 overlay_t_in_bytes = 0;
    td::uint32 overlay_t_in_pckts = 0;
    
    for (auto &n : s->nodes_) {
      sb << "   adnl_id: " << n->adnl_id_ << " ip_addr: " << n->ip_addr_ << " broadcast_errors: " << n->bdcst_errors_ << " fec_broadcast_errors: " << n->fec_bdcst_errors_ << " last_in_query: " << n->last_in_query_ << " (" << time_to_human(n->last_in_query_) << ")" << " last_out_query: " << n->last_out_query_ << " (" << time_to_human(n->last_out_query_) << ")" << "\n   throughput:\n    out: " << n->t_out_bytes_ << " bytes/sec, " << n->t_out_pckts_ << " pckts/sec\n    in: " << n->t_in_bytes_ << " bytes/sec, " << n->t_in_pckts_ << " pckts/sec\n";
      
      overlay_t_out_bytes += n->t_out_bytes_;
      overlay_t_out_pckts += n->t_out_pckts_;
      
      overlay_t_in_bytes += n->t_in_bytes_;
      overlay_t_in_pckts += n->t_in_pckts_;
    }
    sb << "  total_throughput:\n   out: " << overlay_t_out_bytes << " bytes/sec, " << overlay_t_out_pckts << " pckts/sec\n   in: " << overlay_t_in_bytes << " bytes/sec, " << overlay_t_in_pckts << " pckts/sec\n";
     
    sb << "  stats:\n";
    for (auto &t : s->stats_) {
      sb << "    " << t->key_ << "\t" << t->value_ << "\n";
    }
    td::TerminalIO::output(sb.as_cslice());
  }
  return td::Status::OK();
}
