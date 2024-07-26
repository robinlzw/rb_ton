#include "emulator-extern.h"
#include "td/utils/logging.h"
#include "td/utils/JsonBuilder.h"
#include "td/utils/misc.h"
#include "td/utils/optional.h"
#include "StringLog.h"
#include <iostream>
#include "crypto/common/bitstring.h"

// 交易模拟参数结构体
struct TransactionEmulationParams {
  uint32_t utime;              // 时间戳
  uint64_t lt;                 // 逻辑时间
  td::optional<std::string> rand_seed_hex; // 随机种子的十六进制字符串
  bool ignore_chksig;          // 是否忽略签名检查
  bool is_tick_tock;           // 是否是 tick-tock 交易
  bool is_tock;                // 是否是 tock 交易
  bool debug_enabled;          // 是否启用调试
};

// 解码交易模拟参数
td::Result<TransactionEmulationParams> decode_transaction_emulation_params(const char* json) {
  TransactionEmulationParams params;

  std::string json_str(json);
  TRY_RESULT(input_json, td::json_decode(td::MutableSlice(json_str)));
  auto &obj = input_json.get_object();

  // 获取时间戳
  TRY_RESULT(utime_field, td::get_json_object_field(obj, "utime", td::JsonValue::Type::Number, false));
  TRY_RESULT(utime, td::to_integer_safe<td::uint32>(utime_field.get_number()));
  params.utime = utime;

  // 获取逻辑时间
  TRY_RESULT(lt_field, td::get_json_object_field(obj, "lt", td::JsonValue::Type::String, false));
  TRY_RESULT(lt, td::to_integer_safe<td::uint64>(lt_field.get_string()));
  params.lt = lt;

  // 获取随机种子
  TRY_RESULT(rand_seed_str, td::get_json_object_string_field(obj, "rand_seed", true));
  if (rand_seed_str.size() > 0) {
    params.rand_seed_hex = rand_seed_str;
  }

  // 获取是否忽略签名检查
  TRY_RESULT(ignore_chksig, td::get_json_object_bool_field(obj, "ignore_chksig", false));
  params.ignore_chksig = ignore_chksig;

  // 获取调试模式是否启用
  TRY_RESULT(debug_enabled, td::get_json_object_bool_field(obj, "debug_enabled", false));
  params.debug_enabled = debug_enabled;

  // 获取 tick-tock 交易标志
  TRY_RESULT(is_tick_tock, td::get_json_object_bool_field(obj, "is_tick_tock", true, false));
  params.is_tick_tock = is_tick_tock;

  // 获取 tock 交易标志
  TRY_RESULT(is_tock, td::get_json_object_bool_field(obj, "is_tock", true, false));
  params.is_tock = is_tock;

  // 校验 tick-tock 参数一致性
  if (is_tock && !is_tick_tock) {
    return td::Status::Error("Inconsistent parameters is_tick_tock=false, is_tock=true");
  }

  return params;
}

// 获取方法参数结构体
struct GetMethodParams {
  std::string code;                   // 合约代码
  std::string data;                   // 合约数据
  int verbosity;                      // 日志详细级别
  td::optional<std::string> libs;     // 库
  td::optional<std::string> prev_blocks_info; // 之前的区块信息
  std::string address;                // 地址
  uint32_t unixtime;                  // 时间戳
  uint64_t balance;                  // 余额
  std::string rand_seed_hex;          // 随机种子
  int64_t gas_limit;                  // gas 限制
  int method_id;                      // 方法 ID
  bool debug_enabled;                 // 是否启用调试
};

// 解码获取方法的参数
td::Result<GetMethodParams> decode_get_method_params(const char* json) {
  GetMethodParams params;

  std::string json_str(json);
  TRY_RESULT(input_json, td::json_decode(td::MutableSlice(json_str)));
  auto &obj = input_json.get_object();

  // 获取合约代码
  TRY_RESULT(code, td::get_json_object_string_field(obj, "code", false));
  params.code = code;

  // 获取合约数据
  TRY_RESULT(data, td::get_json_object_string_field(obj, "data", false));
  params.data = data;

  // 获取日志详细级别
  TRY_RESULT(verbosity, td::get_json_object_int_field(obj, "verbosity", false));
  params.verbosity = verbosity;

  // 获取库
  TRY_RESULT(libs, td::get_json_object_string_field(obj, "libs", true));
  if (libs.size() > 0) {
    params.libs = libs;
  }

  // 获取之前的区块信息
  TRY_RESULT(prev_blocks_info, td::get_json_object_string_field(obj, "prev_blocks_info", true));
  if (prev_blocks_info.size() > 0) {
    params.prev_blocks_info = prev_blocks_info;
  }

  // 获取地址
  TRY_RESULT(address, td::get_json_object_string_field(obj, "address", false));
  params.address = address;

  // 获取时间戳
  TRY_RESULT(unixtime_field, td::get_json_object_field(obj, "unixtime", td::JsonValue::Type::Number, false));
  TRY_RESULT(unixtime, td::to_integer_safe<td::uint32>(unixtime_field.get_number()));
  params.unixtime = unixtime;

  // 获取余额
  TRY_RESULT(balance_field, td::get_json_object_field(obj, "balance", td::JsonValue::Type::String, false));
  TRY_RESULT(balance, td::to_integer_safe<td::uint64>(balance_field.get_string()));
  params.balance = balance;

  // 获取随机种子
  TRY_RESULT(rand_seed_str, td::get_json_object_string_field(obj, "rand_seed", false));
  params.rand_seed_hex = rand_seed_str;

  // 获取 gas 限制
  TRY_RESULT(gas_limit_field, td::get_json_object_field(obj, "gas_limit", td::JsonValue::Type::String, false));
  TRY_RESULT(gas_limit, td::to_integer_safe<td::uint64>(gas_limit_field.get_string()));
  params.gas_limit = gas_limit;

  // 获取方法 ID
  TRY_RESULT(method_id, td::get_json_object_int_field(obj, "method_id", false));
  params.method_id = method_id;

  // 获取调试模式是否启用
  TRY_RESULT(debug_enabled, td::get_json_object_bool_field(obj, "debug_enabled", false));
  params.debug_enabled = debug_enabled;

  return params;
}

// 无操作日志实现
class NoopLog : public td::LogInterface {
 public:
  NoopLog() {
  }

  // 不执行任何操作的日志追加
  void append(td::CSlice new_slice, int log_level) override {
  }

  // 不执行任何操作的日志轮换
  void rotate() override {
  }
};

extern "C" {

// 创建模拟器
void* create_emulator(const char *config, int verbosity) {
    NoopLog logger;

    td::log_interface = &logger;

    SET_VERBOSITY_LEVEL(verbosity_NEVER); // 设置日志详细级别为 NEVER
    return transaction_emulator_create(config, verbosity);
}

// 销毁模拟器
void destroy_emulator(void* em) {
    NoopLog logger;

    td::log_interface = &logger;

    SET_VERBOSITY_LEVEL(verbosity_NEVER); // 设置日志详细级别为 NEVER
    transaction_emulator_destroy(em);
}

// 使用模拟器进行模拟
const char *emulate_with_emulator(void* em, const char* libs, const char* account, const char* message, const char* params) {
    StringLog logger;

    td::log_interface = &logger;
    SET_VERBOSITY_LEVEL(verbosity_DEBUG); // 设置日志详细级别为 DEBUG

    // 解码参数
    auto decoded_params_res = decode_transaction_emulation_params(params);
    if (decoded_params_res.is_error()) {
        return strdup(R"({"fail":true,"message":"Can't decode other params"})");
    }
    auto decoded_params = decoded_params_res.move_as_ok();

    // 设置随机种子
    bool rand_seed_set = true;
    if (decoded_params.rand_seed_hex) {
      rand_seed_set = transaction_emulator_set_rand_seed(em, decoded_params.rand_seed_hex.unwrap().c_str());
    }

    // 设置其他参数
    if (!transaction_emulator_set_libs(em, libs) ||
        !transaction_emulator_set_lt(em, decoded_params.lt) ||
        !transaction_emulator_set_unixtime(em, decoded_params.utime) ||
        !transaction_emulator_set_ignore_chksig(em, decoded_params.ignore_chksig) ||
        !transaction_emulator_set_debug_enabled(em, decoded_params.debug_enabled) ||
        !rand_seed_set) {
        transaction_emulator_destroy(em);
        return strdup(R"({"fail":true,"message":"Can't set params"})");
    }

    // 进行交易模拟
    const char *result;
    if (decoded_params.is_tick_tock) {
      result = transaction_emulator_emulate_tick_tock_transaction(em, account, decoded_params.is_tock);
    } else {
      result = transaction_emulator_emulate_transaction(em, account, message);
    }

    // 构建输出结果
    const char* output = nullptr;
    {
        td::JsonBuilder jb;
        auto json_obj = jb.enter_object();
        json_obj("output", td::JsonRaw(td::Slice(result)));
        json_obj("logs", logger.get_string());
        json_obj.leave();
        output = strdup(jb.string_builder().as_cslice().c_str());
    }
    free((void*) result);

    return output;
}

// 模拟函数
const char *emulate(const char *config, const char* libs, int verbosity, const char* account, const char* message, const char* params) {
    auto em = transaction_emulator_create(config, verbosity);
    auto result = emulate_with_emulator(em, libs, account, message, params);
    transaction_emulator_destroy(em);
    return result;
}

// 执行获取方法
const char *run_get_method(const char *params, const char* stack, const char* config) {
    StringLog logger;

    td::log_interface = &logger;
    SET_VERBOSITY_LEVEL(verbosity_DEBUG); // 设置日志详细级别为 DEBUG

    // 解码获取方法参数
    auto decoded_params_res = decode_get_method_params(params);
    if (decoded_params_res.is_error()) {
        return strdup(R"({"fail":true,"message":"Can't decode params"})");
    }
    auto decoded_params = decoded_params_res.move_as_ok();

    // 创建 TVM 模拟器
    auto tvm = tvm_emulator_create(decoded_params.code.c_str(), decoded_params.data.c_str(), decoded_params.verbosity);

    // 设置模拟器参数
    if ((decoded_params.libs && !tvm_emulator_set_libraries(tvm, decoded_params.libs.value().c_str())) ||
        !tvm_emulator_set_c7(tvm, decoded_params.address.c_str(), decoded_params.unixtime, decoded_params.balance,
                             decoded_params.rand_seed_hex.c_str(), config) ||
        (decoded_params.prev_blocks_info &&
         !tvm_emulator_set_prev_blocks_info(tvm, decoded_params.prev_blocks_info.value().c_str())) ||
        (decoded_params.gas_limit > 0 && !tvm_emulator_set_gas_limit(tvm, decoded_params.gas_limit)) ||
        !tvm_emulator_set_debug_enabled(tvm, decoded_params.debug_enabled)) {
        tvm_emulator_destroy(tvm);
        return strdup(R"({"fail":true,"message":"Can't set params"})");
    }

    // 执行获取方法
    auto res = tvm_emulator_run_get_method(tvm, decoded_params.method_id, stack);

    tvm_emulator_destroy(tvm);

    // 构建输出结果
    const char* output = nullptr;
    {
        td::JsonBuilder jb;
        auto json_obj = jb.enter_object();
        json_obj("output", td::JsonRaw(td::Slice(res)));
        json_obj("logs", logger.get_string());
        json_obj.leave();
        output = strdup(jb.string_builder().as_cslice().c_str());
    }
    free((void*) res);

    return output;
}

}
