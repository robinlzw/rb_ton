这份文档的目的是提供TON区块链配置参数的基本解释，并给出通过大多数验证者达成共识来更改这些参数的逐步说明。我们假设读者已经熟悉LiteClient-HOWTO中解释的Fift和Lite客户端，以及FullNode-HOWTO和Validator-HOWTO中描述的验证者投票配置提案的部分。

### 1. 配置参数

**配置参数**是影响TON区块链验证者和/或基础智能合约行为的某些值。所有配置参数的当前值作为主链状态的一部分存储，并在需要时从当前主链状态中提取。因此，有必要根据特定的主链区块来讨论配置参数的值。每个分片链区块包含对最新已知主链区块的引用；生成和验证分片链区块时，假设使用相应主链状态中的值作为活动值。对于主链区块，使用前一个主链区块的状态来提取活动配置参数。因此，即使尝试在主链区块内更改某些配置参数，这些更改也仅在下一个主链区块中生效。

每个配置参数由一个有符号32位整数索引标识，称为**配置参数索引**或简单地称为**索引**。配置参数的值始终是一个Cell。有些配置参数可能缺失；在这种情况下，有时假设该参数的值为Null。此外，还有一组必须始终存在的**强制性**配置参数；此列表存储在配置参数#10中。

所有配置参数组合成一个**配置字典** - 一个具有有符号32位键（配置参数索引）和值由一个Cell引用组成的Hashmap。换句话说，配置字典是TL-B类型的值（HashmapE 32 ^Cell）。事实上，所有配置参数的集合在主链状态中作为TL-B类型的值`ConfigParams`存储：

```
_config_addr:bits256 config:^(Hashmap 32 ^Cell) = ConfigParams;
```

我们看到，除了配置字典外，`ConfigParams`还包含主链中配置智能合约的256位地址`config_addr`。有关配置智能合约的更多详细信息将在稍后提供。

包含所有配置参数活动值的配置字典通过特殊的TVM寄存器*c7*对所有智能合约可用，当它们的代码在交易中执行时。更确切地说，当智能合约执行时，*c7*由一个Tuple初始化，其唯一元素是包含多个对执行智能合约有用的“上下文”值的Tuple，例如区块头中记录的当前Unix时间。这个Tuple的第十个条目（即索引为9的条目）包含表示配置字典的Cell。因此，可以通过TVM指令“PUSH c7; FIRST; INDEX 9”或等效指令“CONFIGROOT”访问它。事实上，特殊的TVM指令“CONFIGPARAM”和“CONFIGOPTPARAM”将前面的操作与字典查找结合起来，通过索引返回任何配置参数。有关这些指令的更多详细信息，请参阅TVM文档。这里相关的是，所有配置参数对于所有智能合约（无论是主链还是分片链）都易于访问，智能合约可以检查它们并使用它们执行特定检查。例如，智能合约可能会从配置参数中提取工作链数据存储价格，以计算存储用户提供的数据块的价格。

配置参数的值不是任意的。事实上，如果配置参数索引*i*是非负的，则该参数的值必须是TL-B类型（ConfigParam i）的有效值。这种限制由验证者执行，验证者不会接受非负索引的配置参数更改，除非它们是相应TL-B类型的有效值。

因此，这些参数的结构在源文件`crypto/block/block.tlb`中定义，不同值的*i*定义了不同的（ConfigParam i）。例如，

```
_config_addr:bits256 = ConfigParam 0;
_elector_addr:bits256 = ConfigParam 1;
_dns_root_addr:bits256 = ConfigParam 4;  // root TON DNS resolver

capabilities#c4 version:uint32 capabilities:uint64 = GlobalVersion;
_GlobalVersion = ConfigParam 8;  // all zero if absent
```

我们看到配置参数#8包含一个没有引用且恰好有104个数据位的Cell。前四位必须是11000100，然后存储当前启用的“全局版本”的32位，接着是64位整数，对应于当前启用的功能标志。TON区块链文档的附录中将提供所有配置参数的更详细描述；目前，可以检查`crypto/block/block.tlb`中的TL-B方案，并检查不同参数在验证者源代码中的使用方式。

与非负索引的配置参数不同，负索引的配置参数可以包含任意值。至少，验证者不会对其值施加限制。因此，它们可以用于存储重要信息（例如某些智能合约必须开始操作的Unix时间），这些信息对于区块生成不是关键，但由一些基础智能合约使用。

### 2. 更改配置参数

我们已经解释了所有配置参数的当前值存储在主链状态的特定部分。那么，它们是如何更改的呢？

事实上，主链中存在一个特殊的智能合约，称为**配置智能合约**。其地址由我们之前描述的`ConfigParams`中的`config_addr`字段确定。其数据的第一个Cell引用必须包含所有配置参数的最新副本。当生成新的主链区块时，通过其地址`config_addr`查找配置智能合约，并从其数据的第一个Cell引用中提取新的配置字典。经过一些有效性检查（例如验证任何具有非负32位索引*i*的值确实是TL-B类型（ConfigParam i）的有效值）后，验证者将此新配置字典复制到包含ConfigParams的主链部分。这是在所有交易创建后执行的，因此仅检查配置智能合约中存储的新配置字典的最终版本。如果有效性检查失败，则“真实”配置字典保持不变。通过这种方式，配置智能合约无法安装无效的配置参数值。如果新配置字典与当前配置字典一致，则不进行检查，也不会进行任何更改。

通过这种方式，所有配置参数的更改都是由配置智能合约执行的，其代码确定了更改配置参数的规则。目前，配置智能合约支持两种更改配置参数的模式：

1. 通过特定私钥签名的外部消息，该私钥对应于存储在配置智能合约数据中的公钥。这是公测网中使用的方法，可能也是由一个实体控制的小型私有测试网络中使用的方法，因为它使操作员能够轻松更改任何配置参数的值。请注意，可以通过旧密钥签名的特殊外部消息更改此公钥，如果将其更改为零，则此机制被禁用。因此，可以在启动后进行微调，然后永久禁用。
2. 通过创建“配置提案”，然后由验证者投票支持或反对这些提案。通常，一个配置提案必须获得超过3/4的所有验证者（按权重）的投票，不仅在一轮中，而且在几轮中（即，连续几轮验证者组必须确认提议的参数更改）。这是TON区块链主网使用的分布式治理机制。

我们希望详细描述第二种更改配置参数的方式。


### 3. 创建配置提案

一个新的**配置提案**包含以下数据：
- 要更改的配置参数的索引
- 配置参数的新值（或Null，如果要删除该参数）
- 提案的过期Unix时间
- 一个标志，指示提案是否为**关键**
- 一个可选的**旧值哈希**，包含当前值的单元哈希（提案只能在当前值具有指示的哈希时激活）

任何拥有主链钱包的人都可以创建新的配置提案，只要他支付了足够的费用。然而，只有验证者可以对现有的配置提案进行投票。

请注意，配置提案分为**关键**和**普通**两种。关键配置提案可以更改任何配置参数，包括所谓的关键参数（关键配置参数的列表存储在配置参数#10中，该参数本身也是关键参数）。然而，创建关键配置提案的费用更高，通常需要在多轮中收集更多验证者的投票（普通和关键配置提案的精确投票要求存储在关键配置参数#11中）。另一方面，普通配置提案费用较低，但不能更改关键配置参数。

为了创建新的配置提案，首先需要生成一个包含提议新值的BoC（bag-of-cells）文件。生成方式取决于要更改的配置参数。例如，如果我们想创建参数-239，包含UTF-8字符串"TEST"（即0x54455354），可以按以下方式创建`config-param-239.boc`文件：调用Fift，然后输入

```fift
<b "TEST" $, b> 2 boc+>B "config-param-239.boc" B>file
bye
```

结果将创建一个21字节的`config-param-239.boc`文件，其中包含所需值的序列化。

对于更复杂的情况，特别是具有非负索引的配置参数，这种直接方法不太适用。我们建议使用`create-state`（在构建目录中可作为`crypto/create-state`使用），并从源文件`crypto/smartcont/gen-zerostate.fif`和`crypto/smartcont/CreateState.fif`中复制和编辑适当部分，这些文件通常用于创建TON区块链的零状态（对应于其他区块链架构的“创世区块”）。

例如，考虑配置参数#8，它包含当前启用的全局区块链版本和功能：

```tlb
capabilities#c4 version:uint32 capabilities:uint64 = GlobalVersion;
_GlobalVersion = ConfigParam 8;
```

我们可以通过运行lite-client并输入`getconfig 8`来检查其当前值：

```lite-client
> getconfig 8
...
ConfigParam(8) = (
  (capabilities version:1 capabilities:6))

x{C4000000010000000000000006}
```

现在假设我们想启用由位#3（+8）表示的功能，即capReportVersion（启用时，此功能强制所有协同者在他们生成的区块头中报告其支持的版本和功能）。因此，我们希望版本=1且功能=14。在这个例子中，我们仍然可以猜测正确的序列化，并通过在Fift中输入直接创建BoC文件：

```fift
x{C400000001000000000000000E} s>c 2 boc+>B "config-param8.boc" B>file
```

（结果创建了一个30字节的`config-param8.boc`文件，包含所需的值。）

然而，在更复杂的情况下，这可能不是一个选项，所以让我们以不同的方式做这个例子。即，我们可以检查源文件`crypto/smartcont/gen-zerostate.fif`和`crypto/smartcont/CreateState.fif`中的相关部分：

```fift
// version capabilities --
{ <b x{c4} s, rot 32 u, swap 64 u, b> 8 config! } : config.version!
1 constant capIhr
2 constant capCreateStats
4 constant capBounceMsgBody
8 constant capReportVersion
16 constant capSplitMergeTransactions
```

和

```fift
// version capabilities
1 capCreateStats capBounceMsgBody or capReportVersion or config.version!
```

我们看到“config.version!”没有最后的“8 config!”基本上做了我们需要的事情，因此我们可以创建一个临时Fift脚本，例如，`create-param8.fif`：

```fift
#!/usr/bin/fift -s
"TonUtil.fif" include

1 constant capIhr
2 constant capCreateStats
4 constant capBounceMsgBody
8 constant capReportVersion
16 constant capSplitMergeTransactions
{ <b x{c4} s, rot 32 u, swap 64 u, b> } : prepare-param8

// create new value for config param #8
1 capCreateStats capBounceMsgBody or capReportVersion or prepare-param8
// check the validity of this value
dup 8 is-valid-config? not abort"not a valid value for chosen configuration parameter"
// print
dup ."Serialized value = " <s csr.
// save into file provided as first command line argument
2 boc+>B $1 tuck B>file
."(Saved into file " type .")" cr
```

现在，如果我们运行`fift -s create-param8.fif config-param8.boc`，或者更好的是，从构建目录中运行`crypto/create-state -s create-param8.fif config-param8.boc`，我们会看到以下输出：

```output
Serialized value = x{C400000001000000000000000E}
(Saved into file config-param8.boc)
```

并且我们获得了与之前相同内容的30字节文件`config-param8.boc`。

一旦我们有了配置参数所需值的文件，我们就调用源树目录`crypto/smartcont`中的脚本`create-config-proposal.fif`，并提供适当的参数。同样，我们建议使用`create-state`（在构建目录中可作为`crypto/create-state`使用），因为它是Fift的特殊扩展版本，能够进行更多与区块链相关的有效性检查。

#### 使用create-state创建配置提案

我们使用命令`crypto/create-state -s create-config-proposal.fif 8 config-param8.boc -x 1100000`，生成配置提案。

```bash
$ crypto/create-state -s create-config-proposal.fif 8 config-param8.boc -x 1100000
```

输出如下：

```plaintext
Loading new value of configuration parameter 8 from file config-param8.boc
x{C400000001000000000000000E}

Non-critical configuration proposal will expire at 1586779536 (in 1100000 seconds)
Query id is 6810441749056454664 
resulting internal message body: x{6E5650525E838CB0000000085E9455904_}
 x{F300000008A_}
  x{C400000001000000000000000E}

B5EE9C7241010301002C0001216E5650525E838CB0000000085E9455904001010BF300000008A002001AC400000001000000000000000ECD441C3C
(a total of 104 data bits, 0 cell references -> 59 BoC data bytes)
(Saved to file config-msg-body.boc)
```

我们得到了一个内部消息体，需要将其与适量的Grams一起从任何驻留在主链上的（钱包）智能合约发送到配置智能合约。可以通过在lite-client中输入`getconfig 0`来获取配置智能合约的地址：

```lite-client
> getconfig 0
ConfigParam(0) = ( config_addr:x5555555555555555555555555555555555555555555555555555555555555555)
x{5555555555555555555555555555555555555555555555555555555555555555}
```

我们看到配置智能合约的地址是`-1:5555...5555`。通过运行该智能合约的适当get方法，可以找出创建此配置提案所需的支付金额：

```lite-client
> runmethod -1:5555555555555555555555555555555555555555555555555555555555555555 proposal_storage_price 0 1100000 104 0
...
arguments:  [ 0 1100000 104 0 75077 ] 
result:  [ 2340800000 ] 
remote result (not to be trusted):  [ 2340800000 ] 
```

get方法`proposal_storage_price`的参数分别是关键标志（此处为0）、该提案将处于活动状态的时间间隔（1.1百万秒）、数据中的总位数（104）和单元引用数（0）。这两个数量可以在`create-config-proposal.fif`的输出中看到。

我们看到，创建此提案需要支付2.3408个测试Grams。最好添加至少1.5个测试Grams以支付处理费用，所以我们将发送4个测试Grams连同请求（所有多余的测试Grams将被退回）。现在，我们使用`wallet.fif`（或我们使用的钱包对应的Fift脚本）创建一个从我们的钱包到配置智能合约的转账，携带4个测试Grams和`config-msg-body.boc`中的消息体。这通常如下所示：

```plaintext
fift -s wallet.fif -a -1:5555555555555555555555555555555555555555555555555555555555555555 -g 4 -B config-msg-body.boc
```


$ fift -s wallet.fif my-wallet -1:5555555555555555555555555555555555555555555555555555555555555555 31 4. -B config-msg-body.boc
...
Transferring GR$4. to account kf9VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVQft = -1:5555555555555555555555555555555555555555555555555555555555555555 seqno=0x1c bounce=-1 
Body of transfer message is x{6E5650525E835154000000085E9293944_}
 x{F300000008A_}
  x{C400000001000000000000000E}

signing message: x{0000001C03}
 x{627FAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA773594000000000000000000000000000006E5650525E835154000000085E9293944_}
  x{F300000008A_}
   x{C400000001000000000000000E}

resulting external message: x{89FE000000000000000000000000000000000000000000000000000000000000000007F0BAA08B4161640FF1F5AA5A748E480AFD16871E0A089F0F017826CDC368C118653B6B0CEBF7D3FA610A798D66522AD0F756DAEECE37394617E876EFB64E9800000000E01C_}
 x{627FAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA773594000000000000000000000000000006E5650525E835154000000085E9293944_}
  x{F300000008A_}
   x{C400000001000000000000000E}

B5EE9C724101040100CB0001CF89FE000000000000000000000000000000000000000000000000000000000000000007F0BAA08B4161640FF1F5AA5A748E480AFD16871E0A089F0F017826CDC368C118653B6B0CEBF7D3FA610A798D66522AD0F756DAEECE37394617E876EFB64E9800000000E01C010189627FAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA773594000000000000000000000000000006E5650525E835154000000085E9293944002010BF300000008A003001AC400000001000000000000000EE1F80CD3
(Saved to file wallet-query.boc)

====================================================

现在，我们使用lite-client将外部消息`wallet-query.boc`发送到区块链：

```lite-client
> sendfile wallet-query.boc
....
external message status is 1
```

等待一段时间后，我们可以检查钱包的接收消息，以查看来自配置智能合约的响应消息，或者，如果我们感到幸运，可以通过配置智能合约的方法`list_proposals`检查所有活动配置提案的列表：

====================================================
> runmethod -1:5555555555555555555555555555555555555555555555555555555555555555 list_proposals
...
arguments:  [ 107394 ] 
result:  [ ([64654898543692093106630260209820256598623953458404398631153796624848083036321 [1586779536 0 [8 C{FDCD887EAF7ACB51DA592348E322BBC0BD3F40F9A801CB6792EFF655A7F43BBC} -1] 112474791597373109254579258586921297140142226044620228506108869216416853782998 () 864691128455135209 3 0 0]]) ] 
remote result (not to be trusted):  [ ([64654898543692093106630260209820256598623953458404398631153796624848083036321 [1586779536 0 [8 C{FDCD887EAF7ACB51DA592348E322BBC0BD3F40F9A801CB6792EFF655A7F43BBC} -1] 112474791597373109254579258586921297140142226044620228506108869216416853782998 () 864691128455135209 3 0 0]]) ] 
...	caching cell FDCD887EAF7ACB51DA592348E322BBC0BD3F40F9A801CB6792EFF655A7F43BBC
====================================================

我们看到所有活动配置提案的列表中只有一个条目，表示为一对

```plaintext
[6465...6321 [1586779536 0 [8 C{FDCD...} -1] 1124...2998 () 8646...209 3 0 0]]
```

这里，第一个数字6465...6321是配置提案的唯一标识符，等于其256位哈希值。这个对的第二部分是描述该配置提案状态的元组。元组的第一个部分是配置提案的过期Unix时间（1586779536）。第二部分（0）是关键性标志。接下来是配置提案本身，由三元组[8 C{FDCD...} -1]描述，其中8是要修改的配置参数的索引，C{FDCD...}是包含新值的单元（以该单元的哈希表示），-1是旧值的可选哈希（-1表示未指定此哈希）。然后我们看到一个大数字1124...2998，表示当前验证者集的标识符，接着是一个空列表()，表示迄今为止投票支持该提案的所有活跃验证者集，然后是*weight_remaining*，等于8646...209 - 一个如果提案在此轮中尚未收集到足够验证者投票权重则为正数的数字，否则为负数。然后我们看到三个数字3 0 0。这些数字分别是*rounds_remaining*（此提案最多会存活三轮，即验证者集的更换），*wins*（该提案在超过3/4验证者（按权重计算）投票支持的轮次计数）和*losses*（该提案未能收集到3/4验证者投票的轮次计数）。

我们可以通过要求lite-client使用其哈希FDCD...或该哈希的足够长前缀唯一标识要检查的单元，来展开单元C{FDCD...}以检查配置参数#8的建议值：

> dumpcell FDC
C{FDCD887EAF7ACB51DA592348E322BBC0BD3F40F9A801CB6792EFF655A7F43BBC} =
  x{C400000001000000000000000E}

我们看到值是 x{C400000001000000000000000E}，这确实是我们在配置提案中嵌入的值。我们甚至可以要求 lite-client 将此单元显示为 TL-B 类型 (ConfigParam 8) 的值：

> dumpcellas ConfigParam8 FDC
dumping cells as values of TLB type (ConfigParam 8)
C{FDCD887EAF7ACB51DA592348E322BBC0BD3F40F9A801CB6792EFF655A7F43BBC} =
  x{C400000001000000000000000E}
(
    (capabilities version:1 capabilities:14))

当我们考虑其他人创建的配置提案时，这尤其有用。

请注意，配置提案从此以后将通过其 256 位哈希值（即巨大的十进制数字 6465...6321）进行标识。我们可以通过运行 get-method `get_proposal` 并将唯一参数设置为配置提案的标识符，来检查特定配置提案的当前状态：

> runmethod -1:5555555555555555555555555555555555555555555555555555555555555555 get_proposal 64654898543692093106630260209820256598623953458404398631153796624848083036321
...
arguments:  [ 64654898543692093106630260209820256598623953458404398631153796624848083036321 94347 ] 
result:  [ [1586779536 0 [8 C{FDCD887EAF7ACB51DA592348E322BBC0BD3F40F9A801CB6792EFF655A7F43BBC} -1] 112474791597373109254579258586921297140142226044620228506108869216416853782998 () 864691128455135209 3 0 0] ] 

我们获得的结果本质上与之前相同，但仅针对一个配置提案，并且没有配置提案开头的标识符。

4. 配置提案投票

一旦创建了配置提案，它需要在当前以及可能的几个后续轮次（选出的验证者集）中获得超过当前所有验证者（按权重，即按质押计算）3/4的选票。通过这种方式，配置参数的更改必须得到不仅是当前验证者集的显著多数的批准，还需要获得多个后续验证者集的批准。

只有当前验证者才能对配置提案进行投票，这些验证者及其永久公钥列在配置参数#34中。投票过程大致如下：

- 验证者的操作员查找 *val-idx*，即当前验证者集中其验证者的（从0开始的）索引，该索引存储在配置参数#34中。
- 操作员调用源代码树目录 `crypto/smartcont` 中的特殊 Fift 脚本 `config-proposal-vote-req.fif`，并将 *val-idx* 和 *config-proposal-id* 作为其参数传入：

    $ fift -s config-proposal-vote-req.fif -i 0 64654898543692093106630260209820256598623953458404398631153796624848083036321
    Creating a request to vote for configuration proposal 0x8ef1603180dad5b599fa854806991a7aa9f280dbdb81d67ce1bedff9d66128a1 on behalf of validator with index 0 
    566F744500008EF1603180DAD5B599FA854806991A7AA9F280DBDB81D67CE1BEDFF9D66128A1
    Vm90RQAAjvFgMYDa1bWZ-oVIBpkaeqnygNvbgdZ84b7f-dZhKKE=
    Saved to file validator-to-sign.req

- 然后，投票请求需要使用连接到验证者的 `validator-engine-console` 中的当前验证者私钥进行签名，使用 `sign <validator-key-id> 566F744...28A1`。这个过程与 Validator-HOWTO 中描述的参与验证者选举的过程类似，但这次需要使用当前活动的密钥。
- 接下来，需要调用另一个脚本 `config-proposal-signed.fif`。它的参数与 `config-proposal-req.fif` 类似，但它需要两个额外的参数：用于签署投票请求的公钥的 base64 表示形式和签名本身的 base64 表示形式。这与 Validator-HOWTO 中描述的过程非常相似。
- 通过这种方式，创建了包含对该配置提案的签名投票的内部消息主体的文件 `vote-msg-body.boc`。
- 然后，需要将 `vote-msg-body.boc` 通过任何位于主链中的智能合约（通常是验证者的控制智能合约）携带到内部消息中，并附带少量的处理费用（通常 1.5 Grams 应该足够）。这与验证者选举过程中使用的程序完全相同。通常通过运行以下命令来实现：

    $ fift -s wallet.fif my_wallet_id -1:5555555555555555555555555555555555555555555555555555555555555555 1 1.5 -B vote-msg-body.boc

（如果使用简单钱包控制验证者）然后从 lite-client 发送生成的文件 `wallet-query.boc`：

外部消息状态为 1 表示消息已成功发送。这样，验证者的投票就成功提交给配置智能合约了。
    > sendfile wallet-query.boc

您可以监控来自配置智能合约的答复消息，以了解您的投票查询的状态。或者，您可以通过配置智能合约的 `show_proposal` 方法来检查配置提案的状态：

> runmethod -1:5555555555555555555555555555555555555555555555555555555555555555 get_proposal 64654898543692093106630260209820256598623953458404398631153796624848083036321
...
arguments:  [ 64654898543692093106630260209820256598623953458404398631153796624848083036321 94347 ] 
result:  [ [1586779536 0 [8 C{FDCD887EAF7ACB51DA592348E322BBC0BD3F40F9A801CB6792EFF655A7F43BBC} -1] 112474791597373109254579258586921297140142226044620228506108869216416853782998 (0) 864691128455135209 3 0 0] ]


我们可以看到，这次所有投票的验证器列表应为非空，并且应该包含您的验证器的索引。在这个例子中，这个列表是 (0)，意味着只有配置参数 #34 中的索引为 0 的验证器进行了投票。如果列表变得足够长，提案状态中的倒数第二个整数（"3 0 0" 中的第一个零）会增加 1，表示该提案获得了新的胜利。如果获胜次数达到或超过配置参数 #11 中指定的值，那么配置提案将被自动接受，提议的更改会立即生效。另一方面，当验证器集发生变化时，已经投票的验证器列表将变为空，*rounds_remaining*（"3 0 0" 中的三）将减少 1，如果它变成负数，配置提案将被销毁。如果没有被销毁，且在这一轮中没有获胜，那么失败次数（"3 0 0" 中的第二个零）会增加。如果失败次数大于配置参数 #11 中指定的值，则配置提案将被丢弃。在这种情况下，所有在某轮中弃权的验证器都将隐式投票反对该提案。



5. 自动投票配置提案的方法
类似于 `validator-engine-console` 中 `createelectionbid` 命令提供的自动化功能，用于参与验证器选举，`validator-engine` 和 `validator-engine-console` 也提供了执行前述步骤的自动化方式，从而生成一个可用于控制钱包的 `vote-msg-body.boc`。为了使用这种方法，您必须将 Fift 脚本 `config-proposal-vote-req.fif` 和 `config-proposal-vote-signed.fif` 安装到与 `validator-engine` 用于查找 `validator-elect-req.fif` 和 `validator-elect-signed.fif` 的目录相同的目录中，如 Validator-HOWTO 第 5 节所述。之后，您只需运行

    createproposalvote 64654898543692093106630260209820256598623953458404398631153796624848083036321 vote-msg-body.boc

在 `validator-engine-console` 中创建 `vote-msg-body.boc`，该文件包含要发送到配置智能合约的内部消息体。

6. 升级配置智能合约和选举者智能合约的代码

可能会发生需要升级配置智能合约本身或选举者智能合约的代码。为此，使用与上述相同的机制。新的代码应存储在一个值单元的唯一引用中，并将该值单元作为配置参数 -1000（用于升级配置智能合约）或 -1001（用于升级选举者智能合约）的新值进行提议。这些参数被视为关键参数，因此更改配置智能合约需要大量的验证器投票（这类似于通过一部新宪法）。我们预计此类更改将首先在测试网络中进行测试，并在公开论坛上讨论提议的更改，然后每个验证器操作员决定是否对提议的更改投票赞成或反对。

另外，可以将关键配置参数 0（配置智能合约的地址）或 1（选举者智能合约的地址）更改为其他值，这些值必须对应于已经存在并正确初始化的智能合约。特别是，新的配置智能合约必须在其持久数据的第一个引用中包含有效的配置字典。由于在不同智能合约之间正确转移更改的数据（如活动配置提案的列表，或验证器选举的前后参与者列表）并不容易，因此在大多数情况下，升级现有智能合约的代码比更改配置智能合约地址更为合适。

有两个辅助脚本用于创建升级配置或选举者智能合约代码的配置提案。即，`create-config-upgrade-proposal.fif` 加载 Fift 汇编源文件（默认是 `auto/config-code.fif`，对应于 FunC 编译器自动生成的代码），并创建对应的配置提案（用于配置参数 -1000）。类似地，`create-elector-upgrade-proposal.fif` 加载 Fift 汇编源文件（默认是 `auto/elector-code.fif`），并使用它来创建用于配置参数 -1001 的配置提案。通过这种方式，创建升级这些两个智能合约中的一个的配置提案应该非常简单。然而，还应发布修改后的 FunC 源代码、用于编译的 FunC 编译器的确切版本，以便所有验证器（或更准确地说是他们的操作员）能够重现配置提案中的代码（并比较哈希值），并在决定是否对提议的更改投票前，研究和讨论源代码及其变更。