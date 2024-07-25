## 2024年6月更新

1. **将Jemalloc设为默认内存分配器**
2. **新增候选广播和缓存**
3. **限制单个地址外部消息广播的速度，设定为合理的高值**
4. **覆盖改进**：修复在小型自定义覆盖网络中丢失对等节点的问题，修复在错过密钥区块时使用错误证书的问题
5. **扩展的统计数据和日志**：用于Celldb的使用、会话统计和持久状态序列化
6. **Tonlib和浏览器修复**
7. **Celldb的精确控制标志**：`--celldb-cache-size`、`--celldb-direct-io` 和 `--celldb-preload-all`
8. **新增validator-console命令**：停止持久状态序列化
9. **使用`@`作为路径分隔符**（仅适用于Windows上的fift和create-state工具）

## 2024年4月更新

1. **模拟器**：新增单次调用优化的runGetMethod方法
2. **Tonlib**：系列改进，包含对`liteServer.getAllShardsInfo`方法的重大更改（详见下文）
3. **数据库**：现在收集使用统计数据，不再序列化过时的持久状态
4. **LiteServer**：新增快速`getOutMsgQueueSizes`方法，预支持非最终区块请求
5. **网络**：块候选人的lz4压缩（默认禁用）
6. **覆盖网络**：新增自定义覆盖网络
7. **交易执行器**：修复了due_payment收集问题

`liteServer.getAllShardsInfo`方法更新以提高效率。此前，proof字段包含两个根的BoC：一个来自块根的BlockState，另一个来自BlockState的ShardHashes。现在，它返回单根proof BoC，具体来说是直接来自块根的ShardHashes的默克尔证明，简化了数据访问和完整性检查。检查proof需要验证`data`中的ShardHashes是否与块中的ShardHashes一致。

此外，这次更新还基于@akifoq的努力解决了due_payment问题。

## 2024年3月更新

1. **准备性代码（尚未启用）**：用于预编译智能合约
2. **与费用相关的操作码的小修**

## 2024年2月更新

1. **改进验证器同步**：
   * 更好地处理区块广播 -> 更快的同步
   * 额外的独立覆盖网络作为同步的第二选择
2. **LiteServer改进**：
   * 完整填充c7和库上下文用于服务器端的rungetmethod
   * runmethods和成功的外部消息缓存
   * 记录LiteServer请求统计数据
3. **精确控制打开的文件**：
   * 几乎即时的验证器启动
   * `--max-archive-fd`选项
   * 自动删除未使用的临时存档文件
   * `--archive-preload-period`选项
4. **准备性代码（尚未启用）**：用于新增的TVM指令，以便在链上降低费用计算

## 2024年1月更新

1. **修正交易中特殊账户的gas计算问题**。此前，gas按常规计算，因此为了进行成本超过30m gas的选举，主链块限制设为37m gas。为安全考虑，提议对特殊账户的交易单独计算gas。此外，对于所有类型的特殊账户交易，将`gas_max`设为`special_gas_limit`。新行为通过设置`ConfigParam 8`中的`version >= 5`来激活。
   * 此外，通过更新配置临时将地址`EQD_v9j1rlsuHHw2FIhcsCFFSD367ldfDdCKcsNmNpIRzUlu`的gas上限设置为`special_gas_limit`，详见[详情](https://t.me/tonstatus/88)。
2. **LiteServer行为改进**
   * 改进所有分片应用状态的检测，以减少“块未应用”错误率
   * 更好的错误日志：分离“块不在数据库中”和“块未应用”错误
   * 修复合并后块的proof生成错误
   * 修复与Proofs中发送过于近期的块相关的大部分“块未应用”问题
   * LiteServer现在在`accept_message`（`set_gas`）之前检查外部消息。
3. **改进DHT工作和存储**、Celldb、config.json修正、对等行为不良检测、验证器会话统计收集、模拟器。
4. **CTOS和XLOAD行为的更改**通过在`ConfigParam 8`中设置`version >= 5`激活：
   * 加载“嵌套库”（即指向另一个库单元的库单元）会抛出异常。
   * 加载库只为库单元加载一次单元，而不是两次（库单元和库中的单元）。
   * `XLOAD`现在有不同的工作方式。当它获取一个库单元时，它返回指向的单元。这允许在需要时加载“嵌套库”。

此外，这次更新还基于@XaBbl4（对等行为不良检测）和@akifoq（CTOS行为和特殊账户的gas限制方案）的努力。

## 2023年12月更新

1. **优化消息队列处理**，现在队列清理速度不再取决于总队列大小
   * 使用lt增强而非随机搜索/连续遍历清理已交付消息
   * 将队列消息的根单元保存在内存中直到过时（缓存）
2. **区块整合/验证限制的变更**
3. **当消息队列过载时停止接受新的外部消息**
4. **根据队列大小设置分片拆分/合并条件**

阅读更多[更新信息](https://blog.ton.org/technical-report-december-5-inscriptions-launch-on-ton)。

## 2023年11月更新

1. **新增TVM功能**（默认禁用）
2. **模拟器改进**：支持库、更高的最大堆栈大小等
3. **Tonlib和tonlib-cli改进**：支持wallet-v4、getconfig、showtransactions等
4. **公共库变更**：现在合约不能发布超过256个库（配置参数），合约不能在initstate中使用公共库（需要显式发布所有库）
5. **存储到期支付变更**：现在到期支付在存储阶段收集，但对于可反弹的消息，费用金额不能超过消息前账户余额

此外，这次更新还基于@aleksej-paschenko（模拟器改进）、@akifoq（安全改进）、Trail of Bits审计员以及所有[TEP-88讨论](https://github.com/ton-blockchain/TEPs/pull/88)的参与者的努力。

## 2023年10月更新
1. **节点的一系列额外安全检查**：操作列表中的特殊单元、外部消息中的初始状态、保存到磁盘之前的对等数据。
2. **浏览器中人类可读的时间戳**

此外，这次更新还基于@akifoq和@mr-tron的努力。

## 2023年6月更新
1. （默认禁用）**新的通货紧缩机制**：部分费用燃烧和黑洞地址
2. **存储合约改进**

此外，这次更新还基于Tonbyte的@DearJohnDoe（存储合约改进）的努力。

## 2023年5月更新
1. **归档管理器优化**
2. **一系列catchain（基本共识协议）安全改进**
3. **Fift库和FunC更新**：更好的错误处理，修复`catch`堆栈恢复
4. **一系列消息队列处理优化**（已经在紧急升级中部署）
5. **改进二进制文件的可移植性**

此外，这次更新还基于@aleksej-paschenko（可移植性改进）、[Disintar团队](https://github.com/disintar/)（归档管理器优化）和[sec3-service](https://github.com/sec3-service)安全审计员（funC改进）的努力。

## 2023年4月更新
1. **CPU负载优化**：以前的DHT重新连接策略过于激进
2. **网络吞吐量改进**：对外部消息广播的精细控制，优化celldb GC，调整状态序列化和块下载时间，rldp2用于状态和归档 
3. **Fift更新**（命名空间）和Fift库更新（改进列表：https://github.com/ton-blockchain/ton/issues/631）
4. **更好地处理funC中的不正确输入**：修复未定义行为并防止某些
