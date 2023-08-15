#pragma once

#include "td/utils/Status.h"
#include "td/utils/buffer.h"
#include "td/utils/misc.h"
#include "td/utils/SharedSlice.h"
#include "td/utils/port/IPAddress.h"
#include "td/actor/actor.h"
#include "ton/ton-types.h"

#include "keys/keys.hpp"

class ValidatorEngineConsole;

class QueryRunner {
 public:
  virtual ~QueryRunner() {
    std::cout << "~QueryRunner()" << std::endl;
  };
  virtual std::string name() const = 0;
  virtual std::string help() const = 0;
  virtual td::Status run(td::actor::ActorId<ValidatorEngineConsole> console, const std::string query_msg) const = 0;
};

template <class T>
class QueryRunnerImpl : public QueryRunner {
 public:
  std::string name() const override {
    return T::get_name();
  }
  std::string help() const override {
    return T::get_help();
  }
  td::Status run(td::actor::ActorId<ValidatorEngineConsole> console, const std::string query_msg) const override {
    td::actor::create_actor<T>(PSTRING() << "query " << name(), std::move(console), query_msg).release();
    return td::Status::OK();
  }
  QueryRunnerImpl() {
  }
};

class Query : public td::actor::Actor {
 public:
  virtual ~Query() = default;
  Query(td::actor::ActorId<ValidatorEngineConsole> console, const std::string query_msg)
      : console_(console), query_msg_(query_msg) {
  }
  void start_up() override;
  virtual td::Status run() = 0;
  virtual td::Status send() = 0;
  void receive_wrap(td::BufferSlice R);
  virtual td::Status receive(td::BufferSlice R) = 0;
  auto create_promise() {
    return td::PromiseCreator::lambda([SelfId = actor_id(this)](td::Result<td::BufferSlice> R) {
      if (R.is_error()) {
        td::actor::send_closure(SelfId, &Query::handle_error, R.move_as_error());
      } else {
        td::actor::send_closure(SelfId, &Query::receive_wrap, R.move_as_ok());
      }
    });
  }
  virtual std::string name() const = 0;
  void handle_error(td::Status error);

  static std::string time_to_human(int unixtime) {
    char time_buffer[80];
    time_t rawtime = unixtime;
    struct tm tInfo;
#if defined(_WIN32) || defined(_WIN64)
    struct tm* timeinfo = localtime_s(&tInfo, &rawtime) ? nullptr : &tInfo;
#else
    struct tm* timeinfo = localtime_r(&rawtime, &tInfo);
#endif
    assert(timeinfo == &tInfo);
    strftime(time_buffer, 80, "%c", timeinfo);
    return std::string(time_buffer);
  }

 protected:
  td::actor::ActorId<ValidatorEngineConsole> console_;
  const std::string &query_msg_;
};

class GetOverlaysStatsQuery : public Query {
 public:
  GetOverlaysStatsQuery(td::actor::ActorId<ValidatorEngineConsole> console, const std::string &query_msg)
      : Query(console, query_msg) {
  }
  td::Status run() override;
  td::Status send() override;
  td::Status receive(td::BufferSlice data) override;
  static std::string get_name() {
    return "getoverlaysstats";
  }
  static std::string get_help() {
    return "getoverlaysstats\tgets stats for all overlays";
  }
  std::string name() const override {
    return get_name();
  }
};
