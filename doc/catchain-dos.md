## Catchain DoS 保护

### 一、共识安全升级
由于Catchain协议描述了验证节点如何达成共识，因此其更新必须同时进行，以确保在任何时候至少有2/3的验证节点（按权重计算）使用相同版本的协议。TON网络中实现这一点的方式是使用配置参数，当某些配置参数的缺失或版本较旧时，会对应旧的行为，而新版本则控制更新后的Catchain协议行为。

因此，Catchain协议的更新应按以下方式进行：每个验证节点升级其节点软件并投票支持新的配置参数。该方法确保在超过3/4（即多于2/3）的验证节点升级后，才会切换到更新后的Catchain协议。

### 二、通过哈希覆盖区块依赖关系
Catchain协议版本由`ConfigParam 29`控制：在`consensus_config_v3`及更早的构造函数中通过`proto_version`字段（对于其他构造函数，`proto_version`等于0）。

对于版本不低于2的Catchain协议，哈希覆盖了Catchain区块依赖关系，这防止了转发依赖关系被篡改的区块。

### 三、限制Catchain区块的最大高度和大小
#### Catchain区块大小
除了`REJECT`消息外，所有Catchain消息的大小都有（并且一直有）限制。更新后，`REJECT`消息的大小也将限制为1024字节。同时，一个区块最多包含每轮消息的区块候选者。因此，16KB的Catchain区块大小限制足以防止DoS攻击。

#### 限制区块高度
另一种潜在的DoS攻击与恶意节点发送过多的Catchain区块有关。注意，限制每秒的最大区块数不是一个好的解决方案，因为这会阻碍节点断开连接后的同步。同时，Catchain组的存在时间非常短（大约几百秒），而区块的生成速度由“自然区块生成速度”和减少依赖关系大小生成的区块数决定。在任何情况下，区块总数由`catchain_lifetime * natural_block_production_speed * (1 + number_of_catchain_participants / max_dependencies_size)`限制。

为了防止DoS攻击，我们限制节点处理的最大区块高度为`catchain_lifetime * natural_block_production_speed * (1 + number_of_catchain_participants / max_dependencies_size)`，其中`catchain_lifetime`由`ConfigParam 28`（`CatchainConfig`）设置，`natural_block_production_speed`和`max_dependencies_size`由`ConfigParam 29`（`ConsensusConfig`）设置（`natural_block_production_speed`计算为`catchain_max_blocks_coeff / 1000`），`number_of_catchain_participants`从Catchain组配置中获取。

默认情况下，`catchain_max_blocks_coeff`设置为零：这是一个特殊值，表示对Catchain区块高度没有限制。建议将`catchain_max_blocks_coeff`设置为`10000`：我们预计自然生产速率约为每3秒一个区块，因此我们设置系数以允许比预期高出30倍的区块生成速率。同时，这个数值足够低，可以防止资源密集型攻击。

为了防止多个同一高度的区块DoS攻击，节点将忽略来自“坏”节点（即创建了分叉的节点）的任何区块，除非新区块是“好”节点的某个其他区块所需要的。
