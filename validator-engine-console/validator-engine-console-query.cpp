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
  // auto b = ton::create_serialize_tl_object<ton::ton_api::engine_validator_getOverlaysStats>();
  // td::actor::send_closure(console_, &ValidatorEngineConsole::envelope_send_query, std::move(b), create_promise());
  std::cout << "lll >> GetOverlaysStatsQuery" << std::endl;
  if (query_msg_.length() != 0 && query_msg_.c_str() != nullptr)
  {
    std::cout << "query_msg_.length() = " << query_msg_.length() << std::endl;
    auto query_str = query_msg_.c_str();
    std::cout << "query_str = " << query_str << std::endl;
    std::cout << "strlen(query_str) = " << strlen(query_str) << std::endl;
    std::cout << "query_msg_ = " << query_msg_ << std::endl;
    td::actor::send_closure(console_, &ValidatorEngineConsole::envelope_send_query, td::BufferSlice(query_str, strlen(query_str)), create_promise());
  }
  std::cout << "lll << GetOverlaysStatsQuery" << std::endl;
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
