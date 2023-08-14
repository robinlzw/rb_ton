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

class Tokenizer {
 public:
  Tokenizer(td::BufferSlice data);

  void skipspc();
  bool endl();
  td::Status check_endl() {
    if (!endl()) {
      return td::Status::Error("extra data after query");
    } else {
      return td::Status::OK();
    }
  }

  td::Result<td::Slice> get_raw_token();
  td::Result<td::Slice> peek_raw_token();

  template <typename T>
  inline td::Result<T> get_token() {
    TRY_RESULT(S, get_raw_token());
    return td::to_integer_safe<T>(S);
  }
  template <typename T>
  inline td::Result<std::vector<T>> get_token_vector();

 private:
  td::BufferSlice data_;
  td::Slice remaining_;
};

template <>
inline td::Result<td::Slice> Tokenizer::get_token() {
  return get_raw_token();
}

template <>
inline td::Result<std::string> Tokenizer::get_token() {
  TRY_RESULT(S, get_raw_token());
  return S.str();
}

template <>
inline td::Result<td::BufferSlice> Tokenizer::get_token() {
  TRY_RESULT(S, get_raw_token());
  TRY_RESULT(F, td::hex_decode(S));
  return td::BufferSlice{F};
}

template <>
inline td::Result<td::SharedSlice> Tokenizer::get_token() {
  TRY_RESULT(S, get_raw_token());
  TRY_RESULT(F, td::hex_decode(S));
  return td::SharedSlice{F};
}

template <>
inline td::Result<ton::PublicKeyHash> Tokenizer::get_token() {
  TRY_RESULT(S, get_raw_token());
  TRY_RESULT(F, td::hex_decode(S));
  if (F.size() == 32) {
    return ton::PublicKeyHash{td::Slice{F}};
  } else {
    return td::Status::Error("cannot parse keyhash: bad length");
  }
}

template <>
inline td::Result<td::Bits256> Tokenizer::get_token() {
  TRY_RESULT(S, get_raw_token());
  TRY_RESULT(F, td::hex_decode(S));
  if (F.size() == 32) {
    td::Bits256 v;
    v.as_slice().copy_from(F);
    return v;
  } else {
    return td::Status::Error("cannot parse keyhash: bad length");
  }
}

template <>
inline td::Result<td::IPAddress> Tokenizer::get_token() {
  TRY_RESULT(S, get_raw_token());
  td::IPAddress addr;
  TRY_STATUS(addr.init_host_port(S.str()));
  return addr;
}

template <typename T>
inline td::Result<std::vector<T>> Tokenizer::get_token_vector() {
  TRY_RESULT(word, get_token<std::string>());
  if (word != "[") {
    return td::Status::Error("'[' expected");
  }

  std::vector<T> res;
  while (true) {
    TRY_RESULT(w, peek_raw_token());

    if (w == "]") {
      get_raw_token();
      return res;
    }
    TRY_RESULT(val, get_token<T>());
    res.push_back(std::move(val));
  }
}

class QueryRunner {
 public:
  virtual ~QueryRunner() {
    std::cout << "~QueryRunner()" << std::endl;
  };
  virtual std::string name() const = 0;
  virtual std::string help() const = 0;
  virtual td::Status run(td::actor::ActorId<ValidatorEngineConsole> console, Tokenizer tokenizer) const = 0;
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
  td::Status run(td::actor::ActorId<ValidatorEngineConsole> console, Tokenizer tokenizer) const override {
    td::actor::create_actor<T>(PSTRING() << "query " << name(), std::move(console), std::move(tokenizer)).release();
    return td::Status::OK();
  }
  QueryRunnerImpl() {
  }
};

class Query : public td::actor::Actor {
 public:
  virtual ~Query() = default;
  Query(td::actor::ActorId<ValidatorEngineConsole> console, Tokenizer tokenizer)
      : console_(console), tokenizer_(std::move(tokenizer)) {
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
  Tokenizer tokenizer_;
};

class GetOverlaysStatsQuery : public Query {
 public:
  GetOverlaysStatsQuery(td::actor::ActorId<ValidatorEngineConsole> console, Tokenizer tokenizer)
      : Query(console, std::move(tokenizer)) {
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
