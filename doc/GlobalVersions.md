## 全球版本（Global Versions）

全局版本是通过`ConfigParam 8`指定的参数（[block.tlb](https://github.com/ton-blockchain/ton/blob/master/crypto/block/block.tlb#L595)）。
不同的全局版本启用TON区块链中的各种功能。

### 版本 4

#### 新的 TVM 指令
- `PREVMCBLOCKS`, `PREVKEYBLOCK`
- `GLOBALID`
- `HASHEXT(A)(R)`
- `ECRECOVER`
- `SENDMSG`
- `RUNVM`, `RUNVMX`
- `GASCONSUMED`
- `RIST255_...` 指令
- `BLS_...` 指令
- `P256_CHKSIGNS`, `P256_CHKSIGNU`

#### 除法
[除法指令](https://ton.org/docs/learn/tvm-instructions/instructions#52-division)可以在除法之前向中间值添加一个数（例如 `(xy+w)/z`）。

#### 栈操作
- `PICK`, `ROLL`, `ROLLREV`, `BLKSWX`, `REVX`, `DROPX`, `XCHGX`, `CHKDEPTH`, `ONLYTOPX`, `ONLYX` 的参数现在是无限的。
- 当参数很大时，`ROLL`, `ROLLREV`, `BLKSWX`, `REVX`, `ONLYTOPX` 消耗更多的Gas。

#### c7 元组
**c7** 元组从10个元素扩展到14个元素：
- **10**: 智能合约的代码。
- **11**: 收到消息的值。
- **12**: 存储阶段收集的费用。
- **13**: 以前区块的信息。

#### 行动阶段
- 如果“发送消息”操作失败，账户必须支付处理消息单元的费用。
- 在“发送消息”、“保留”、“更改库”操作中，如果操作失败，带有 +16 标志的操作会反弹（bounce）。

#### 存储阶段
- 未支付的存储费用现在保存到 `due_payment`。

### 版本 5

#### Gas 限制
版本 5 增加了特殊合约的Gas限制。
- 所有特殊合约交易的Gas限制设置为`ConfigParam 20`中的`special_gas_limit`（在写作时为35M）。以前，只有ticktock交易有这个限制，而普通交易的默认限制是 `gas_limit` Gas（1M）。
- 特殊合约的Gas使用在检查区块限制时不被考虑。这允许在保持主链区块限制较低的同时，为选举人提供较高的Gas限制。
- `EQD_v9j1rlsuHHw2FIhcsCFFSD367ldfDdCKcsNmNpIRzUlu` 的Gas限制增加到 `special_gas_limit * 2`，直到2024-02-29（[来源](https://t.me/tonstatus/88)）。

#### 加载库
- 加载“嵌套库”（即一个库单元指向另一个库单元）时会抛出异常。
- 加载库只消耗一次单元加载的Gas，而不是两次。
- `XLOAD` 返回库单元指向的单元，这允许在需要时加载嵌套库。

### 版本 6

#### c7 元组
**c7** 元组从14个元素扩展到17个元素：
- **14**: 包含一些配置参数的元组，以单元切片形式表示。
  - **0**: 来自`ConfigParam 18`的`StoragePrices`。
  - **1**: `ConfigParam 19`（全局ID）。
  - **2**: `ConfigParam 20`（主链Gas价格）。
  - **3**: `ConfigParam 21`（Gas价格）。
  - **4**: `ConfigParam 24`（主链转发费）。
  - **5**: `ConfigParam 25`（转发费）。
  - **6**: `ConfigParam 43`（尺寸限制）。
- **15**: `due_payment` - 当前存储费用的债务（纳吨）。
- **16**: 当前合约的预编译Gas使用量（如果是预编译的，根据`ConfigParam 45`）。

#### 新的 TVM 指令

##### 费用计算
- `GETGASFEE`: 计算Gas费用。
- `GETSTORAGEFEE`: 计算存储费用。
- `GETFORWARDFEE`: 计算转发费用。
- `GETPRECOMPILEDGAS`: 返回当前合约的预编译Gas使用量（如果是预编译的）。
- `GETORIGINALFWDFEE`: 计算消息的原始转发费。
- `GETGASFEESIMPLE`: 计算没有固定价格的Gas费用。
- `GETFORWARDFEESIMPLE`: 计算没有总价格的转发费用。

##### 单元操作
用于处理Merkle证明的操作：
- `CLEVEL`: 返回单元的级别。
- `CLEVELMASK`: 返回单元的级别掩码。
- `i CHASHI`: 返回单元的第 `i` 个哈希值。
- `i CDEPTHI`: 返回单元的第 `i` 个深度。
- `CHASHIX`: 返回单元的第 `i` 个哈希值。
- `CDEPTHIX`: 返回单元的第 `i` 个深度。

#### 其他更改
- `GLOBALID` 从元组中获取 `ConfigParam 19`，减少Gas使用。
- `SENDMSG` 从元组中获取 `ConfigParam 24/25`（消息价格），并使用 `ConfigParam 43` 获取最大消息单元。

### 版本 7

[明确归零](https://github.com/ton-blockchain/ton/pull/957/files) `due_payment` 在支付到期债务后。

### 版本 8

- 在无效的 `action_send_msg` 上检查模式。如果设置了 `IGNORE_ERROR` (+2) 位，忽略操作；如果设置了 `BOUNCE_ON_FAIL` (+16) 位，则反弹（bounce）。
- 更改随机种子生成以修复 `addr_rewrite` 和 `addr` 的混合问题。
- 对于无法发送的无效和有效消息，将 `IGNORE_ERROR` 模式的 `skipped_actions` 填写。
- 允许通过外部消息解冻。

