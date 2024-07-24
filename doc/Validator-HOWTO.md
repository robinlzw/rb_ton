这份文档的目的是为TON区块链的全节点设置提供逐步指导，作为验证者。我们假设TON区块链的全节点已经按照FullNode-HOWTO中的说明运行起来。我们还假设您对TON区块链的Lite客户端有一定的了解。

注意，验证者必须在安装在可靠数据中心的专用高性能服务器上运行，并且您需要大量的克朗（如果您想在“测试网”中运行验证者，则为测试克朗）作为验证者的赌注。如果您的验证者工作不正确或长时间不可用，您可能会失去部分或全部赌注，因此使用高性能、可靠的服务器是有意义的。我们建议使用至少每个处理器有八个核心的双处理器服务器，至少256 MiB RAM，至少8 TB的传统HDD存储和至少512 GB的更快SSD存储，以及1 Gbit/s网络（和互联网）连接，以可靠地容纳峰值负载。

0. 下载和编译

基本指令与TON区块链全节点相同，如FullNode-HOWTO中所解释的。实际上，任何全节点如果发现其私钥对应的公钥出现在当前TON区块链实例的当前验证者集中，都会自动作为验证者工作。特别是，全节点和验证者使用相同的二进制文件`validator-engine`，并通过相同的`validator-engine-console`进行控制。

1. 验证者的控制智能合约

为了运行验证者，您需要一个已经运行并且完全同步到当前区块链状态的全节点，以及一个在主链中持有大量克朗（或者如果您想在“测试网”TON区块链实例中运行验证者，则为测试克朗）的钱包。通常您需要在生产网络中至少有100,001克朗，在测试网络中至少有10,001测试克朗。实际值（以纳克朗计）可以在Lite客户端中通过键入`getconfig 17`获得，查看配置参数#17的值，再加上一个克朗。

每个验证者由其（Ed25519）公钥标识。在验证者选举期间，验证者（或其公钥）还与主链中的智能合约相关联。为了简单起见，我们说验证者由这个智能合约“控制”（例如，钱包智能合约）。只有当赌注来自其关联的智能合约时，才会接受代表该验证者的赌注，并且只有该关联的智能合约有权在验证者的赌注解冻后收集验证者的赌注，以及验证者从验证者池中收集的奖金份额（例如，区块挖矿费、交易和消息转发费，由TON区块链的用户支付给验证者）。通常，奖金会按比例分配给验证者的有效赌注。另一方面，拥有更多赌注的验证者被分配更多的工作量（即，他们需要为更多的分片链创建和验证区块），因此重要的是不要下注一个会给您节点带来处理能力的验证工作量的金额。

请注意，每个验证者（通过其公钥标识）最多可以与一个控制智能合约（位于主链中）相关联，但同一个控制智能合约可以与多个验证者相关联。这样，您可以在不同的物理服务器上运行多个验证者，并从同一个智能合约中为它们下注。如果其中一个验证者停止工作并且您失去了它的赌注，其他验证者应该继续运行并保留它们的赌注，并有可能获得奖金。

2. 创建控制智能合约

如果您没有控制智能合约，您可以简单地在主链中创建一个钱包。可以使用位于源代码树的crypto/smartcont子目录中的new-wallet.fif脚本来创建简单的钱包。以下假设您已将环境变量FIFTPATH配置为包含<source-root>/crypto/fift/lib:<source-root>/crypto/smartcont，并且您的PATH包括带有Fift二进制文件的目录（位于<build-directory>/crypto/fift）。然后您可以简单地运行：
```
$ fift -s new-wallet.fif -1 my_wallet_id
```
其中“my_wallet_id”是您想为您的新钱包分配的任何标识符，-1是主链的工作链标识符。如果您没有设置FIFTPATH和PATH，那么您将不得不在构建目录中运行更长版本的此命令，如下所示：
```
$ crypto/fift -I <source-dir>/crypto/fift/lib:<source-dir>/crypto/smartcont -s new-wallet.fif -1 my_wallet_id
```
一旦您运行此脚本，将显示新智能合约的地址：
```
...
new wallet address = -1:af17db43f40b6aa24e7203a9f8c8652310c88c125062d1129fe883eaa1bd6763 
(Saving address to file my_wallet_id.addr)
Non-bounceable address (for init): 0f-vF9tD9Atqok5yA6n4yGUjEMiMElBi0RKf6IPqob1nYzqK
Bounceable address (for later access): kf-vF9tD9Atqok5yA6n4yGUjEMiMElBi0RKf6IPqob1nY2dP
...
```
（将钱包创建查询保存到文件my_wallet_id-query.boc）

现在my_wallet_id.pk是包含控制此钱包的私钥的新文件（您必须保密），my_wallet_id.addr是包含此钱包地址的（不那么秘密的）文件。完成此操作后，您必须将一些（测试）克朗转移到您的钱包的不可反弹地址，并在Lite客户端中运行“sendfile my_wallet_id-query.boc”以完成创建新钱包。这个过程在LiteClient-HOWTO中有更详细的解释。

如果您在“主网”中运行验证者，最好使用更复杂的钱包智能合约（例如，多签名钱包）。对于“测试网”，简单的钱包应该足够了。

3. 选举智能合约

选举智能合约是位于主链中的特殊智能合约。其完整地址是-1:xxx..xxx，其中-1是工作链标识符（-1对应于主链），xxx..xxx是其在主链内的256位地址的十六进制表示。为了找到这个地址，您必须从区块链的最新状态中读取配置参数#1。这可以通过在Lite客户端中使用命令`getconfig 1`轻松完成：
```
> getconfig 1
ConfigParam(1) = ( elector_addr:xA4C2C7C05B093D470DE2316DBA089FA0DD775FD9B1EBFC9DC9D04B498D3A2DDA)
x{A4C2C7C05B093D470DE2316DBA089FA0DD775FD9B1EBFC9DC9D04B498D3A2DDA}
```
在这种情况下，完整的选举地址是-1:A4C2C7C05B093D470DE2316DBA089FA0DD775FD9B1EBFC9DC9D04B498D3A2DDA

我们假设您熟悉Lite客户端，并且知道如何运行它以及如何获取它的全局配置文件。请注意，上述命令可以通过使用Lite客户端的'-c'命令行选项在批处理模式下运行：
```
$ lite-client -C <global-config-file> -c 'getconfig 1'
...
ConfigParam(1) = ( elector_addr:xA4C2C7C05B093D470DE2316DBA089FA0DD775FD9B1EBFC9DC9D04B498D3A2DDA)
x{A4C2C7C05B093D470DE2316DBA089FA0DD775FD9B1EBFC9DC9D04B498D3A2DDA}

$
```

选举智能合约有几种用途。最重要的是，您可以通过向选举智能合约发送消息，从控制智能合约参与验证者选举或收集解冻的赌注和奖金。您还可以通过调用选举智能合约的所谓“get-methods”来了解当前的验证者选举及其参与者。

具体来说，运行
```
> runmethod -1:A4C2C7C05B093D470DE2316DBA089FA0DD775FD9B1EBFC9DC9D04B498D3A2DDA active_election_id
...
arguments:  [ 86535 ] 
result:  [ 1567633899 ] 
```

（或者lite-client -C <global-config> -c "runmethod -1:<elector-addr> active_election_id"在批处理模式下）将返回当前活动选举的标识符（一个非零整数，通常是被选举的验证者组的服务期限开始的Unix时间），或者如果当前没有活动选举，则返回0。在这个例子中，活动选举的标识符是1567633899。

您还可以通过运行方法“participant_list”而不是“active_election_id”来恢复所有活动参与者的列表（256位验证者公钥及其相应赌注的对，以纳克朗表示）。

4. 创建验证者公钥和ADNL地址

为了参加验证者选举，您需要知道选举标识符（通过运行选举智能合约的get-method "active_elections_id"获得），以及您的验证者公钥。
公钥是通过运行validator-engine-console（如FullNode-HOWTO中所解释）并运行以下命令创建的：
```
$ validator-engine-console ...
...
conn ready
> newkey
created new key BCA335626726CF2E522D287B27E4FAFFF82D1D98615957DB8E224CB397B2EB67
> exportpub BCA335626726CF2E522D287B27E4FAFFF82D1D98615957DB8E224CB397B2EB67
got public key: xrQTSIQEsqZkWnoADMiBnyBFRUUweXTvzRQFqb5nHd5xmeE6
> addpermkey BCA335626726CF2E522D287B27E4FAFFF82D1D98615957DB8E224CB397B2EB67 1567633899 1567733900
success
```

现在全节点（validator-engine）已生成了一个新的密钥对，导出了公钥的base64表示（xrQT...E6），并将其注册为从Unix时间1567633899（等于选举标识符）开始签署区块的持久密钥，直到1567733900（等于前一个数字加上在Lite客户端中通过键入"getconfig 15"获得的配置参数#15中的验证者集的期限，再加上以防选举实际发生得比预期晚的安全余量）。

您还需要定义一个临时密钥，供验证者用于参与网络共识协议。最简单的方法（足以用于测试目的）是将此密钥设置为持久（区块签名）密钥：
```
> addtempkey BCA335626726CF2E522D287B27E4FAFFF82D1D98615957DB8E224CB397B2EB67 BCA335626726CF2E522D287B27E4FAFFF82D1D98615957DB8E224CB397B2EB67 1567733900
success
```

最好还创建一个专门用于验证者目的的ADNL地址：
```
> newkey
created new key C5C2B94529405FB07D1DDFB4C42BFB07727E7BA07006B2DB569FBF23060B9E5C
> addadnl C5C2B94529405FB07D1DDFB4C42BFB07727E7BA07006B2DB569FBF23060B9E5C 0
success
> addvalidatoraddr BCA335626726CF2E522D287B27E4FAFFF82D1D98615957DB8E224CB397B2EB67 C5C2B94529405FB07D1DDFB4C42BFB07727E7BA07006B2DB569FBF23060B9E5C 1567733900
success
```

现在C5C2B94529405FB07D1DDFB4C42BFB07727E7BA07006B2DB569FBF23060B9E5C是一个新的ADNL地址，将被全节点用于以公钥BCA...B67运行为验证者，过期时间设置为1567733900。

5. 创建选举参与请求

特殊脚本validator-elect-req.fif（位于<source-dir>/crypto/smartcont）用于创建必须由验证者签名才能参加选举的消息。其运行方式如下：
```
$ fift -s validator-elect-req.fif <wallet-addr> <elect-utime> <max-factor> <adnl-addr> [<savefile>]
```

例如，
```
$ fift -s validator-elect-req.fif kf-vF9tD9Atqok5yA6n4yGUjEMiMElBi0RKf6IPqob1nY2dP 1567633899 2.7 C5C2B94529405FB07D1DDFB4C42BFB07727E7BA07006B2DB569FBF23060B9E5C
```

或者，如果您通过new-wallet.fif创建了控制钱包，您可以使用@my_wallet_id.addr而不是复制钱包地址kf-vF...dP：

```
$ fift -s validator-elect-req.fif @my_wallet_id.addr 1567633899 2.7 C5C2B94529405FB07D1DDFB4C42BFB07727E7BA07006B2DB569FBF23060B9E5C

Creating a request to participate in validator elections at time 1567633899 from smart contract Uf+vF9tD9Atqok5yA6n4yGUjEMiMElBi0RKf6IPqob1nY4EA = -1:af17db43f40b6aa24e7203a9f8c8652310c88c125062d1129fe883eaa1bd6763  with maximal stake factor with respect to the minimal stake 176947/65536 and validator ADNL address c5c2b94529405fb07d1ddfb4c42bfb07727e7ba07006b2db569fbf23060b9e5c 
654C50745D7031EB0002B333AF17DB43F40B6AA24E7203A9F8C8652310C88C125062D1129FE883EAA1BD6763C5C2B94529405FB07D1DDFB4C42BFB07727E7BA07006B2DB569FBF23060B9E5C
ZUxQdF1wMesAArMzrxfbQ_QLaqJOcgOp-MhlIxDIjBJQYtESn-iD6qG9Z2PFwrlFKUBfsH0d37TEK_sHcn57oHAGsttWn78jBgueXA==
```
在这里<max-factor> = 2.7是允许的您赌注与当选验证者组中最小验证者赌注之间的最大比率。这样您可以确保您的赌注不会超过最小赌注的2.7倍，因此您的验证者的工作量最多是最低的2.7倍。如果您的赌注与其他验证者的赌注相比太大，那么它将被剪切到这个值（最小赌注的2.7倍），剩余部分将在选举后立即退还给您（即，退还给您的验证者的控制智能合约）。

现在您获得了一个十六进制的二进制字符串（654C...9E5C）和base64形式，需要由验证者签名。这可以在validator-engine-console中完成：
```
> sign BCA335626726CF2E522D287B27E4FAFFF82D1D98615957DB8E224CB397B2EB67 654C50745D7031EB0002B333AF17DB43F40B6AA24E7203A9F8C8652310C88C125062D1129FE883EAA1BD6763C5C2B94529405FB07D1DDFB4C42BFB07727E7BA07006B2DB569FBF23060B9E5C
got signature ovf9cmr2J/speJEtMU+tZm6zH/GBEyZCPpaukqL3mmNH9Wipyoys63VFh0yR386bARHKMPpfKAYBYslOjdSjCQ
```
这里BCA...B67是我们验证者的签名密钥的标识符，654...E5C是由validator-elect-req.fif生成的消息。签名是ovf9...jCQ（这是64字节Ed25519签名的base64表示）。

现在您必须运行另一个脚本validator-elect-signed.fif，它也需要验证者的公钥和签名：


```
$ fift -s validator-elect-signed.fif @my_wallet_id.addr 1567633899 2.7 C5C2B94529405FB07D1DDFB4C42BFB07727E7BA07006B2DB569FBF23060B9E5C xrQTSIQEsqZkWnoADMiBnyBFRUUweXTvzRQFqb5nHd5xmeE6 ovf9cmr2J/speJEtMU+tZm6zH/GBEyZCPpaukqL3mmNH9Wipyoys63VFh0yR386bARHKMPpfKAYBYslOjdSjCQ==
Creating a request to participate in validator elections at time 1567633899 from smart contract Uf+vF9tD9Atqok5yA6n4yGUjEMiMElBi0RKf6IPqob1nY4EA = -1:af17db43f40b6aa24e7203a9f8c8652310c88c125062d1129fe883eaa1bd6763  with maximal stake factor with respect to the minimal stake 176947/65536 and validator ADNL address c5c2b94529405fb07d1ddfb4c42bfb07727e7ba07006b2db569fbf23060b9e5c 
String to sign is: 654C50745D7031EB0002B333AF17DB43F40B6AA24E7203A9F8C8652310C88C125062D1129FE883EAA1BD6763C5C2B94529405FB07D1DDFB4C42BFB07727E7BA07006B2DB569FBF23060B9E5C
Provided a valid Ed25519 signature A2F7FD726AF627FB2978912D314FAD666EB31FF1811326423E96AE92A2F79A6347F568A9CA8CACEB7545874C91DFCE9B0111CA30FA5F28060162C94E8DD4A309 with validator public key 8404B2A6645A7A000CC8819F20454545307974EFCD1405A9BE671DDE7199E13A
query_id set to 1567632790 

Message body is x{4E73744B000000005D702D968404B2A6645A7A000CC8819F20454545307974EFCD1405A9BE671DDE7199E13A5D7031EB0002B333C5C2B94529405FB07D1DDFB4C42BFB07727E7BA07006B2DB569FBF23060B9E5C}
 x{A2F7FD726AF627FB2978912D314FAD666EB31FF1811326423E96AE92A2F79A6347F568A9CA8CACEB7545874C91DFCE9B0111CA30FA5F28060162C94E8DD4A309}
```
Saved to file validator-query.boc
-----------------------

或者，如果您在与您的钱包相同的机器上运行validator-engine-console，您可以跳过上述步骤，而是使用Validator Console中的`createelectionbid`命令直接创建一个文件（例如"validator-query.boc"），其中包含包含您签名的选举参与请求的消息体。为此命令工作，您必须使用`-f <fift-dir>`命令行选项运行validator-engine，其中<fift-dir>是包含所有必需Fift源文件副本的目录（例如Fift.fif、TonUtil.fif、validator-elect-req.fif和validator-elect-signed.fif），即使这些文件通常位于不同的源目录（<source-dir>/crypto/fift/lib和<source-dir>/crypto/smartcont）。

现在您有一个包含您的选举参与请求的消息体。您必须从控制智能合约发送它，携带赌注作为其价值（外加一个额外的克朗用于发送确认）。如果您使用简单的钱包智能合约，可以通过使用wallet.fif的`-B`命令行参数来完成：
--------------------------------------------
```
$ fift -s wallet.fif my_wallet_id -1:A4C2C7C05B093D470DE2316DBA089FA0DD775FD9B1EBFC9DC9D04B498D3A2DDA 1 100001. -B validator-query.boc 
Source wallet address = -1:af17db43f40b6aa24e7203a9f8c8652310c88c125062d1129fe883eaa1bd6763 
kf-vF9tD9Atqok5yA6n4yGUjEMiMElBi0RKf6IPqob1nY2dP
Loading private key from file my_wallet_id.pk
Transferring GR$100001. to account kf-kwsfAWwk9Rw3iMW26CJ-g3Xdf2bHr_J3J0EtJjTot2lHQ = -1:a4c2c7c05b093d470de2316dba089fa0dd775fd9b1ebfc9dc9d04b498d3a2dda seqno=0x1 bounce=-1 
Body of transfer message is x{4E73744B000000005D702D968404B2A6645A7A000CC8819F20454545307974EFCD1405A9BE671DDE7199E13A5D7031EB0002B333C5C2B94529405FB07D1DDFB4C42BFB07727E7BA07006B2DB569FBF23060B9E5C}
 x{A2F7FD726AF627FB2978912D314FAD666EB31FF1811326423E96AE92A2F79A6347F568A9CA8CACEB7545874C91DFCE9B0111CA30FA5F28060162C94E8DD4A309}

signing message: x{0000000101}
 x{627FD26163E02D849EA386F118B6DD044FD06EBBAFECD8F5FE4EE4E825A4C69D16ED32D79A60A8500000000000000000000000000001}
  x{4E73744B000000005D702D968404B2A6645A7A000CC8819F20454545307974EFCD1405A9BE671DDE7199E13A5D7031EB0002B333C5C2B94529405FB07D1DDFB4C42BFB07727E7BA07006B2DB569FBF23060B9E5C}
   x{A2F7FD726AF627FB2978912D314FAD666EB31FF1811326423E96AE92A2F79A6347F568A9CA8CACEB7545874C91DFCE9B0111CA30FA5F28060162C94E8DD4A309}

resulting external message: x{89FF5E2FB687E816D5449CE40753F190CA4621911824A0C5A2253FD107D5437ACEC6049CF8B8EA035B0446E232DB8C1DFEA97738076162B2E053513310D2A3A66A2A6C16294189F8D60A9E33D1E74518721B126A47DA3A813812959BD0BD607923B010000000080C_}
 x{627FD26163E02D849EA386F118B6DD044FD06EBBAFECD8F5FE4EE4E825A4C69D16ED32D79A60A8500000000000000000000000000001}
  x{4E73744B000000005D702D968404B2A6645A7A000CC8819F20454545307974EFCD1405A9BE671DDE7199E13A5D7031EB0002B333C5C2B94529405FB07D1DDFB4C42BFB07727E7BA07006B2DB569FBF23060B9E5C}
   x{A2F7FD726AF627FB2978912D314FAD666EB31FF1811326423E96AE92A2F79A6347F568A9CA8CACEB7545874C91DFCE9B0111CA30FA5F28060162C94E8DD4A309}

B5EE9C7241040401000000013D0001CF89FF5E2FB687E816D5449CE40753F190CA4621911824A0C5A2253FD107D5437ACEC6049CF8B8EA035B0446E232DB8C1DFEA97738076162B2E053513310D2A3A66A2A6C16294189F8D60A9E33D1E74518721B126A47DA3A813812959BD0BD607923B010000000080C01016C627FD26163E02D849EA386F118B6DD044FD06EBBAFECD8F5FE4EE4E825A4C69D16ED32D79A60A85000000000000000000000000000010201A84E73744B000000005D702D968404B2A6645A7A000CC8819F20454545307974EFCD1405A9BE671DDE7199E13A5D7031EB0002B333C5C2B94529405FB07D1DDFB4C42BFB07727E7BA07006B2DB569FBF23060B9E5C030080A2F7FD726AF627FB2978912D314FAD666EB31FF1811326423E96AE92A2F79A6347F568A9CA8CACEB7545874C91DFCE9B0111CA30FA5F28060162C94E8DD4A309062A7721
(Saved to file wallet-query.boc)
```
----------------------------------

现在您只需从Lite客户端发送wallet-query.boc（而不是验证者控制台）：
```
> sendfile wallet-query.boc
```
或者您可以使用Lite客户端的批处理模式：
```
$ lite-client -C <config-file> -c "sendfile wallet-query.boc"
```
这是一个由您的私钥签名的外部消息（控制您的钱包）；它指示您的钱包智能合约向选举智能合约发送一个内部消息，其中包含预定的有效载荷（包含验证者出价并由其密钥签名）并转移指定数量的克朗。当选举智能合约收到这个内部消息时，它会注册您的出价（赌注等于指定的克朗数量减一），并向您（即，钱包智能合约）发送确认（携带1克朗减去消息转发费的变更）或带有错误代码的拒绝消息（携带几乎所有原始赌注金额减去处理费）。

您可以通过运行选举智能合约的get-method "participant_list"来检查您的赌注是否被接受。

6. 回收赌注和奖金

如果您的赌注在选举中只被部分接受（因为<max-factor>），或者在您的赌注解冻后（这发生在您的验证者当选的验证者组的任期结束后的某个时间），您可能想要收回全部或部分赌注，以及您的验证者应得的奖金份额。选举智能合约不会将赌注和奖金以消息的形式发送给您（即，控制智能合约）。相反，它会将应退还给您的金额记入一个特殊表中，您可以使用get-method "compute_returned_stake"（它期望控制智能合约的地址作为参数）来检查：
```
$ lite-client -C global-config.json -rc 'runmethod -1:A4C2C7C05B093D470DE2316DBA089FA0DD775FD9B1EBFC9DC9D04B498D3A2DDA compute_returned_stake 0xaf17db43f40b6aa24e7203a9f8c8652310c88c125062d1129fe883eaa1bd6763'
arguments:  [ 79196899299028790296381692623119733846152089453039582491866112477478757689187 130944 ]
result: [ 0 ]
```
如果结果为零，则没有欠您的款项。否则，您将看到部分或全部赌注，可能还有一些奖金。在这种情况下，您可以使用recover-stake.fif创建赌注回收请求：


-----------------------------
```
$ fift -s recover-stake.fif
query_id for stake recovery message is set to 1567634299 

Message body is x{47657424000000005D70337B}
```
Saved to file recover-query.boc
-----------------------------

再次，您必须将recover-query.boc作为消息的有效载荷从控制智能合约（即，您的钱包）发送到选举智能合约：

```
$ fift -s wallet.fif my_wallet_id <dest-addr> <my-wallet-seqno> <gram-amount> -B recover-query.boc
```
For example,
```
$ fift -s wallet.fif my_wallet_id -1:A4C2C7C05B093D470DE2316DBA089FA0DD775FD9B1EBFC9DC9D04B498D3A2DDA 2 1. -B recover-query.boc 
...
```
(Saved to file wallet-query.boc)

请注意，此消息携带一个小值（一个克朗）仅用于支付消息转发和处理费。如果您指示的值为零，则选举智能合约将不会处理该消息（在TON区块链的上下文中，几乎没有任何用途的消息的值恰好为零）。

一旦准备好wallet-query.boc，您可以从Lite客户端发送它：

```
$ liteclient -C <config> -c 'sendfile wallet-query.boc'
```
如果您做的一切正确（特别是指示了您的钱包的正确seqno而不是示例中的“2”），您将从选举智能合约获得一个消息，其中包含您请求的少量价值的变更（本例中为1.克朗）加上恢复的赌注和奖金部分。

7. 参加下一次选举

请注意，即使在包含您的当选验证者的验证者组的任期结束之前，也会宣布下一组验证者的新的选举。您可能也希望参加它们。
为此，您可以使用相同的验证者，但您必须生成一个新的验证者密钥和新的ADNL地址。
您还必须在之前的赌注被退回之前做出新的赌注（因为您的之前的赌注只有在下一组验证者变得活跃后的某个时间才会解冻并退回），
所以如果您想参加并发选举，那么将您的克朗赌注超过一半可能没有意义。
