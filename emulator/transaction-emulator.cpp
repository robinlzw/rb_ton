#include <string>
#include "transaction-emulator.h"
#include "crypto/common/refcnt.hpp"
#include "vm/vm.h"
#include "tdutils/td/utils/Time.h"

using td::Ref;
using namespace std::string_literals;

namespace emulator {

// 模拟一笔交易，并返回交易结果
td::Result<std::unique_ptr<TransactionEmulator::EmulationResult>> TransactionEmulator::emulate_transaction(
    block::Account&& account, td::Ref<vm::Cell> msg_root, ton::UnixTime utime, ton::LogicalTime lt, int trans_type) {

    // 存储旧的主参数和配置数据
    td::Ref<vm::Cell> old_mparams;
    std::vector<block::StoragePrices> storage_prices;
    block::StoragePhaseConfig storage_phase_cfg{&storage_prices};
    block::ComputePhaseConfig compute_phase_cfg;
    block::ActionPhaseConfig action_phase_cfg;
    td::RefInt256 masterchain_create_fee, basechain_create_fee;
    
    // 如果没有提供Unix时间，则使用默认的成员变量unixtime_或当前时间
    if (!utime) {
        utime = unixtime_;
    }
    if (!utime) {
        utime = (unsigned)std::time(nullptr);
    }

    // 获取区块链配置参数
    auto fetch_res = block::FetchConfigParams::fetch_config_params(config_, prev_blocks_info_, &old_mparams,
                                                                   &storage_prices, &storage_phase_cfg,
                                                                   &rand_seed_, &compute_phase_cfg,
                                                                   &action_phase_cfg, &masterchain_create_fee,
                                                                   &basechain_create_fee, account.workchain, utime);
    // 如果获取配置失败，返回错误信息
    if(fetch_res.is_error()) {
        return fetch_res.move_as_error_prefix("cannot fetch config params ");
    }

    // 初始化虚拟机环境
    TRY_STATUS(vm::init_vm(debug_enabled_));

    // 如果没有提供逻辑时间，则使用默认的成员变量lt_或计算得到的值
    if (!lt) {
        lt = lt_;
    }
    if (!lt) {
        lt = (account.last_trans_lt_ / block::ConfigInfo::get_lt_align() + 1) * block::ConfigInfo::get_lt_align(); // 在上一个交易后的下一块
    }
    // 设置账户的块逻辑时间
    account.block_lt = lt - lt % block::ConfigInfo::get_lt_align();

    // 配置计算阶段的虚拟机字典、签名忽略标志、日志配置等
    compute_phase_cfg.libraries = std::make_unique<vm::Dictionary>(libraries_);
    compute_phase_cfg.ignore_chksig = ignore_chksig_;
    compute_phase_cfg.with_vm_log = true;
    compute_phase_cfg.vm_log_verbosity = vm_log_verbosity_;

    // 记录模拟开始时间
    double start_time = td::Time::now();
    // 创建交易
    auto res = create_transaction(msg_root, &account, utime, lt, trans_type,
                                                    &storage_phase_cfg, &compute_phase_cfg,
                                                    &action_phase_cfg);
    double elapsed = td::Time::now() - start_time;

    // 如果创建交易失败，返回错误信息
    if(res.is_error()) {
        return res.move_as_error_prefix("cannot run message on account ");
    }
    std::unique_ptr<block::transaction::Transaction> trans = res.move_as_ok();

    // 如果计算阶段未被接受且为外部消息，则返回不被接受的结果
    if (!trans->compute_phase->accepted && trans->in_msg_extern) {
        auto vm_log = trans->compute_phase->vm_log;
        auto vm_exit_code = trans->compute_phase->exit_code;
        return std::make_unique<TransactionEmulator::EmulationExternalNotAccepted>(std::move(vm_log), vm_exit_code, elapsed);
    }

    // 序列化交易，若失败则返回错误
    if (!trans->serialize()) {
        return td::Status::Error(-669,"cannot serialize new transaction for smart contract "s + trans->account.addr.to_hex());
    }

    // 提交交易，若失败则返回错误
    auto trans_root = trans->commit(account);
    if (trans_root.is_null()) {
        return td::Status::Error(PSLICE() << "cannot commit new transaction for smart contract");
    }

    // 返回模拟成功结果，包括交易根、账户、VM日志和动作列表
    return std::make_unique<TransactionEmulator::EmulationSuccess>(std::move(trans_root), std::move(account), 
      std::move(trans->compute_phase->vm_log), std::move(trans->compute_phase->actions), elapsed);
}

// 模拟现有交易，检查交易的一致性
td::Result<TransactionEmulator::EmulationSuccess> TransactionEmulator::emulate_transaction(block::Account&& account, td::Ref<vm::Cell> original_trans) {

    block::gen::Transaction::Record record_trans;
    // 解包原始交易，如果失败返回错误
    if (!tlb::unpack_cell(original_trans, record_trans)) {
        return td::Status::Error("Failed to unpack Transaction");
    }

    // 设置逻辑时间和Unix时间
    ton::LogicalTime lt = record_trans.lt;
    ton::UnixTime utime = record_trans.now;
    account.now_ = utime;
    account.block_lt = record_trans.lt - record_trans.lt % block::ConfigInfo::get_lt_align();
    td::Ref<vm::Cell> msg_root = record_trans.r1.in_msg->prefetch_ref();
    // 获取交易描述符的标识
    int tag = block::gen::t_TransactionDescr.get_tag(vm::load_cell_slice(record_trans.description));

    // 确定交易类型
    int trans_type = block::transaction::Transaction::tr_none;
    switch (tag) {
        case block::gen::TransactionDescr::trans_ord: {
            trans_type = block::transaction::Transaction::tr_ord;
            break;
        }
        case block::gen::TransactionDescr::trans_storage: {
            trans_type = block::transaction::Transaction::tr_storage;
            break;
        }
        case block::gen::TransactionDescr::trans_tick_tock: {
            block::gen::TransactionDescr::Record_trans_tick_tock tick_tock;
            if (!tlb::unpack_cell(record_trans.description, tick_tock)) {
                return td::Status::Error("Failed to unpack tick tock transaction description");
            }
            trans_type = tick_tock.is_tock ? block::transaction::Transaction::tr_tock : block::transaction::Transaction::tr_tick;
            break;
        }
        case block::gen::TransactionDescr::trans_split_prepare: {
            trans_type = block::transaction::Transaction::tr_split_prepare;
            break;
        }
        case block::gen::TransactionDescr::trans_split_install: {
            trans_type = block::transaction::Transaction::tr_split_install;
            break;
        }
        case block::gen::TransactionDescr::trans_merge_prepare: {
            trans_type = block::transaction::Transaction::tr_merge_prepare;
            break;
        }
        case block::gen::TransactionDescr::trans_merge_install: {
            trans_type = block::transaction::Transaction::tr_merge_install;
            break;
        }
    }

    // 模拟交易
    TRY_RESULT(emulation, emulate_transaction(std::move(account), msg_root, utime, lt, trans_type));

    auto emulation_result = dynamic_cast<EmulationSuccess&>(*emulation);
    // 检查交易哈希是否匹配
    if (td::Bits256(emulation_result.transaction->get_hash().bits()) != td::Bits256(original_trans->get_hash().bits())) {
        return td::Status::Error("transaction hash mismatch");
    }

    // 检查账户状态是否更新
    if (!check_state_update(emulation_result.account, record_trans)) {
        return td::Status::Error("account hash mismatch");
    }

    return emulation_result;
}

// 模拟一系列交易，并返回模拟链结果
td::Result<TransactionEmulator::EmulationChain> TransactionEmulator::emulate_transactions_chain(block::Account&& account, std::vector<td::Ref<vm::Cell>>&& original_transactions) {

    std::vector<td::Ref<vm::Cell>> emulated_transactions;
    // 对每个原始交易进行模拟，并收集模拟后的交易
    for (const auto& original_trans : original_transactions) {
        if (original_trans.is_null()) {
            continue;
        }

        TRY_RESULT(emulation_result, emulate_transaction(std::move(account), original_trans));
        emulated_transactions.push_back(std::move(emulation_result.transaction));
        account = std::move(emulation_result.account);
    }

    // 返回模拟链结果，包括模拟后的交易和最终账户状态
    return TransactionEmulator::EmulationChain{ std::move(emulated_transactions), std::move(account) };
}

// 检查账户状态是否更新
bool TransactionEmulator::check_state_update(const block::Account& account, const block::gen::Transaction::Record& trans) {
    block::gen::HASH_UPDATE::Record hash_update;
    return tlb::type_unpack_cell(trans.state_update, block::gen::t_HASH_UPDATE_Account, hash_update) &&
        hash_update.new_hash == account.total_state->get_hash().bits();
}

// 创建新的交易
td::Result<std::unique_ptr<block::transaction::Transaction>> TransactionEmulator::create_transaction(
                                                         td::Ref<vm::Cell> msg_root, block::Account* acc,
                                                         ton::UnixTime utime, ton::LogicalTime lt, int trans_type,
                                                         block::StoragePhaseConfig* storage_phase_cfg,
                                                         block::ComputePhaseConfig* compute_phase_cfg,
                                                         block::ActionPhaseConfig* action_phase_cfg) {
  bool external{false}, ihr_delivered{false}, need_credit_phase{false};

  // 如果消息根单元不为空，加载并检查是否为外部消息
  if (msg_root.not_null()) {
    auto cs = vm::load_cell_slice(msg_root);
    external = block::gen::t_CommonMsgInfo.get_tag(cs);
  }

  // 根据交易类型确定是否需要信用阶段
  if (trans_type == block::transaction::Transaction::tr_ord) {
    need_credit_phase = !external;
  } else if (trans_type == block::transaction::Transaction::tr_merge_install) {
    need_credit_phase = true;
  }

  // 创建一个新的交易实例
  std::unique_ptr<block::transaction::Transaction> trans =
      std::make_unique<block::transaction::Transaction>(*acc, trans_type, lt, utime, msg_root);

  // 如果消息根单元不为空，解包输入消息，并根据需要进行处理
  if (msg_root.not_null() && !trans->unpack_input_msg(ihr_delivered, action_phase_cfg)) {
    if (external) {
      // 如果是外部消息且未被接受，返回错误
      return td::Status::Error(-701,"inbound external message rejected by account "s + acc->addr.to_hex() +
                                                           " before smart-contract execution");
    }
    // 解包输入消息失败，返回错误
    return td::Status::Error(-669,"cannot unpack input message for a new transaction");
  }

  // 根据交易是否开启了回弹阶段，准备存储阶段和信用阶段
  if (trans->bounce_enabled) {
    if (!trans->prepare_storage_phase(*storage_phase_cfg, true)) {
      // 准备存储阶段失败，返回错误
      return td::Status::Error(-669,"cannot create storage phase of a new transaction for smart contract "s + acc->addr.to_hex());
    }
    if (need_credit_phase && !trans->prepare_credit_phase()) {
      // 准备信用阶段失败，返回错误
      return td::Status::Error(-669,"cannot create credit phase of a new transaction for smart contract "s + acc->addr.to_hex());
    }
  } else {
    if (need_credit_phase && !trans->prepare_credit_phase()) {
      // 准备信用阶段失败，返回错误
      return td::Status::Error(-669,"cannot create credit phase of a new transaction for smart contract "s + acc->addr.to_hex());
    }
    if (!trans->prepare_storage_phase(*storage_phase_cfg, true, need_credit_phase)) {
      // 准备存储阶段失败，返回错误
      return td::Status::Error(-669,"cannot create storage phase of a new transaction for smart contract "s + acc->addr.to_hex());
    }
  }

  // 准备计算阶段，如果失败返回错误
  if (!trans->prepare_compute_phase(*compute_phase_cfg)) {
    return td::Status::Error(-669,"cannot create compute phase of a new transaction for smart contract "s + acc->addr.to_hex());
  }

  // 如果计算阶段未被接受，且不是外部消息且跳过原因未设置，则返回错误
  if (!trans->compute_phase->accepted) {
    if (!external && trans->compute_phase->skip_reason == block::ComputePhase::sk_none) {
      return td::Status::Error(-669,"new ordinary transaction for smart contract "s + acc->addr.to_hex() +
                " has not been accepted by the smart contract (?)");
    }
  }

  // 如果计算阶段成功，准备动作阶段；如果需要回弹阶段且计算阶段未成功，准备回弹阶段
  if (trans->compute_phase->success && !trans->prepare_action_phase(*action_phase_cfg)) {
    return td::Status::Error(-669,"cannot create action phase of a new transaction for smart contract "s + acc->addr.to_hex());
  }

  if (trans->bounce_enabled && !trans->compute_phase->success && !trans->prepare_bounce_phase(*action_phase_cfg)) {
    return td::Status::Error(-669,"cannot create bounce phase of a new transaction for smart contract "s + acc->addr.to_hex());
  }

  return trans;
}

// 设置Unix时间
void TransactionEmulator::set_unixtime(ton::UnixTime unixtime) {
  unixtime_ = unixtime;
}

// 设置逻辑时间
void TransactionEmulator::set_lt(ton::LogicalTime lt) {
  lt_ = lt;
}

// 设置随机种子
void TransactionEmulator::set_rand_seed(td::BitArray<256>& rand_seed) {
  rand_seed_ = rand_seed;
}

// 设置是否忽略检查签名标志
void TransactionEmulator::set_ignore_chksig(bool ignore_chksig) {
  ignore_chksig_ = ignore_chksig;
}

// 设置配置
void TransactionEmulator::set_config(block::Config &&config) {
  config_ = std::forward<block::Config>(config);
}

// 设置虚拟机库
void TransactionEmulator::set_libs(vm::Dictionary &&libs) {
  libraries_ = std::forward<vm::Dictionary>(libs);
}

// 设置调试模式标志
void TransactionEmulator::set_debug_enabled(bool debug_enabled) {
  debug_enabled_ = debug_enabled;
}

// 设置前一个区块信息
void TransactionEmulator::set_prev_blocks_info(td::Ref<vm::Tuple> prev_blocks_info) {
  prev_blocks_info_ = std::move(prev_blocks_info);
}

} // namespace emulator
