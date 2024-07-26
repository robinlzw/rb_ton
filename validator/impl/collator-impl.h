/*
    This file is part of TON Blockchain Library.

    TON Blockchain Library is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    TON Blockchain Library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with TON Blockchain Library.  If not, see <http://www.gnu.org/licenses/>.

    Copyright 2017-2020 Telegram Systems LLP
*/
#pragma once
#include "interfaces/validator-manager.h"
#include "shard.hpp"
#include "top-shard-descr.hpp"
#include "common/refcnt.hpp"
#include "vm/cells.h"
#include "vm/dict.h"
#include "block/mc-config.h"
#include "block/block.h"
#include "block/transaction.h"
#include "block/block-db.h"
#include "block/output-queue-merger.h"
#include "vm/cells/MerkleProof.h"
#include "vm/cells/MerkleUpdate.h"
#include <map>
#include <queue>
#include "common/global-version.h"

namespace ton {

namespace validator {
using td::Ref;

class Collator final : public td::actor::Actor {
  static constexpr int supported_version() {
    return SUPPORTED_VERSION;
  }
  static constexpr long long supported_capabilities() {
    return ton::capCreateStatsEnabled | ton::capBounceMsgBody | ton::capReportVersion | ton::capShortDequeue;
  }
  using LtCellRef = block::LtCellRef;
  using NewOutMsg = block::NewOutMsg;
  const ShardIdFull shard_;
  ton::BlockId new_id;
  bool busy_{false};
  bool before_split_{false};
  bool after_split_{false};
  bool after_merge_{false};
  bool want_split_{false};
  bool want_merge_{false};
  bool right_child_{false};
  bool preinit_complete{false};
  bool is_key_block_{false};
  bool block_full_{false};
  bool inbound_queues_empty_{false};
  bool libraries_changed_{false};
  bool prev_key_block_exists_{false};
  bool is_hardfork_{false};
  UnixTime min_ts;
  BlockIdExt min_mc_block_id;
  std::vector<BlockIdExt> prev_blocks;
  std::vector<Ref<ShardState>> prev_states;
  std::vector<Ref<BlockData>> prev_block_data;
  Ed25519_PublicKey created_by_;
  Ref<ValidatorSet> validator_set_;
  td::actor::ActorId<ValidatorManager> manager;
  td::Timestamp timeout;
  td::Timestamp queue_cleanup_timeout_, soft_timeout_, medium_timeout_;
  td::Promise<BlockCandidate> main_promise;
  ton::BlockSeqno last_block_seqno{0};
  ton::BlockSeqno prev_mc_block_seqno{0};
  ton::BlockSeqno new_block_seqno{0};
  ton::BlockSeqno prev_key_block_seqno_{0};
  int step{0};
  int pending{0};
  static constexpr int max_ihr_msg_size = 65535;   // 64k
  static constexpr int max_ext_msg_size = 65535;   // 64k
  static constexpr int max_blk_sign_size = 65535;  // 64k
  static constexpr bool shard_splitting_enabled = true;

 public:
  Collator(ShardIdFull shard, bool is_hardfork, td::uint32 min_ts, BlockIdExt min_masterchain_block_id,
           std::vector<BlockIdExt> prev, Ref<ValidatorSet> validator_set, Ed25519_PublicKey collator_id,
           td::actor::ActorId<ValidatorManager> manager, td::Timestamp timeout, td::Promise<BlockCandidate> promise);
  ~Collator() override = default;
  bool is_busy() const {
    return busy_;
  }
  ShardId get_shard() const {
    return shard_.shard;
  }
  WorkchainId workchain() const {
    return shard_.workchain;
  }
  static constexpr td::uint32 priority() {
    return 2;
  }

  static td::Result<std::unique_ptr<block::transaction::Transaction>>
                        impl_create_ordinary_transaction(Ref<vm::Cell> msg_root,
                                                         block::Account* acc,
                                                         UnixTime utime, LogicalTime lt,
                                                         block::StoragePhaseConfig* storage_phase_cfg,
                                                         block::ComputePhaseConfig* compute_phase_cfg,
                                                         block::ActionPhaseConfig* action_phase_cfg,
                                                         bool external, LogicalTime after_lt);

 private:
  void start_up() override;
  void alarm() override;
  int verbosity{3 * 0};
  int verify{1};
  ton::LogicalTime start_lt, max_lt;
  ton::UnixTime now_;
  ton::UnixTime prev_now_;
  ton::UnixTime now_upper_limit_{~0U};
  unsigned out_msg_queue_ops_{}, in_descr_cnt_{}, out_descr_cnt_{};
  Ref<MasterchainStateQ> mc_state_;
  Ref<BlockData> prev_mc_block;
  BlockIdExt mc_block_id_;
  Ref<vm::Cell> mc_state_root;
  Ref<vm::Cell> mc_block_root;
  td::BitArray<256> rand_seed_ = td::Bits256::zero();
  std::unique_ptr<block::ConfigInfo> config_;
  std::unique_ptr<block::ShardConfig> shard_conf_;
  std::map<BlockSeqno, Ref<MasterchainStateQ>> aux_mc_states_;
  std::vector<block::McShardDescr> neighbors_;
  std::unique_ptr<block::OutputQueueMerger> nb_out_msgs_;
  std::vector<ton::StdSmcAddress> special_smcs;
  std::vector<std::pair<ton::StdSmcAddress, int>> ticktock_smcs;
  Ref<vm::Cell> prev_block_root;
  Ref<vm::Cell> prev_state_root_, prev_state_root_pure_;
  Ref<vm::Cell> state_root;                              // (new) shardchain state
  Ref<vm::Cell> state_update;                            // Merkle update from prev_state_root to state_root
  std::shared_ptr<vm::CellUsageTree> state_usage_tree_;  // used to construct Merkle update
  Ref<vm::CellSlice> new_config_params_;
  Ref<vm::Cell> old_mparams_;
  ton::LogicalTime prev_state_lt_;
  ton::LogicalTime shards_max_end_lt_{0};
  ton::UnixTime prev_state_utime_;
  int global_id_{0};
  ton::BlockSeqno min_ref_mc_seqno_{~0U};
  ton::BlockSeqno vert_seqno_{~0U}, prev_vert_seqno_{~0U};
  ton::BlockIdExt prev_key_block_;
  ton::LogicalTime prev_key_block_lt_;
  bool accept_msgs_{true};
  bool shard_conf_adjusted_{false};
  bool ihr_enabled_{false};
  bool create_stats_enabled_{false};
  bool report_version_{false};
  bool skip_topmsgdescr_{false};
  bool skip_extmsg_{false};
  bool short_dequeue_records_{false};
  td::uint64 overload_history_{0}, underload_history_{0};
  td::uint64 block_size_estimate_{};
  Ref<block::WorkchainInfo> wc_info_;
  std::vector<Ref<ShardTopBlockDescription>> shard_block_descr_;
  std::vector<Ref<ShardTopBlockDescrQ>> used_shard_block_descr_;
  std::unique_ptr<vm::Dictionary> shard_libraries_;
  Ref<vm::Cell> mc_state_extra_;
  std::unique_ptr<vm::AugmentedDictionary> account_dict;
  std::map<ton::StdSmcAddress, std::unique_ptr<block::Account>> accounts;
  std::vector<block::StoragePrices> storage_prices_;
  block::StoragePhaseConfig storage_phase_cfg_{&storage_prices_};
  block::ComputePhaseConfig compute_phase_cfg_;
  block::ActionPhaseConfig action_phase_cfg_;
  td::RefInt256 masterchain_create_fee_, basechain_create_fee_;
  std::unique_ptr<block::BlockLimits> block_limits_;
  std::unique_ptr<block::BlockLimitStatus> block_limit_status_;
  ton::LogicalTime min_new_msg_lt{std::numeric_limits<td::uint64>::max()};
  block::CurrencyCollection total_balance_, old_total_balance_, total_validator_fees_;
  block::CurrencyCollection global_balance_, old_global_balance_, import_created_{0};
  Ref<vm::Cell> recover_create_msg_, mint_msg_;
  Ref<vm::Cell> new_block;
  block::ValueFlow value_flow_{block::ValueFlow::SetZero()};
  std::unique_ptr<vm::AugmentedDictionary> fees_import_dict_;
  std::map<ton::Bits256, int> ext_msg_map;
  struct ExtMsg {
    Ref<vm::Cell> cell;
    ExtMessage::Hash hash;
    int priority;
  };
  std::vector<ExtMsg> ext_msg_list_;
  std::priority_queue<NewOutMsg, std::vector<NewOutMsg>, std::greater<NewOutMsg>> new_msgs;
  std::pair<ton::LogicalTime, ton::Bits256> last_proc_int_msg_, first_unproc_int_msg_;
  std::unique_ptr<vm::AugmentedDictionary> in_msg_dict, out_msg_dict, out_msg_queue_, sibling_out_msg_queue_;
  td::uint32 out_msg_queue_size_ = 0;
  std::unique_ptr<vm::Dictionary> ihr_pending;
  std::shared_ptr<block::MsgProcessedUptoCollection> processed_upto_, sibling_processed_upto_;
  std::unique_ptr<vm::Dictionary> block_create_stats_;
  std::map<td::Bits256, int> block_create_count_;
  unsigned block_create_total_{0};
  std::vector<ExtMessage::Hash> bad_ext_msgs_, delay_ext_msgs_;
  Ref<vm::Cell> shard_account_blocks_;  // ShardAccountBlocks
  std::vector<Ref<vm::Cell>> collated_roots_;
  std::unique_ptr<ton::BlockCandidate> block_candidate;

  td::PerfWarningTimer perf_timer_;
  //
  block::Account* lookup_account(td::ConstBitPtr addr) const;
  std::unique_ptr<block::Account> make_account_from(td::ConstBitPtr addr, Ref<vm::CellSlice> account,
                                                    bool force_create);
  td::Result<block::Account*> make_account(td::ConstBitPtr addr, bool force_create = false);
  td::actor::ActorId<Collator> get_self() {
    return actor_id(this);
  }
  bool init_utime();
  bool init_lt();
  bool fetch_config_params();
  bool fatal_error(td::Status error);
  bool fatal_error(int err_code, std::string err_msg);
  bool fatal_error(std::string err_msg, int err_code = -666);
  void check_pending();
  void after_get_mc_state(td::Result<std::pair<Ref<MasterchainState>, BlockIdExt>> res);
  void after_get_shard_state(int idx, td::Result<Ref<ShardState>> res);
  void after_get_block_data(int idx, td::Result<Ref<BlockData>> res);
  void after_get_shard_blocks(td::Result<std::vector<Ref<ShardTopBlockDescription>>> res);
  bool preprocess_prev_mc_state();
  bool register_mc_state(Ref<MasterchainStateQ> other_mc_state);
  bool request_aux_mc_state(BlockSeqno seqno, Ref<MasterchainStateQ>& state);
  Ref<MasterchainStateQ> get_aux_mc_state(BlockSeqno seqno) const;
  void after_get_aux_shard_state(ton::BlockIdExt blkid, td::Result<Ref<ShardState>> res);
  bool fix_one_processed_upto(block::MsgProcessedUpto& proc, const ton::ShardIdFull& owner);
  bool fix_processed_upto(block::MsgProcessedUptoCollection& upto);
  void got_neighbor_out_queue(int i, td::Result<Ref<MessageQueue>> res);
  void got_out_queue_size(size_t i, td::Result<td::uint32> res);
  bool adjust_shard_config();
  bool store_shard_fees(ShardIdFull shard, const block::CurrencyCollection& fees,
                        const block::CurrencyCollection& created);
  bool store_shard_fees(Ref<block::McShardHash> descr);
  bool import_new_shard_top_blocks();
  bool register_shard_block_creators(std::vector<td::Bits256> creator_list);
  bool init_block_limits();
  bool compute_minted_amount(block::CurrencyCollection& to_mint);
  bool init_value_create();
  bool try_collate();
  bool do_preinit();
  bool do_collate();
  bool create_special_transactions();
  bool create_special_transaction(block::CurrencyCollection amount, Ref<vm::Cell> dest_addr_cell,
                                  Ref<vm::Cell>& in_msg);
  bool create_ticktock_transactions(int mask);
  bool create_ticktock_transaction(const ton::StdSmcAddress& smc_addr, ton::LogicalTime req_start_lt, int mask);
  Ref<vm::Cell> create_ordinary_transaction(Ref<vm::Cell> msg_root, bool is_special_tx = false);
  bool check_cur_validator_set();
  bool unpack_last_mc_state();
  bool unpack_last_state();
  bool unpack_merge_last_state();
  bool unpack_one_last_state(block::ShardState& ss, BlockIdExt blkid, Ref<vm::Cell> prev_state_root);
  bool split_last_state(block::ShardState& ss);
  bool import_shard_state_data(block::ShardState& ss);
  bool add_trivial_neighbor();
  bool add_trivial_neighbor_after_merge();
  bool out_msg_queue_cleanup();
  bool dequeue_message(Ref<vm::Cell> msg_envelope, ton::LogicalTime delivered_lt);
  bool check_prev_block(const BlockIdExt& listed, const BlockIdExt& prev, bool chk_chain_len = true);
  bool check_prev_block_exact(const BlockIdExt& listed, const BlockIdExt& prev);
  bool check_this_shard_mc_info();
  bool request_neighbor_msg_queues();
  bool request_out_msg_queue_size();
  void update_max_lt(ton::LogicalTime lt);
  bool is_masterchain() const {
    return shard_.is_masterchain();
  }
  bool is_our_address(Ref<vm::CellSlice> addr_ref) const;
  bool is_our_address(ton::AccountIdPrefixFull addr_prefix) const;
  bool is_our_address(const ton::StdSmcAddress& addr) const;
  void after_get_external_messages(td::Result<std::vector<std::pair<Ref<ExtMessage>, int>>> res);
  td::Result<bool> register_external_message_cell(Ref<vm::Cell> ext_msg, const ExtMessage::Hash& ext_hash,
                                                  int priority);
  // td::Result<bool> register_external_message(td::Slice ext_msg_boc);
  void register_new_msg(block::NewOutMsg msg);
  void register_new_msgs(block::transaction::Transaction& trans);
  bool process_new_messages(bool enqueue_only = false);
  int process_one_new_message(block::NewOutMsg msg, bool enqueue_only = false, Ref<vm::Cell>* is_special = nullptr);
  bool process_inbound_internal_messages();
  bool process_inbound_message(Ref<vm::CellSlice> msg, ton::LogicalTime lt, td::ConstBitPtr key,
                               const block::McShardDescr& src_nb);
  bool process_inbound_external_messages();
  int process_external_message(Ref<vm::Cell> msg);
  bool enqueue_message(block::NewOutMsg msg, td::RefInt256 fwd_fees_remaining, ton::LogicalTime enqueued_lt);
  bool enqueue_transit_message(Ref<vm::Cell> msg, Ref<vm::Cell> old_msg_env, ton::AccountIdPrefixFull prev_prefix,
                               ton::AccountIdPrefixFull cur_prefix, ton::AccountIdPrefixFull dest_prefix,
                               td::RefInt256 fwd_fee_remaining);
  bool delete_out_msg_queue_msg(td::ConstBitPtr key);
  bool insert_in_msg(Ref<vm::Cell> in_msg);
  bool insert_out_msg(Ref<vm::Cell> out_msg);
  bool insert_out_msg(Ref<vm::Cell> out_msg, td::ConstBitPtr msg_hash);
  bool register_out_msg_queue_op(bool force = false);
  bool update_min_mc_seqno(ton::BlockSeqno some_mc_seqno);
  bool combine_account_transactions();
  bool update_public_libraries();
  bool update_account_public_libraries(Ref<vm::Cell> orig_libs, Ref<vm::Cell> final_libs, const td::Bits256& addr);
  bool add_public_library(td::ConstBitPtr key, td::ConstBitPtr addr, Ref<vm::Cell> library);
  bool remove_public_library(td::ConstBitPtr key, td::ConstBitPtr addr);
  bool check_block_overload();
  bool update_block_creator_count(td::ConstBitPtr key, unsigned shard_incr, unsigned mc_incr);
  int creator_count_outdated(td::ConstBitPtr key, vm::CellSlice& cs);
  bool update_block_creator_stats();
  bool create_mc_state_extra();
  bool create_shard_state();
  td::Result<Ref<vm::Cell>> get_config_data_from_smc(const ton::StdSmcAddress& cfg_addr);
  bool try_fetch_new_config(const ton::StdSmcAddress& cfg_addr, Ref<vm::Cell>& new_config);
  bool update_processed_upto();
  bool compute_out_msg_queue_info(Ref<vm::Cell>& out_msg_queue_info);
  bool compute_total_balance();
  bool store_master_ref(vm::CellBuilder& cb);
  bool store_prev_blk_ref(vm::CellBuilder& cb, bool after_merge);
  bool store_zero_state_ref(vm::CellBuilder& cb);
  bool store_version(vm::CellBuilder& cb) const;
  bool create_block_info(Ref<vm::Cell>& block_info);
  bool check_value_flow();
  bool create_block_extra(Ref<vm::Cell>& block_extra);
  bool update_shard_config(const block::WorkchainSet& wc_set, const block::CatchainValidatorsConfig& ccvc,
                           bool update_cc);
  bool create_mc_block_extra(Ref<vm::Cell>& mc_block_extra);
  bool create_block();
  Ref<vm::Cell> collate_shard_block_descr_set();
  bool create_collated_data();
  bool create_block_candidate();
  void return_block_candidate(td::Result<td::Unit> saved);
  bool update_last_proc_int_msg(const std::pair<ton::LogicalTime, ton::Bits256>& new_lt_hash);

 public:
  static td::uint32 get_skip_externals_queue_size();
};

}  // namespace validator

}  // namespace ton

/*

`Collator` 类是 TON 区块链中用于区块生成的核心组件之一。它负责收集交易、处理消息、生成新区块，并确保新区块符合区块链的规则和状态。以下是 `Collator` 类的主要特点和功能：

### 成员变量
- `shard_`: 表示当前工作链和分片的标识。
- `new_id`: 新生成区块的 ID。
- `busy_`: 表示是否正在生成区块。
- `before_split_`、`after_split_`、`after_merge_`: 表示分片状态，用于处理分片合并和分裂。
- `want_split_`、`want_merge_`: 表示是否希望进行分片分裂或合并。
- `right_child_`: 表示是否是分裂后右侧的子分片。
- `preinit_complete`: 表示预初始化是否完成。
- `is_key_block_`: 表示是否是关键区块。
- `block_full_`: 表示区块是否已满。
- `inbound_queues_empty_`: 表示是否所有传入队列都为空。
- `libraries_changed_`: 表示库是否已更改。
- `prev_key_block_exists_`: 表示是否存在前一个关键区块。
- `is_hardfork_`: 表示是否是硬分叉。
- `min_ts`: 最小时间戳。
- `min_mc_block_id`: 主链区块 ID 的最小值。
- `prev_blocks`: 前一个区块的 ID 列表。
- `prev_states`: 前一个分片区块的状态列表。
- `prev_block_data`: 前一个区块的数据列表。
- `created_by_`: 创建者的公钥。
- `validator_set_`: 验证者集合。
- `manager`: 验证者管理器的 Actor ID。
- `timeout`: 超时时间戳。
- `queue_cleanup_timeout_`、`soft_timeout_`、`medium_timeout_`: 不同的超时设置。
- `main_promise`: 主要承诺，用于返回生成的区块候选。
- `last_block_seqno`、`prev_mc_block_seqno`、`new_block_seqno`、`prev_key_block_seqno_`: 不同区块序列号。
- `step`、`pending`: 用于控制生成区块的步骤和挂起的操作。

### 静态成员函数
- `supported_version()`: 返回支持的版本。
- `supported_capabilities()`: 返回支持的功能。

### 构造函数
- `Collator(ShardIdFull shard, bool is_hardfork, td::uint32 min_ts, BlockIdExt min_masterchain_block_id, std::vector<BlockIdExt> prev, Ref<ValidatorSet> validator_set, Ed25519_PublicKey collator_id, td::actor::ActorId<ValidatorManager> manager, td::Timestamp timeout, td::Promise<BlockCandidate> promise)`: 构造函数，初始化 Collator 对象。

### 成员函数
- `is_busy()`: 检查是否正在生成区块。
- `get_shard()`: 获取分片 ID。
- `workchain()`: 获取工作链 ID。
- `priority()`: 获取优先级。

### 私有成员函数
- `start_up()`: 启动时调用，初始化 Collator。
- `alarm()`: 处理超时。
- `init_utime()`: 初始化时间戳。
- `init_lt()`: 初始化逻辑时间。
- `fetch_config_params()`: 获取配置参数。
- `fatal_error()`: 处理致命错误。
- `check_pending()`: 检查挂起的操作。
- `after_get_mc_state()`: 处理获取主链状态后的操作。
- `after_get_shard_state()`: 处理获取分片状态后的操作。
- `after_get_block_data()`: 处理获取区块数据后的操作。
- `after_get_shard_blocks()`: 处理获取分片区块描述后的操作。
- `preprocess_prev_mc_state()`: 预处理前一个主链状态。
- `register_mc_state()`: 注册主链状态。
- `request_aux_mc_state()`: 请求辅助主链状态。
- `get_aux_mc_state()`: 获取辅助主链状态。
- `fix_one_processed_upto()`: 修复单个已处理的消息上限。
- `fix_processed_upto()`: 修复已处理的消息上限。
- `got_neighbor_out_queue()`: 获取邻居出队列。
- `got_out_queue_size()`: 获取出队列大小。
- `adjust_shard_config()`: 调整分片配置。
- `store_shard_fees()`: 存储分片费用。
- `import_new_shard_top_blocks()`: 导入新的分片区块顶部。
- `register_shard_block_creators()`: 注册分片区块创建者。
- `init_block_limits()`: 初始化区块限制。
- `compute_minted_amount()`: 计算要铸造的金额。
- `init_value_create()`: 初始化值创建。
- `try_collate()`: 尝试生成区块。
- `do_preinit()`: 执行预初始化。
- `do_collate()`: 执行区块生成。
- `create_special_transactions()`: 创建特殊交易。
- `create_ticktock_transactions()`: 创建 TickTock 交易。
- `create_ordinary_transaction()`: 创建普通交易。
- `check_cur_validator_set()`: 检查当前验证者集合。
- `unpack_last_mc_state()`: 解包最后一个主链状态。
- `unpack_last_state()`: 解包最后一个状态。
- `unpack_merge_last_state()`: 解包合并后的状态。
- `unpack_one_last_state()`: 解包一个状态。
- `split_last_state()`: 分割最后一个状态。
- `import_shard_state_data()`: 导入分片状态数据。
- `add_trivial_neighbor()`: 添加平凡的邻居。
- `out_msg_queue_cleanup()`: 出队列清理。
- `dequeue_message()`: 出队列消息。
- `check_prev_block()`: 检查前一个区块。
- `check_this_shard_mc_info()`: 检查此分片的主链信息。
- `request_neighbor_msg_queues()`: 请求邻居消息队列。
- `request_out_msg_queue_size()`: 请求出队列大小。
- `update_max_lt()`: 更新最大逻辑时间。
- `is_our_address()`: 检查是否是我们的地址。
- `after_get_external_messages()`: 获取外部消息后的操作。
- `register_external_message_cell()`: 注册外部消息单元。
- `register_new_msg()`: 注册新消息。
- `register_new_msgs()`: 注册新消息。
- `process_new_messages()`: 处理新消息。
- `process_inbound_internal_messages()`: 处理传入的内部消息。
- `process_inbound_external_messages()`: 处理传入的外部消息。
- `process_external_message()`: 处理外部消息。
- `enqueue_message()`: 入队列消息。
- `enqueue_transit_message()`: 入队列传输消息。
- `delete_out_msg_queue_msg()`: 删除出队列消息。
- `insert_in_msg()`: 插入传入消息。
- `insert_out_msg()`: 插入传出消息。
- `register_out_msg_queue_op()`: 注册出队列操作。
- `update_min_mc_seqno()`: 更新最小主链序列号。
- `combine_account_transactions()`: 组合账户交易。
- `update_public_libraries()`: 更新公共库。
- `add_public_library()`: 添加公共库。
- `remove_public_library()`: 移除公共库。
- `check_block_overload()`: 检查区块过载。
- `update_block_creator_count()`: 更新区块创建者计数。
- `update_block_creator_stats()`: 更新区块创建者统计。
- `create_mc_state_extra()`: 创建主链状态额外信息。
- `create_shard_state()`: 创建分片状态。
- `get_config_data_from_smc()`: 从智能合约获取配置数据。
- `try_fetch_new_config()`: 尝试获取新配置。
- `update_processed_upto()`: 更新已处理的消息上限。
- `compute_out_msg_queue_info()`: 计算传出消息队列信息。
- `compute_total_balance()`: 计算总余额。
- `store_master_ref()`: 存储主链引用。
- `store_prev_blk_ref()`: 存储前一个区块引用。
- `store_zero_state_ref()`: 存储零状态引用。
- `store_version()`: 存储版本。
- `create_block_info()`: 创建区块信息。
- `check_value_flow()`: 检查价值流。
- `create_block_extra()`: 创建区块额外信息。
- `update_shard_config()`: 更新分片配置。
- `create_mc_block_extra()`: 创建主链区块额外信息。
- `create_block()`: 创建区块。
- `collate_shard_block_descr_set()`: 收集分片区块描述集。
- `create_collated_data()`: 创建收集的数据。
- `create_block_candidate()`: 创建区块候选。
- `return_block_candidate()`: 返回区块候选。
- `update_last_proc_int_msg()`: 更新最后处理的内部消息。

*/

/*
在TON区块链中，`Collator` 类是一个核心组件，其主要作用是负责生成新的区块。以下是 `Collator` 类的关键职责和功能：

1. **区块组装**：`Collator` 收集交易和消息，并将它们组织成一个新的区块。

2. **状态管理**：它管理分片的状态，包括处理账户状态、交易执行结果和状态转换。

3. **交易处理**：`Collator` 处理传入的交易，包括验证交易的有效性、执行交易逻辑和更新账户余额。

4. **消息队列管理**：它管理消息队列，包括内部消息和外部消息，确保消息按正确的顺序被处理。

5. **分片和主链交互**：`Collator` 与主链交互，获取主链状态和配置，以确保分片区块的生成与主链保持一致。

6. **安全性保证**：通过验证交易和消息的签名，`Collator` 确保网络的安全性。

7. **区块验证**：在将区块添加到区块链之前，`Collator` 需要验证区块的有效性，包括检查区块的工作量证明（PoW）和区块头信息。

8. **硬分叉处理**：`Collator` 处理区块链的硬分叉，确保在区块链升级时区块的连续性和一致性。

9. **配置参数管理**：它从主链获取配置参数，并根据这些参数调整区块生成的逻辑。

10. **错误处理**：`Collator` 能够处理和报告在区块生成过程中出现的错误，确保系统的稳定性。

11. **性能监控**：通过监控区块生成过程中的性能指标，`Collator` 可以优化区块生成的效率。

12. **与验证者管理器交互**：`Collator` 与 `ValidatorManager` 交互，获取验证者集合信息，这对于区块的验证和确认至关重要。

13. **超时管理**：`Collator` 管理区块生成的超时，确保在预定时间内完成区块的生成。

总的来说，`Collator` 类在TON区块链中扮演着组装新区块、维护网络状态和推动区块链进展的关键角色。
它通过与区块链的其他组件（如主链、验证者集合、消息队列等）交互，确保区块链的稳定运行和数据的一致性。

*/

