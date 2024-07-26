#pragma once
#include "smc-envelope/SmartContract.h"

namespace emulator {
class TvmEmulator {
  ton::SmartContract smc_; // 智能合约对象，用于存储合约代码和数据
  ton::SmartContract::Args args_; // 模拟参数对象
public:
  using Answer = ton::SmartContract::Answer; // 使用SmartContract中的Answer类型

  // 构造函数，初始化智能合约的代码和数据
  TvmEmulator(td::Ref<vm::Cell> code, td::Ref<vm::Cell> data): smc_({code, data}) {
  }

  // 设置虚拟机日志的详细级别
  void set_vm_verbosity_level(int vm_log_verbosity) {
    args_.set_vm_verbosity_level(vm_log_verbosity);
  }

  // 设置共享库
  void set_libraries(vm::Dictionary&& libraries) {
    args_.set_libraries(libraries);
  }

  // 设置Gas限制
  void set_gas_limit(int64_t limit) {
    args_.set_limits(vm::GasLimits(limit));
  }

  // 设置合约的环境变量，包括地址、时间、余额、随机种子和配置
  void set_c7(block::StdAddress address, uint32_t unixtime, uint64_t balance, td::BitArray<256> rand_seed, std::shared_ptr<const block::Config> config) {
    args_.set_address(address);
    args_.set_now(unixtime);
    args_.set_balance(balance);
    args_.set_rand_seed(rand_seed);
    if (config) {
      args_.set_config(config);
    }
  }

  // 直接设置C7寄存器内容
  void set_c7_raw(td::Ref<vm::Tuple> c7) {
    args_.set_c7(std::move(c7));
  }

  // 设置前一个区块的信息
  void set_prev_blocks_info(td::Ref<vm::Tuple> tuple) {
    args_.set_prev_blocks_info(std::move(tuple));
  }

  // 启用或禁用调试模式
  void set_debug_enabled(bool debug_enabled) {
    args_.set_debug_enabled(debug_enabled);
  }

  // 运行智能合约的get方法
  Answer run_get_method(int method_id, td::Ref<vm::Stack> stack) {
    return smc_.run_get_method(args_.set_stack(stack).set_method_id(method_id));
  }

  // 发送外部消息
  Answer send_external_message(td::Ref<vm::Cell> message_body) {
    return smc_.send_external_message(message_body, args_);
  }

  // 发送内部消息，包含指定的金额
  Answer send_internal_message(td::Ref<vm::Cell> message_body, uint64_t amount) {
    return smc_.send_internal_message(message_body, args_.set_amount(amount));
  }
};
}
