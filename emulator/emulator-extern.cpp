#include "emulator-extern.h"
#include "td/utils/base64.h"
#include "td/utils/Status.h"
#include "td/utils/JsonBuilder.h"
#include "td/utils/logging.h"
#include "td/utils/Variant.h"
#include "td/utils/overloaded.h"
#include "transaction-emulator.h"
#include "tvm-emulator.hpp"
#include "crypto/vm/stack.hpp"
#include "crypto/vm/memo.h"

// 将 base64 编码的 BOC 转换为 Cell 对象
td::Result<td::Ref<vm::Cell>> boc_b64_to_cell(const char *boc) {
  TRY_RESULT_PREFIX(boc_decoded, td::base64_decode(td::Slice(boc)), "无法解码 base64 boc: ");
  return vm::std_boc_deserialize(boc_decoded);
}

// 将 Cell 对象转换为 base64 编码的 BOC
td::Result<std::string> cell_to_boc_b64(td::Ref<vm::Cell> cell) {
  TRY_RESULT_PREFIX(boc, vm::std_boc_serialize(std::move(cell), vm::BagOfCells::Mode::WithCRC32C), "无法序列化 Cell: ");
  return td::base64_encode(boc.as_slice());
}

// 创建成功响应的 JSON 字符串
const char *success_response(std::string&& transaction, std::string&& new_shard_account, std::string&& vm_log, 
                             td::optional<std::string>&& actions, double elapsed_time) {
  td::JsonBuilder jb;
  auto json_obj = jb.enter_object();
  json_obj("success", td::JsonTrue());
  json_obj("transaction", std::move(transaction));
  json_obj("shard_account", std::move(new_shard_account));
  json_obj("vm_log", std::move(vm_log));
  if (actions) {
    json_obj("actions", actions.unwrap());
  } else {
    json_obj("actions", td::JsonNull());
  }
  json_obj("elapsed_time", elapsed_time);
  json_obj.leave();
  return strdup(jb.string_builder().as_cslice().c_str());
}

// 创建错误响应的 JSON 字符串
const char *error_response(std::string&& error) {
  td::JsonBuilder jb;
  auto json_obj = jb.enter_object();
  json_obj("success", td::JsonFalse());
  json_obj("error", std::move(error));
  json_obj("external_not_accepted", td::JsonFalse());
  json_obj.leave();
  return strdup(jb.string_builder().as_cslice().c_str());
}

// 创建外部消息未被接受的响应的 JSON 字符串
const char *external_not_accepted_response(std::string&& vm_log, int vm_exit_code, double elapsed_time) {
  td::JsonBuilder jb;
  auto json_obj = jb.enter_object();
  json_obj("success", td::JsonFalse());
  json_obj("error", "外部消息未被智能合约接受");
  json_obj("external_not_accepted", td::JsonTrue());
  json_obj("vm_log", std::move(vm_log));
  json_obj("vm_exit_code", vm_exit_code);
  json_obj("elapsed_time", elapsed_time);
  json_obj.leave();
  return strdup(jb.string_builder().as_cslice().c_str());
}

// 宏定义，用于返回错误响应
#define ERROR_RESPONSE(error) return error_response(error)

// 解码配置 BOC 并返回配置对象
td::Result<block::Config> decode_config(const char* config_boc) {
  TRY_RESULT_PREFIX(config_params_cell, boc_b64_to_cell(config_boc), "无法反序列化配置参数 boc: ");
  auto global_config = block::Config(config_params_cell, td::Bits256::zero(), block::Config::needWorkchainInfo | block::Config::needSpecialSmc | block::Config::needCapabilities);
  TRY_STATUS_PREFIX(global_config.unpack(), "无法解包配置参数: ");
  return global_config;
}

// 创建 TransactionEmulator 对象
void *transaction_emulator_create(const char *config_params_boc, int vm_log_verbosity) {
  auto global_config_res = decode_config(config_params_boc);
  if (global_config_res.is_error()) {
    LOG(ERROR) << global_config_res.move_as_error().message();
    return nullptr;
  }

  return new emulator::TransactionEmulator(global_config_res.move_as_ok(), vm_log_verbosity);
}

// 模拟交易
const char *transaction_emulator_emulate_transaction(void *transaction_emulator, const char *shard_account_boc, const char *message_boc) {
  auto emulator = static_cast<emulator::TransactionEmulator *>(transaction_emulator);
  
  // 将 message BOC 转换为 Cell 对象
  auto message_cell_r = boc_b64_to_cell(message_boc);
  if (message_cell_r.is_error()) {
    ERROR_RESPONSE(PSTRING() << "无法反序列化消息 boc: " << message_cell_r.move_as_error());
  }
  auto message_cell = message_cell_r.move_as_ok();
  auto message_cs = vm::load_cell_slice(message_cell);
  int msg_tag = block::gen::t_CommonMsgInfo.get_tag(message_cs);

  // 将 shard account BOC 转换为 Cell 对象
  auto shard_account_cell = boc_b64_to_cell(shard_account_boc);
  if (shard_account_cell.is_error()) {
    ERROR_RESPONSE(PSTRING() << "无法反序列化 shard account boc: " << shard_account_cell.move_as_error());
  }
  auto shard_account_slice = vm::load_cell_slice(shard_account_cell.ok_ref());
  block::gen::ShardAccount::Record shard_account;
  if (!tlb::unpack(shard_account_slice, shard_account)) {
    ERROR_RESPONSE(PSTRING() << "无法解包 shard account cell");
  }

  td::Ref<vm::CellSlice> addr_slice;
  auto account_slice = vm::load_cell_slice(shard_account.account);
  bool account_exists = block::gen::t_Account.get_tag(account_slice) == block::gen::Account::account;
  if (block::gen::t_Account.get_tag(account_slice) == block::gen::Account::account_none) {
    if (msg_tag == block::gen::CommonMsgInfo::ext_in_msg_info) {
      block::gen::CommonMsgInfo::Record_ext_in_msg_info info;
      if (!tlb::unpack(message_cs, info)) {
        ERROR_RESPONSE(PSTRING() <<  "无法解包外部消息");
      }
      addr_slice = std::move(info.dest);
    }
    else if (msg_tag == block::gen::CommonMsgInfo::int_msg_info) {
      block::gen::CommonMsgInfo::Record_int_msg_info info;
      if (!tlb::unpack(message_cs, info)) {
          ERROR_RESPONSE(PSTRING() << "无法解包内部消息");
      }
      addr_slice = std::move(info.dest);
    } else {
      ERROR_RESPONSE(PSTRING() << "仅支持外部消息和内部消息");
    }
  } else if (block::gen::t_Account.get_tag(account_slice) == block::gen::Account::account) {
    block::gen::Account::Record_account account_record;
    if (!tlb::unpack(account_slice, account_record)) {
      ERROR_RESPONSE(PSTRING() << "无法解包账户 cell");
    }
    addr_slice = std::move(account_record.addr);
  } else {
    ERROR_RESPONSE(PSTRING() << "无法解析账户 cell");
  }
  
  // 提取工作链 ID 和智能合约地址
  ton::WorkchainId wc;
  ton::StdSmcAddress addr;
  if (!block::tlb::t_MsgAddressInt.extract_std_address(addr_slice, wc, addr)) {
    ERROR_RESPONSE(PSTRING() << "无法提取账户地址");
  }

  auto account = block::Account(wc, addr.bits());
  ton::UnixTime now = emulator->get_unixtime();
  if (!now) {
    now = (unsigned)std::time(nullptr);
  }
  bool is_special = wc == ton::masterchainId && emulator->get_config().is_special_smartcontract(addr);
  if (account_exists) {
    if (!account.unpack(vm::load_cell_slice_ref(shard_account_cell.move_as_ok()), now, is_special)) {
      ERROR_RESPONSE(PSTRING() << "无法解包 shard account");
    }
  } else {
    if (!account.init_new(now)) {
      ERROR_RESPONSE(PSTRING() << "无法初始化新账户");
    }
    account.last_trans_lt_ = shard_account.last_trans_lt;
    account.last_trans_hash_ = shard_account.last_trans_hash;
  }

  auto result = emulator->emulate_transaction(std::move(account), message_cell, now, 0, block::transaction::Transaction::tr_ord);
  if (result.is_error()) {
    ERROR_RESPONSE(PSTRING() << "模拟交易失败: " << result.move_as_error());
  }
  auto emulation_result = result.move_as_ok();

  auto external_not_accepted = dynamic_cast<emulator::TransactionEmulator::EmulationExternalNotAccepted *>(emulation_result.get());
  if (external_not_accepted) {
    return external_not_accepted_response(std::move(external_not_accepted->vm_log), external_not_accepted->vm_exit_code, 
                                          external_not_accepted->elapsed_time);
  }

  auto emulation_success = dynamic_cast<emulator::TransactionEmulator::EmulationSuccess&>(*emulation_result);
  auto trans_boc_b64 = cell_to_boc_b64(std::move(emulation_success.transaction));
  if (trans_boc_b64.is_error()) {
    ERROR_RESPONSE(PSTRING() << "无法序列化交易为 boc " << trans_boc_b64.move_as_error());
  }

  auto new_shard_account_cell = vm::CellBuilder().store_ref(emulation_success.account.total_state)
                               .store_bits(emulation_success.account.last_trans_hash_.as_bitslice())
                               .store_long(emulation_success.account.last_trans_lt_).finalize();
  auto new_shard_account_boc_b64 = cell_to_boc_b64(std::move(new_shard_account_cell));
  if (new_shard_account_boc_b64.is_error()) {
    ERROR_RESPONSE(PSTRING() << "无法序列化 ShardAccount 为 boc " << new_shard_account_boc_b64.move_as_error());
  }

  td::optional<td::string> actions_boc_b64;
  if (emulation_success.actions.not_null()) {
    auto actions_boc_b64_result = cell_to_boc_b64(std::move(emulation_success.actions));
    if (actions_boc_b64_result.is_error()) {
      ERROR_RESPONSE(PSTRING() << "无法序列化 actions 列表 cell 为 boc " << actions_boc_b64_result.move_as_error());
    }
    actions_boc_b64 = actions_boc_b64_result.move_as_ok();
  }

  return success_response(trans_boc_b64.move_as_ok(), new_shard_account_boc_b64.move_as_ok(), std::move(emulation_success.vm_log), 
                          std::move(actions_boc_b64), emulation_success.elapsed_time);
}

// 模拟 Tick/Tock 交易
const char *transaction_emulator_emulate_tick_tock_transaction(void *transaction_emulator, const char *shard_account_boc, bool is_tock) {
  auto emulator = static_cast<emulator::TransactionEmulator *>(transaction_emulator);
  
  // 将 shard account BOC 转换为 Cell 对象
  auto shard_account_cell = boc_b64_to_cell(shard_account_boc);
  if (shard_account_cell.is_error()) {
    ERROR_RESPONSE(PSTRING() << "无法反序列化 shard account boc: " << shard_account_cell.move_as_error());
  }
  auto shard_account_slice = vm::load_cell_slice(shard_account_cell.ok_ref());
  block::gen::ShardAccount::Record shard_account;
  if (!tlb::unpack(shard_account_slice, shard_account)) {
    ERROR_RESPONSE(PSTRING() << "无法解包 shard account cell");
  }

  td::Ref<vm::CellSlice> addr_slice;
  auto account_slice = vm::load_cell_slice(shard_account.account);
  if (block::gen::t_Account.get_tag(account_slice) == block::gen::Account::account_none) {
    ERROR_RESPONSE(PSTRING() <<  "无法在 account_none 上运行 tick/tock 交易");
  }
  block::gen::Account::Record_account account_record;
  if (!tlb::unpack(account_slice, account_record)) {
    ERROR_RESPONSE(PSTRING() << "无法解包账户 cell");
  }
  addr_slice = std::move(account_record.addr);
  
  // 提取工作链 ID 和智能合约地址
  ton::WorkchainId wc;
  ton::StdSmcAddress addr;
  if (!block::tlb::t_MsgAddressInt.extract_std_address(addr_slice, wc, addr)) {
    ERROR_RESPONSE(PSTRING() << "无法提取账户地址");
  }

  auto account = block::Account(wc, addr.bits());
  ton::UnixTime now = emulator->get_unixtime();
  if (!now) {
    now = (unsigned)std::time(nullptr);
  }
  bool is_special = wc == ton::masterchainId && emulator->get_config().is_special_smartcontract(addr);
  if (!account.unpack(vm::load_cell_slice_ref(shard_account_cell.move_as_ok()), now, is_special)) {
    ERROR_RESPONSE(PSTRING() << "无法解包 shard account");
  }

  auto trans_type = is_tock ? block::transaction::Transaction::tr_tock : block::transaction::Transaction::tr_tick;
  auto result = emulator->emulate_transaction(std::move(account), {}, now, 0, trans_type);
  if (result.is_error()) {
    ERROR_RESPONSE(PSTRING() << "模拟交易失败: " << result.move_as_error());
  }
  auto emulation_result = result.move_as_ok();

  auto emulation_success = dynamic_cast<emulator::TransactionEmulator::EmulationSuccess&>(*emulation_result);
  auto trans_boc_b64 = cell_to_boc_b64(std::move(emulation_success.transaction));
  if (trans_boc_b64.is_error()) {
    ERROR_RESPONSE(PSTRING() << "无法序列化交易为 boc " << trans_boc_b64.move_as_error());
  }

  auto new_shard_account_cell = vm::CellBuilder().store_ref(emulation_success.account.total_state)
                               .store_bits(emulation_success.account.last_trans_hash_.as_bitslice())
                               .store_long(emulation_success.account.last_trans_lt_).finalize();
  auto new_shard_account_boc_b64 = cell_to_boc_b64(std::move(new_shard_account_cell));
  if (new_shard_account_boc_b64.is_error()) {
    ERROR_RESPONSE(PSTRING() << "无法序列化 ShardAccount 为 boc " << new_shard_account_boc_b64.move_as_error());
  }

  td::optional<td::string> actions_boc_b64;
  if (emulation_success.actions.not_null()) {
    auto actions_boc_b64_result = cell_to_boc_b64(std::move(emulation_success.actions));
    if (actions_boc_b64_result.is_error()) {
      ERROR_RESPONSE(PSTRING() << "无法序列化 actions 列表 cell 为 boc " << actions_boc_b64_result.move_as_error());
    }
    actions_boc_b64 = actions_boc_b64_result.move_as_ok();
  }

  return success_response(trans_boc_b64.move_as_ok(), new_shard_account_boc_b64.move_as_ok(), std::move(emulation_success.vm_log), 
                          std::move(actions_boc_b64), emulation_success.elapsed_time);
}

// 设置 Unix 时间戳
bool transaction_emulator_set_unixtime(void *transaction_emulator, uint32_t unixtime) {
  auto emulator = static_cast<emulator::TransactionEmulator *>(transaction_emulator);

  emulator->set_unixtime(unixtime);

  return true;
}

// 设置 LT 值
bool transaction_emulator_set_lt(void *transaction_emulator, uint64_t lt) {
  auto emulator = static_cast<emulator::TransactionEmulator *>(transaction_emulator);

  emulator->set_lt(lt);

  return true;
}

// 设置随机种子
bool transaction_emulator_set_rand_seed(void *transaction_emulator, const char* rand_seed_hex) {
  auto emulator = static_cast<emulator::TransactionEmulator *>(transaction_emulator);

  auto rand_seed_hex_slice = td::Slice(rand_seed_hex);
  if (rand_seed_hex_slice.size() != 64) {
    LOG(ERROR) << "随机种子应为 64 字符的十六进制字符串";
    return false;
  }
  auto rand_seed_bytes = td::hex_decode(rand_seed_hex_slice);
  if (rand_seed_bytes.is_error()) {
    LOG(ERROR) << "无法解码十六进制随机种子";
    return false;
  }
  td::BitArray<256> rand_seed;
  rand_seed.as_slice().copy_from(rand_seed_bytes.move_as_ok());

  emulator->set_rand_seed(rand_seed);
  return true;
}

// 设置是否忽略签名检查
bool transaction_emulator_set_ignore_chksig(void *transaction_emulator, bool ignore_chksig) {
  auto emulator = static_cast<emulator::TransactionEmulator *>(transaction_emulator);

  emulator->set_ignore_chksig(ignore_chksig);

  return true;
}

// 设置配置
bool transaction_emulator_set_config(void *transaction_emulator, const char* config_boc) {
  auto emulator = static_cast<emulator::TransactionEmulator *>(transaction_emulator);

  auto global_config_res = decode_config(config_boc);
  if (global_config_res.is_error()) {
    LOG(ERROR) << global_config_res.move_as_error().message();
    return false;
  }

  emulator->set_config(global_config_res.move_as_ok());

  return true;
}

// 设置库
bool transaction_emulator_set_libs(void *transaction_emulator, const char* shardchain_libs_boc) {
  auto emulator = static_cast<emulator::TransactionEmulator *>(transaction_emulator);

  if (shardchain_libs_boc != nullptr) {
    auto shardchain_libs_cell = boc_b64_to_cell(shardchain_libs_boc);
    if (shardchain_libs_cell.is_error()) {
      LOG(ERROR) << "无法反序列化 shardchain libraries boc: " << shardchain_libs_cell.move_as_error();
      return false;
    }
    emulator->set_libs(vm::Dictionary(shardchain_libs_cell.move_as_ok(), 256));
  }

  return true;
}


// 设置调试模式启用状态
bool transaction_emulator_set_debug_enabled(void *transaction_emulator, bool debug_enabled) {
  auto emulator = static_cast<emulator::TransactionEmulator *>(transaction_emulator);

  emulator->set_debug_enabled(debug_enabled);

  return true;
}

// 设置之前的区块信息
bool transaction_emulator_set_prev_blocks_info(void *transaction_emulator, const char* info_boc) {
  auto emulator = static_cast<emulator::TransactionEmulator *>(transaction_emulator);

  if (info_boc != nullptr) {
    auto info_cell = boc_b64_to_cell(info_boc);
    if (info_cell.is_error()) {
      LOG(ERROR) << "无法反序列化之前的区块 boc: " << info_cell.move_as_error();
      return false;
    }
    vm::StackEntry info_value;
    if (!info_value.deserialize(info_cell.move_as_ok())) {
      LOG(ERROR) << "无法反序列化之前的区块元组";
      return false;
    }
    if (info_value.is_null()) {
      emulator->set_prev_blocks_info({});
    } else if (info_value.is_tuple()) {
      emulator->set_prev_blocks_info(info_value.as_tuple());
    } else {
      LOG(ERROR) << "无法设置之前的区块元组: 不是一个元组";
      return false;
    }
  }

  return true;
}

// 销毁交易模拟器实例
void transaction_emulator_destroy(void *transaction_emulator) {
  delete static_cast<emulator::TransactionEmulator *>(transaction_emulator);
}

// 设置日志详细级别
bool emulator_set_verbosity_level(int verbosity_level) {
  if (0 <= verbosity_level && verbosity_level <= VERBOSITY_NAME(NEVER)) {
    SET_VERBOSITY_LEVEL(VERBOSITY_NAME(FATAL) + verbosity_level);
    return true;
  }
  return false;
}

// 创建 TVM 模拟器
void *tvm_emulator_create(const char *code, const char *data, int vm_log_verbosity) {
  auto code_cell = boc_b64_to_cell(code);
  if (code_cell.is_error()) {
    LOG(ERROR) << "无法反序列化代码 boc: " << code_cell.move_as_error();
    return nullptr;
  }
  auto data_cell = boc_b64_to_cell(data);
  if (data_cell.is_error()) {
    LOG(ERROR) << "无法反序列化数据 boc: " << data_cell.move_as_error();
    return nullptr;
  }

  auto emulator = new emulator::TvmEmulator(code_cell.move_as_ok(), data_cell.move_as_ok());
  emulator->set_vm_verbosity_level(vm_log_verbosity);
  return emulator;
}

// 设置 TVM 模拟器的库
bool tvm_emulator_set_libraries(void *tvm_emulator, const char *libs_boc) {
  vm::Dictionary libs{256};
  auto libs_cell = boc_b64_to_cell(libs_boc);
  if (libs_cell.is_error()) {
    LOG(ERROR) << "无法反序列化库 boc: " << libs_cell.move_as_error();
    return false;
  }
  libs = vm::Dictionary(libs_cell.move_as_ok(), 256);

  auto emulator = static_cast<emulator::TvmEmulator *>(tvm_emulator);
  emulator->set_libraries(std::move(libs));

  return true;
}

// 设置 TVM 模拟器的 C7 状态
bool tvm_emulator_set_c7(void *tvm_emulator, const char *address, uint32_t unixtime, uint64_t balance, const char *rand_seed_hex, const char *config_boc) {
  auto emulator = static_cast<emulator::TvmEmulator *>(tvm_emulator);
  auto std_address = block::StdAddress::parse(td::Slice(address));
  if (std_address.is_error()) {
    LOG(ERROR) << "无法解析地址: " << std_address.move_as_error();
    return false;
  }
  
  std::shared_ptr<block::Config> global_config;
  if (config_boc != nullptr) {
    auto config_params_cell = boc_b64_to_cell(config_boc);
    if (config_params_cell.is_error()) {
      LOG(ERROR) << "无法反序列化配置参数 boc: " << config_params_cell.move_as_error();
      return false;
    }
    global_config = std::make_shared<block::Config>(
        config_params_cell.move_as_ok(), td::Bits256::zero(),
        block::Config::needWorkchainInfo | block::Config::needSpecialSmc | block::Config::needCapabilities);
    auto unpack_res = global_config->unpack();
    if (unpack_res.is_error()) {
      LOG(ERROR) << "无法解包配置参数";
      return false;
    }
  }

  auto rand_seed_hex_slice = td::Slice(rand_seed_hex);
  if (rand_seed_hex_slice.size() != 64) {
    LOG(ERROR) << "随机种子应为 64 字符的十六进制字符串";
    return false;
  }
  auto rand_seed_bytes = td::hex_decode(rand_seed_hex_slice);
  if (rand_seed_bytes.is_error()) {
    LOG(ERROR) << "无法解码十六进制随机种子";
    return false;
  }
  td::BitArray<256> rand_seed;
  rand_seed.as_slice().copy_from(rand_seed_bytes.move_as_ok());

  emulator->set_c7(std_address.move_as_ok(), unixtime, balance, rand_seed, std::const_pointer_cast<const block::Config>(global_config));
  
  return true;
}

// 设置 TVM 模拟器的之前区块信息
bool tvm_emulator_set_prev_blocks_info(void *tvm_emulator, const char* info_boc) {
  auto emulator = static_cast<emulator::TvmEmulator *>(tvm_emulator);

  if (info_boc != nullptr) {
    auto info_cell = boc_b64_to_cell(info_boc);
    if (info_cell.is_error()) {
      LOG(ERROR) << "无法反序列化之前的区块 boc: " << info_cell.move_as_error();
      return false;
    }
    vm::StackEntry info_value;
    if (!info_value.deserialize(info_cell.move_as_ok())) {
      LOG(ERROR) << "无法反序列化之前的区块元组";
      return false;
    }
    if (info_value.is_null()) {
      emulator->set_prev_blocks_info({});
    } else if (info_value.is_tuple()) {
      emulator->set_prev_blocks_info(info_value.as_tuple());
    } else {
      LOG(ERROR) << "无法设置之前的区块元组: 不是一个元组";
      return false;
    }
  }

  return true;
}

// 设置 TVM 模拟器的 gas 限制
bool tvm_emulator_set_gas_limit(void *tvm_emulator, int64_t gas_limit) {
  auto emulator = static_cast<emulator::TvmEmulator *>(tvm_emulator);
  emulator->set_gas_limit(gas_limit);
  return true;
}

// 设置 TVM 模拟器的调试模式启用状态
bool tvm_emulator_set_debug_enabled(void *tvm_emulator, bool debug_enabled) {
  auto emulator = static_cast<emulator::TvmEmulator *>(tvm_emulator);
  emulator->set_debug_enabled(debug_enabled);
  return true;
}

// 执行 TVM 模拟器的 get 方法
const char *tvm_emulator_run_get_method(void *tvm_emulator, int method_id, const char *stack_boc) {
  auto stack_cell = boc_b64_to_cell(stack_boc);
  if (stack_cell.is_error()) {
    ERROR_RESPONSE(PSTRING() << "无法反序列化栈 cell: " << stack_cell.move_as_error().to_string());
  }
  auto stack_cs = vm::load_cell_slice(stack_cell.move_as_ok());
  td::Ref<vm::Stack> stack;
  if (!vm::Stack::deserialize_to(stack_cs, stack)) {
     ERROR_RESPONSE(PSTRING() << "无法反序列化栈");
  }

  auto emulator = static_cast<emulator::TvmEmulator *>(tvm_emulator);
  auto result = emulator->run_get_method(method_id, stack);
  
  vm::FakeVmStateLimits fstate(3500);  // 限制递归 (反) 序列化调用
  vm::VmStateInterface::Guard guard(&fstate);
  
  vm::CellBuilder stack_cb;
  if (!result.stack->serialize(stack_cb)) {
    ERROR_RESPONSE(PSTRING() << "无法序列化栈");
  }
  auto result_stack_boc = cell_to_boc_b64(stack_cb.finalize());
  if (result_stack_boc.is_error()) {
    ERROR_RESPONSE(PSTRING() << "无法序列化栈 cell: " << result_stack_boc.move_as_error().to_string());
  }

  td::JsonBuilder jb;
  auto json_obj = jb.enter_object();
  json_obj("success", td::JsonTrue());
  json_obj("stack", result_stack_boc.move_as_ok());
  json_obj("gas_used", std::to_string(result.gas_used));
  json_obj("vm_exit_code", result.code);
  json_obj("vm_log", result.vm_log);
  if (!result.missing_library) {
    json_obj("missing_library", td::JsonNull());
  } else {
    json_obj("missing_library", result.missing_library.value().to_hex());
  }
  json_obj.leave();

  return strdup(jb.string_builder().as_cslice().c_str());
}

// 执行 TVM 模拟器的运行方法
const char *tvm_emulator_emulate_run_method(uint32_t len, const char *params_boc, int64_t gas_limit) {
  auto params_cell = vm::std_boc_deserialize(td::Slice(params_boc, len));
  if (params_cell.is_error()) {
    return nullptr;
  }
  auto params_cs = vm::load_cell_slice(params_cell.move_as_ok());
  auto code = params_cs.fetch_ref();
  auto data = params_cs.fetch_ref();

  auto stack_cs = vm::load_cell_slice(params_cs.fetch_ref());
  auto params = vm::load_cell_slice(params_cs.fetch_ref());
  auto c7_cs = vm::load_cell_slice(params.fetch_ref());
  auto libs = vm::Dictionary(params.fetch_ref(), 256);

  auto method_id = params_cs.fetch_long(32);

  td::Ref<vm::Stack> stack;
  if (!vm::Stack::deserialize_to(stack_cs, stack)) {
    return nullptr;
  }

  td::Ref<vm::Stack> c7;
  if (!vm::Stack::deserialize_to(c7_cs, c7)) {
    return nullptr;
  }

  auto emulator = new emulator::TvmEmulator(code, data);
  emulator->set_vm_verbosity_level(0);
  emulator->set_gas_limit(gas_limit);
  emulator->set_c7_raw(c7->fetch(0).as_tuple());
  if (libs.is_empty()) {
    emulator->set_libraries(std::move(libs));
  }
  auto result = emulator->run_get_method(int(method_id), stack);
  delete emulator;

  vm::CellBuilder stack_cb;
  if (!result.stack->serialize(stack_cb)) {
    return nullptr;
  }

  vm::CellBuilder cb;
  cb.store_long(result.code, 32);
  cb.store_long(result.gas_used, 64);
  cb.store_ref(stack_cb.finalize());

  auto ser = vm::std_boc_serialize(cb.finalize());
  if (!ser.is_ok()) {
    return nullptr;
  }
  auto sok = ser.move_as_ok();

  auto sz = uint32_t(sok.size());
  char* rn = (char*)malloc(sz + 4);
  memcpy(rn, &sz, 4);
  memcpy(rn+4, sok.data(), sz);

  return rn;
}

// 发送外部消息
const char *tvm_emulator_send_external_message(void *tvm_emulator, const char *message_body_boc) {
  auto message_body_cell = boc_b64_to_cell(message_body_boc);
  if (message_body_cell.is_error()) {
    ERROR_RESPONSE(PSTRING() << "无法反序列化消息体 boc: " << message_body_cell.move_as_error());
  }

  auto emulator = static_cast<emulator::TvmEmulator *>(tvm_emulator);
  auto result = emulator->send_external_message(message_body_cell.move_as_ok());

  td::JsonBuilder jb;
  auto json_obj = jb.enter_object();
  json_obj("success", td::JsonTrue());
  json_obj("gas_used", std::to_string(result.gas_used));
  json_obj("vm_exit_code", result.code);
  json_obj("accepted", td::JsonBool(result.accepted));
  json_obj("vm_log", result.vm_log);
  if (!result.missing_library) {
    json_obj("missing_library", td::JsonNull());
  } else {
    json_obj("missing_library", result.missing_library.value().to_hex());
  }
  if (result.actions.is_null()) {
    json_obj("actions", td::JsonNull());
  } else {
    json_obj("actions", cell_to_boc_b64(result.actions).move_as_ok());
  }
  json_obj("new_code", cell_to_boc_b64(result.new_state.code).move_as_ok());
  json_obj("new_data", cell_to_boc_b64(result.new_state.data).move_as_ok());
  json_obj.leave();

  return strdup(jb.string_builder().as_cslice().c_str());
}

// 发送内部消息
const char *tvm_emulator_send_internal_message(void *tvm_emulator, const char *message_body_boc, uint64_t amount) {
  auto message_body_cell = boc_b64_to_cell(message_body_boc);
  if (message_body_cell.is_error()) {
    ERROR_RESPONSE(PSTRING() << "无法反序列化消息体 boc: " << message_body_cell.move_as_error());
  }

  auto emulator = static_cast<emulator::TvmEmulator *>(tvm_emulator);
  auto result = emulator->send_internal_message(message_body_cell.move_as_ok(), amount);

  td::JsonBuilder jb;
  auto json_obj = jb.enter_object();
  json_obj("success", td::JsonTrue());
  json_obj("gas_used", std::to_string(result.gas_used));
  json_obj("vm_exit_code", result.code);
  json_obj("accepted", td::JsonBool(result.accepted));
  json_obj("vm_log", result.vm_log);
  if (!result.missing_library) {
    json_obj("missing_library", td::JsonNull());
  } else {
    json_obj("missing_library", result.missing_library.value().to_hex());
  }
  if (result.actions.is_null()) {
    json_obj("actions", td::JsonNull());
  } else {
    json_obj("actions", cell_to_boc_b64(result.actions).move_as_ok());
  }
  json_obj("new_code", cell_to_boc_b64(result.new_state.code).move_as_ok());
  json_obj("new_data", cell_to_boc_b64(result.new_state.data).move_as_ok());
  json_obj.leave();

  return strdup(jb.string_builder().as_cslice().c_str());
}

// 销毁 TVM 模拟器实例
void tvm_emulator_destroy(void *tvm_emulator) {
  delete static_cast<emulator::TvmEmulator *>(tvm_emulator);
}
