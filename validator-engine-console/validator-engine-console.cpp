#include "validator-engine-console.h"
#include "adnl/adnl-ext-client.h"
#include "tl-utils/lite-utils.hpp"
#include "auto/tl/ton_api_json.h"
#include "auto/tl/lite_api.h"
#include "td/utils/OptionParser.h"
#include "td/utils/Time.h"
#include "td/utils/filesystem.h"
#include "td/utils/format.h"
#include "td/utils/Random.h"
#include "td/utils/crypto.h"
#include "td/utils/port/signals.h"
#include "td/utils/port/stacktrace.h"
#include "td/utils/port/StdStreams.h"
#include "td/utils/port/FileFd.h"
#include "terminal/terminal.h"
#include "ton/lite-tl.hpp"
#include "block/block-db.h"
#include "block/block.h"
#include "block/block-parse.h"
#include "block/block-auto.h"
#include "block/mc-config.h"
#include "block/check-proof.h"
#include "vm/boc.h"
#include "vm/cellops.h"
#include "vm/cells/MerkleProof.h"
#include "ton/ton-shard.h"

#if TD_DARWIN || TD_LINUX
#include <unistd.h>
#include <fcntl.h>
#endif
#include <iostream>
#include <sstream>
#include "git.h"

int verbosity;

std::unique_ptr<ton::adnl::AdnlExtClient::Callback> ValidatorEngineConsole::make_callback() {
  class Callback : public ton::adnl::AdnlExtClient::Callback {
   public:
    void on_ready() override {
      std::cout << "ValidatorEngineConsole on_ready\n";
      td::actor::send_closure(id_, &ValidatorEngineConsole::conn_ready);
    }
    void on_stop_ready() override {
      td::actor::send_closure(id_, &ValidatorEngineConsole::conn_closed);
    }
    Callback(td::actor::ActorId<ValidatorEngineConsole> id) : id_(std::move(id)) {
    }

   private:
    td::actor::ActorId<ValidatorEngineConsole> id_;
  };
  return std::make_unique<Callback>(actor_id(this));
}

void ValidatorEngineConsole::run() {
  client_ = ton::adnl::AdnlExtClient::create(ton::adnl::AdnlNodeIdFull{server_public_key_}, private_key_, remote_addr_,
                                             make_callback());
  auto overlays_stats_query_ptr = std::make_unique<QueryRunnerImpl<GetOverlaysStatsQuery>>();
  std::unique_ptr<QueryRunner> query_runner_ptr = std::move(overlays_stats_query_ptr);
  add_query_runner(std::move(query_runner_ptr));
  std::cout << "<< ValidatorEngineConsole::run()\n";
}

bool ValidatorEngineConsole::envelope_send_query(td::BufferSlice query, td::Promise<td::BufferSlice> promise) {
  std::cout << "lll >> envelope_send_query" << std::endl;
  if (!ready_ || client_.empty()) {
    promise.set_error(td::Status::Error(ton::ErrorCode::notready, "failed to send query to server: not ready"));
    return false;
  }
  auto P = td::PromiseCreator::lambda([promise = std::move(promise)](td::Result<td::BufferSlice> R) mutable {
    if (R.is_error()) {
      promise.set_error(R.move_as_error());
      return;
    }
    auto data = R.move_as_ok();
    auto F = ton::fetch_tl_object<ton::ton_api::engine_validator_controlQueryError>(data.clone(), true);
    if (F.is_ok()) {
      auto f = F.move_as_ok();
      promise.set_error(td::Status::Error(f->code_, f->message_));
      return;
    }
    std::cout << "-------> get_query_result = " << data.data() << std::endl << std::endl;
    promise.set_result(std::move(data));
  });

  td::BufferSlice b = ton::serialize_tl_object(
      ton::create_tl_object<ton::ton_api::engine_validator_controlQuery>(std::move(query)), true);
  std::cout << " ValidatorEngineConsole::envelope_send_query client_ send_query" << std::endl;
  td::actor::send_closure(client_, &ton::adnl::AdnlExtClient::send_query, "query", std::move(b),
                          td::Timestamp::in(1.0), std::move(P));
  return true;
}

void ValidatorEngineConsole::got_result(bool success) {
  std::cout << "got_result" << std::endl;
  if (!success && ex_mode_) {
    std::_Exit(2);
  }
  if (ex_queries_.size() > 0) {
    auto data = std::move(ex_queries_[0]);
    ex_queries_.erase(ex_queries_.begin());
    parse_line(std::move(data));
    sleep(4);
  }
  if (ex_mode_ && ex_queries_.size() == 0) {
    std::_Exit(0);
  }
}

void ValidatorEngineConsole::show_help(std::string command, td::Promise<td::BufferSlice> promise) {
  if (command.size() == 0) {
    td::TerminalIO::out() << "list of available commands:\n";
    for (auto& cmd : query_runners_) {
      td::TerminalIO::out() << cmd.second->help() << "\n";
    }
  } else {
    auto it = query_runners_.find(command);
    if (it != query_runners_.end()) {
      td::TerminalIO::out() << it->second->help() << "\n";
    } else {
      td::TerminalIO::out() << "unknown command '" << command << "'\n";
    }
  }
  promise.set_value(td::BufferSlice{});
}

void ValidatorEngineConsole::show_license(td::Promise<td::BufferSlice> promise) {
  td::TerminalIO::out() << R"(Copyright)" << "\n";
  promise.set_value(td::BufferSlice{});
}

void ValidatorEngineConsole::parse_line(td::BufferSlice data) {
  for (const auto &item : query_runners_)
  {
    item.second->run(actor_id(this), data.data());
  }
}

void ValidatorEngineConsole::set_private_key(td::BufferSlice file_name) {
  auto R = [&]() -> td::Result<ton::PrivateKey> {
    TRY_RESULT_PREFIX(conf_data, td::read_file("/home/lzw/dataset/myLocalTon-dht/genesis/bin/certs/client"), "failed to read: ");
    return ton::PrivateKey::import(conf_data.as_slice());
  }();

  if (R.is_error()) {
    LOG(FATAL) << "bad private key: " << R.move_as_error();
  }
  private_key_ = R.move_as_ok();
}

void ValidatorEngineConsole::set_public_key(td::BufferSlice file_name) {
  auto R = [&]() -> td::Result<ton::PublicKey> {
    TRY_RESULT_PREFIX(conf_data, td::read_file("/home/lzw/dataset/myLocalTon-dht/genesis/bin/certs/server.pub"), "failed to read: ");

    return ton::PublicKey::import(conf_data.as_slice());
  }();

  if (R.is_error()) {
    LOG(FATAL) << "bad server public key: " << R.move_as_error();
  }
  server_public_key_ = R.move_as_ok();
}

int main() {  
  SET_VERBOSITY_LEVEL(verbosity_INFO);
  td::actor::ActorOwn<ValidatorEngineConsole> x;
  td::actor::Scheduler scheduler({8});
  auto sch = td::thread([&] { scheduler.run(); });
  scheduler.run_in_context([&] {
    x = td::actor::create_actor<ValidatorEngineConsole>("console");
    td::IPAddress addr;
    addr.init_host_port("localhost:4441");
    td::actor::send_closure(x, &ValidatorEngineConsole::set_remote_addr, addr);

    td::actor::send_closure(x, &ValidatorEngineConsole::set_private_key, 
      td::BufferSlice{"/home/lzw/dataset/myLocalTon-dht/genesis/bin/certs/client"});
    td::actor::send_closure(x, &ValidatorEngineConsole::set_public_key, 
      td::BufferSlice{"/home/lzw/dataset/myLocalTon-dht/genesis/bin/certs/server.pub"});

    std::cout << "x.id = " << x.get().actor_info().get_name().c_str() << std::endl;
    td::actor::send_closure(x, &ValidatorEngineConsole::run);
  });

  scheduler.run_in_context([&] {
  });

  // scheduler.run();
  sch.join();
  td::actor::send_closure(x, &ValidatorEngineConsole::add_cmd, td::BufferSlice{"xxx"});
  return 0;
}
