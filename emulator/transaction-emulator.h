#pragma once
#include "crypto/common/refcnt.hpp"
#include "ton/ton-types.h"
#include "crypto/vm/cells.h"
#include "block/transaction.h"
#include "block/block-auto.h"
#include "block/block-parse.h"
#include "block/mc-config.h"

namespace emulator {
class TransactionEmulator {
  block::Config config_; // 配置信息
  vm::Dictionary libraries_; // 库
  int vm_log_verbosity_; // 虚拟机日志详细级别
  ton::UnixTime unixtime_; // Unix时间
  ton::LogicalTime lt_; // 逻辑时间
  td::BitArray<256> rand_seed_; // 随机种子
  bool ignore_chksig_; // 是否忽略签名检查
  bool debug_enabled_; // 是否启用调试
  td::Ref<vm::Tuple> prev_blocks_info_; // 前一个区块的信息

public:
  // 构造函数，初始化配置、库字典和其他参数
  TransactionEmulator(block::Config&& config, int vm_log_verbosity = 0) :
    config_(std::move(config)), libraries_(256), vm_log_verbosity_(vm_log_verbosity),
    unixtime_(0), lt_(0), rand_seed_(td::BitArray<256>::zero()), ignore_chksig_(false), debug_enabled_(false) {
  }

  // 模拟结果的结构体
  struct EmulationResult {
    std::string vm_log; // 虚拟机日志
    double elapsed_time; // 模拟消耗的时间

    EmulationResult(std::string vm_log_, double elapsed_time_) : vm_log(vm_log_), elapsed_time(elapsed_time_) {}
    virtual ~EmulationResult() = default;
  };

  // 成功模拟的结构体
  struct EmulationSuccess: EmulationResult {
    td::Ref<vm::Cell> transaction; // 交易信息
    block::Account account; // 账户信息
    td::Ref<vm::Cell> actions; // 操作信息

    EmulationSuccess(td::Ref<vm::Cell> transaction_, block::Account account_, std::string vm_log_, td::Ref<vm::Cell> actions_, double elapsed_time_) :
      EmulationResult(vm_log_, elapsed_time_), transaction(transaction_), account(account_) , actions(actions_)
    {}
  };

  // 外部消息未接受的结构体
  struct EmulationExternalNotAccepted: EmulationResult {
    int vm_exit_code; // 虚拟机退出码

    EmulationExternalNotAccepted(std::string vm_log_, int vm_exit_code_, double elapsed_time_) :
      EmulationResult(vm_log_, elapsed_time_), vm_exit_code(vm_exit_code_)
    {}
  };

  // 交易链模拟结果的结构体
  struct EmulationChain {
    std::vector<td::Ref<vm::Cell>> transactions; // 交易信息列表
    block::Account account; // 账户信息
  };

  // 获取配置
  const block::Config& get_config() {
    return config_;
  }

  // 获取当前Unix时间
  ton::UnixTime get_unixtime() {
    return unixtime_;
  }

  // 模拟交易的函数
  td::Result<std::unique_ptr<EmulationResult>> emulate_transaction(
      block::Account&& account, td::Ref<vm::Cell> msg_root, ton::UnixTime utime, ton::LogicalTime lt, int trans_type);

  // 模拟交易成功的函数
  td::Result<EmulationSuccess> emulate_transaction(block::Account&& account, td::Ref<vm::Cell> original_trans);
  // 模拟一系列交易链的函数
  td::Result<EmulationChain> emulate_transactions_chain(block::Account&& account, std::vector<td::Ref<vm::Cell>>&& original_transactions);

  // 设置Unix时间
  void set_unixtime(ton::UnixTime unixtime);
  // 设置逻辑时间
  void set_lt(ton::LogicalTime lt);
  // 设置随机种子
  void set_rand_seed(td::BitArray<256>& rand_seed);
  // 设置是否忽略签名检查
  void set_ignore_chksig(bool ignore_chksig);
  // 设置配置
  void set_config(block::Config &&config);
  // 设置库字典
  void set_libs(vm::Dictionary &&libs);
  // 设置调试模式
  void set_debug_enabled(bool debug_enabled);
  // 设置前一个区块的信息
  void set_prev_blocks_info(td::Ref<vm::Tuple> prev_blocks_info);

private:
  // 检查账户状态更新
  bool check_state_update(const block::Account& account, const block::gen::Transaction::Record& trans);

  // 创建交易
  td::Result<std::unique_ptr<block::transaction::Transaction>> create_transaction(
                                                         td::Ref<vm::Cell> msg_root, block::Account* acc,
                                                         ton::UnixTime utime, ton::LogicalTime lt, int trans_type,
                                                         block::StoragePhaseConfig* storage_phase_cfg,
                                                         block::ComputePhaseConfig* compute_phase_cfg,
                                                         block::ActionPhaseConfig* action_phase_cfg);
};
} // namespace emulator
