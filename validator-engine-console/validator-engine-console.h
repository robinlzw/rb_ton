#pragma once
#include "adnl/adnl-ext-client.h"
#include "tl-utils/tl-utils.hpp"
#include "ton/ton-types.h"
#include "terminal/terminal.h"
#include "vm/cells.h"
#include "validator-engine-console-query.h"

#include <map>

class ValidatorEngineConsole : public td::actor::Actor {
 private:
  td::actor::ActorOwn<ton::adnl::AdnlExtClient> client_;
  td::actor::ActorOwn<td::TerminalIO> io_;

  bool readline_enabled_ = true;

  td::IPAddress remote_addr_;
  ton::PrivateKey private_key_;
  ton::PublicKey server_public_key_;

  bool ready_ = false;
  bool inited_ = false;

  td::Timestamp fail_timeout_;
  bool ex_mode_ = false;
  std::vector<td::BufferSlice> ex_queries_;

  std::unique_ptr<ton::adnl::AdnlExtClient::Callback> make_callback();

  std::map<std::string, std::unique_ptr<QueryRunner>> query_runners_;
  void add_query_runner(std::unique_ptr<QueryRunner> runner) {
    std::cout << "00 runner = " << runner.get() << std::endl;
    if (runner)
    {
      // static std::string name = runner->name();
      // if (query_runners_.find(name) == query_runners_.end())
      // {
      //   std::cout << "name = " << name << std::endl;
      //   query_runners_[name] = std::move(runner);
      //   std::cout << "runner = " << runner.get() << std::endl;
      //   std::cout << "query_runners_[name] = " << query_runners_[name].get() << std::endl;
      // }
      
      query_runners_[runner->name()] = std::move(runner);
      for (const auto & item : query_runners_)
      {
        std::cout << "item[" << item.first << "] = " << item.second.get() << std::endl;
      }
       
    }
  }

 public:
  void conn_ready() {
    td::TerminalIO::out() << "conn ready\n";
    std::cout << "conn ready\n";
    ready_ = true;
    got_result();
  }
  void conn_closed() {
    td::TerminalIO::out() << "conn failed\n";
    ready_ = false;
  }
  void set_readline_enabled(bool value) {
    readline_enabled_ = value;
  }
  void set_remote_addr(td::IPAddress addr) {
    remote_addr_ = addr;
  }
  void set_private_key(td::BufferSlice file_name);
  void set_public_key(td::BufferSlice file_name);

  void add_cmd(td::BufferSlice data) {
    std::cout << "add_cmd done\n";
    ex_mode_ = true;
    ex_queries_.push_back(std::move(data));
    set_readline_enabled(false);
    std::cout << "ex_queries_.size() = " << ex_queries_.size() << std::endl;
    if (ready_)
    {
      got_result();
    }
  }
  void set_fail_timeout(td::Timestamp ts) {
    fail_timeout_ = ts;
    alarm_timestamp().relax(fail_timeout_);
  }
  void alarm() override {
    if (fail_timeout_.is_in_past()) {
      std::_Exit(7);
    }
    if (ex_mode_ && ex_queries_.size() == 0) {
      std::_Exit(0);
    }
    alarm_timestamp().relax(fail_timeout_);
  }

  void close() {
    stop();
  }
  void tear_down() override {
    // FIXME: do not work in windows
    //td::actor::SchedulerContext::get()->stop();
    io_.reset();
    std::_Exit(0);
  }

  bool envelope_send_query(td::BufferSlice query, td::Promise<td::BufferSlice> promise);
  void got_result(bool success = true);
  void show_help(std::string command, td::Promise<td::BufferSlice> promise);
  void show_license(td::Promise<td::BufferSlice> promise);

  void parse_line(td::BufferSlice data);
  ValidatorEngineConsole() {
  }

  void run();
};
