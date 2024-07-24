本文的目的是提供一个关于 TON DNS 的简要介绍，TON DNS 是一个将人类可读的域名（如 `test.ton` 或 `mysite.temp.ton`）转换为 TON 智能合约地址、TON 网络中服务使用的 ADNL 地址（如 TON Sites）等的服务。

1. 域名

TON DNS 使用熟悉的域名格式，域名由 UTF-8 编码的字符串组成，长度最多为 126 字节，不同部分通过点（`.`）分隔。域名中不允许出现空字符（即零字节），以及更一般的 0 到 32 范围内的字节。例如，`test.ton` 和 `mysite.temp.ton` 是有效的 TON DNS 域名。与通常的域名不同的是，TON DNS 域名是区分大小写的；如果需要不区分大小写，可以在执行 TON DNS 查找之前将所有域名转换为小写。

目前，仅以 `.ton` 结尾的域名被认为是有效的 TON DNS 域名。这一规则将来可能会改变。然而，请注意，定义与互联网中已有的顶级域名（如 `.com` 或 `.to`）相同的顶级域名是不明智的，因为有人可能会注册一个 TON 域名 `google.com`，在此 TON 域名上部署一个 TON 站点，从其他看似无害的 TON 站点创建一个隐蔽链接到该 TON 站点的页面，并从毫无察觉的访客那里窃取 `google.com` 的 Cookie。

在内部，TON DNS 处理域名的方式如下。首先，将域名按点字符 `.` 切分成各个组件。然后，为每个组件追加空字符，并将所有组件按相反的顺序拼接在一起。例如，`google.com` 变成 `com\0google\0`。


2. 解析 TON DNS 域名

TON DNS 域名的解析过程如下。首先，通过检查最近的主链状态中的配置参数 #4 来定位 *根 DNS 智能合约*。该参数包含根 DNS 智能合约在主链中的 256 位地址。

然后，调用根 DNS 智能合约的特殊获取方法 `dnsresolve`（方法 ID 123660），传入两个参数。第一个参数是一个包含域名内部表示的 CellSlice，其数据位长度为 *8n*，其中 *n* 是内部表示的字节长度（最多 127 字节）。第二个参数是一个带符号的 16 位整数，表示所需的 *类别*。如果类别为零，则请求所有类别。

如果该获取方法失败，则 TON DNS 查找不成功。否则，该获取方法返回两个值。第一个是 *8m*，表示解析的域名内部表示前缀的长度（以位为单位），0 < m <= n。第二个是一个 Cell，包含所需域名和类别的 TON DNS 记录，或者是根字典，其中包含 16 位带符号整数键（类别）和值等于相应 TON DNS 记录的序列化形式。如果根 DNS 智能合约无法解析该域名，即没有非空前缀是智能合约已知的有效域名，则返回 (0, null)。换句话说，m = 0 表示 TON DNS 查找没有找到所需域名的数据。在这种情况下，TON DNS 查找也会失败。

如果 m = n，则结果的第二部分是一个包含所需域名和类别的有效 TON DNS 记录的 Cell，或者是 Null（如果该域名没有该类别的 TON DNS 记录）。无论哪种情况，解析过程停止，获得的 TON DNS 记录将被反序列化，并获取所需的信息（如记录类型及其参数，如智能合约地址或 ADNL 地址）。

最后，如果 m < n，则查找目前已成功，但仅对原始内部表示的 m 字节前缀提供了部分结果。返回的是所有已知的最长前缀。例如，尝试在根 DNS 智能合约中查找 `mysite.test.ton`（即内部表示中的 `ton\0test\0mysite\0`）可能会返回 8m=72，对应于前缀 `ton\0test\0`，即通常域名表示中的子域名 "test.ton"。在这种情况下，dnsresolve() 返回此前缀的类别 -1 的值，无论客户端最初请求的类别是什么。按照惯例，类别 -1 通常包含 *dns_next_resolver* 类型的 TON DNS 记录，包含下一个解析器智能合约的地址（该智能合约可以存在于任何其他工作链中，如基础链）。如果确实如此，解析过程将通过运行获取方法 `dnsresolve` 来继续进行，使用下一个解析器的内部表示域名（仅包含到目前为止未解析的部分）。然后，下一个解析器智能合约要么报告错误或没有记录，要么返回最终结果，或者返回另一个前缀和下一个解析器智能合约。在后一种情况下，过程将继续进行，直到解析完所有原始域名为止。


3. 使用 LiteClient 和 TonLib 解析 TON DNS 域名

上述过程可以通过使用 TON LiteClient 或 TONLib 自动进行。例如，可以在 LiteClient 中调用命令 `dnsresolve test.ton 1` 来解析 "test.ton" 的类别为 1 的记录，并获得以下结果：

================================================

> dnsresolve test.ton
...
Result for domain 'test.ton' category 1
raw data: x{AD011B3CBBE404F47FFEF92D0D7894C5C6F215F677732A49E544F16D1E75643D46AB00}

category #1 : (dns_adnl_address adnl_addr:x1B3CBBE404F47FFEF92D0D7894C5C6F215F677732A49E544F16D1E75643D46AB flags:0)
	adnl address 1B3CBBE404F47FFEF92D0D7894C5C6F215F677732A49E544F16D1E75643D46AB = UNTZO7EAT2H77XZFUGXRFGFY3ZBL5TXOMVETZKE6FWR45LEHVDKXAUY

================================================


在这种情况下，"test.ton" 的 TON DNS 记录是一个 `dns_adnl_address` 记录，包含 ADNL 地址 UNTZO7EAT2H77XZFUGXRFGFY3ZBL5TXOMVETZKE6FWR45LEHVDKXAUY。

另一种方法是使用 `tonlib-cli` 并输入以下命令：

================================================

> dns resolve root test.ton 1
Redirect resolver
...
Done
  test.ton 1 ADNL:untzo7eat2h77xzfugxrfgfy3zbl5txomvetzke6fwr45lehvdkxauy

================================================


这是一种更简洁的结果表示方式。

最后，如果使用 RLDP-HTTP Proxy 的客户端模式从浏览器访问 TON 网站，如 `TONSites-HOWTO.txt` 中所述，TONLib 解析器会自动调用来解析终端用户输入的所有域名，因此对 `http://test.ton/testnet/last` 的 HTTP 查询会自动通过 RLDP 转发到 ADNL 地址 `untzo7eat2h77xzfugxrfgfy3zbl5txomvetzke6fwr45lehvdkxauy`。

4. 注册新域名

假设您有一个新的 TON 网站，具有新生成的 ADNL 地址，例如 `vcqmha5j3ceve35ammfrhqty46rkhi455otydstv66pk2tmf7rl25f3`。当然，最终用户可以输入 `http://vcqmha5j3ceve35ammfrhqty46rkhi455otydstv66pk2tmf7rl25f3.adnl/` 来访问您的 TON 网站，但这并不方便。相反，您可以注册一个新域名，比如 `mysite.temp.ton`，并在类别 1 中包含 `dns_adnl_address` 记录，记录中包含您网站的 ADNL 地址 `vcq...25f3`。这样，用户只需在浏览器中输入 `mysite.temp.ton` 即可访问您的网站。

一般来说，您需要联系上级域名的所有者，要求他在他的 DNS 解析器智能合约中添加您的子域名记录。然而，TON 区块链的测试网络中有一个特殊的解析器智能合约用于 `temp.ton`，允许任何人自动注册尚未注册的 `temp.ton` 的子域名，只需向该智能合约支付少量费用（以测试 Gram 计）。在这种情况下，我们首先需要找出该智能合约的地址，例如通过使用 Lite Client：


================================================

> dnsresolve temp.ton -1
...
category #-1 : (dns_next_resolver
  resolver:(addr_std
    anycast:nothing workchain_id:0 address:x190BD756F6C0E7948DC26CB47968323177FB20344F8F9A50918CAF87ECB34B79))
	next resolver 0:190BD756F6C0E7948DC26CB47968323177FB20344F8F9A50918CAF87ECB34B79 = EQAZC9dW9sDnlI3CbLR5aDIxd_sgNE-PmlCRjK-H7LNLeUXN

================================================

我们看到这个自动 DNS 智能合约的地址是 EQAZC9dW9sDnlI3CbLR5aDIxd_sgNE-PmlCRjK-H7LNLeUXN。我们可以运行几个获取方法来计算注册子域名所需的价格，并了解子域名将被注册的期限：

================================================

> runmethod EQAZC9dW9sDnlI3CbLR5aDIxd_sgNE-PmlCRjK-H7LNLeUXN getstdperiod
...
arguments:  [ 67418 ] 
result:  [ 700000 ] 
remote result (not to be trusted):  [ 700000 ] 
> runmethod EQAZC9dW9sDnlI3CbLR5aDIxd_sgNE-PmlCRjK-H7LNLeUXN getppr
...
arguments:  [ 109522 ] 
result:  [ 100000000 ] 
remote result (not to be trusted):  [ 100000000 ] 

================================================

我们看到子域名的注册期限为 700000 秒（大约八天），注册价格为 100000000 ng = 0.1 测试 Gram 每个域名，加上每比特和每个存储单元的额外费用，这些费用可以通过运行 `getppb` 和 `getppc` 方法来了解。

现在我们希望这个智能合约注册我们的子域名。为此，我们必须从我们的钱包创建一条特别的消息，发送给自动 DNS 智能合约。假设我们有一个钱包 `my_new_wallet`，地址为 kQABzslAMKOVwkSkkWfelS1pYSDOSyTcgn0yY_loQvyo_ZgI。我们可以运行以下 Fift 脚本（来自源代码树的 `crypto/smartcont` 子目录）：

```bash
fift -s auto-dns.fif <auto-dns-smc-addr> add <my-subdomain> <expire-time> owner <my-wallet-addr> cat 1 adnl <my-site-adnl-address>
```
For example:

===============================================

$ fift -s auto-dns.fif EQAZC9dW9sDnlI3CbLR5aDIxd_sgNE-PmlCRjK-H7LNLeUXN add 'mysite' 700000 owner kQABzslAMKOVwkSkkWfelS1pYSDOSyTcgn0yY_loQvyo_ZgI cat 1 adnl vcqmha5j3ceve35ammfrhqty46rkhi455otydstv66pk2tmf7rl25f3
Automatic DNS smart contract address = 0:190bd756f6c0e7948dc26cb47968323177fb20344f8f9a50918caf87ecb34b79 
kQAZC9dW9sDnlI3CbLR5aDIxd_sgNE-PmlCRjK-H7LNLef5H
Action: add mysite 1583865040 
Operation code: 0x72656764 
Value: x{2_}
 x{BC000C_}
  x{AD0145061C1D4EC44A937D0318589E13C73D151D1CEF5D3C0E53AFBCF56A6C2FE2BD00}
 x{BFFFF4_}
  x{9FD3800039D928061472B84894922CFBD2A5AD2C2419C9649B904FA64C7F2D085F951FA01_}

Internal message body is: x{726567645E5D2E700481CE3F0EDAF2E6D2E8CA00BCCFB9A1_}
 x{2_}
  x{BC000C_}
   x{AD0145061C1D4EC44A937D0318589E13C73D151D1CEF5D3C0E53AFBCF56A6C2FE2BD00}
  x{BFFFF4_}
   x{9FD3800039D928061472B84894922CFBD2A5AD2C2419C9649B904FA64C7F2D085F951FA01_}

B5EE9C7241010601007800012F726567645E5D2E700481CE3F0EDAF2E6D2E8CA00BCCFB9A10102012002030105BC000C040105BFFFF4050046AD0145061C1D4EC44A937D0318589E13C73D151D1CEF5D3C0E53AFBCF56A6C2FE2BD0000499FD3800039D928061472B84894922CFBD2A5AD2C2419C9649B904FA64C7F2D085F951FA01070E6337D
Query_id is 6799642071046147647 = 0x5E5D2E700481CE3F 
(Saved to file dns-msg-body.boc)

================================================

我们看到，这个查询的内部消息体已经被创建并保存到文件 `dns-msg-body.boc` 中。现在，你需要从你的钱包 kQAB..ZgI 向自动 DNS 智能合约 EQA..UXN 发送支付，同时附上来自文件 `dns-msg-body.boc` 的消息体，以便自动 DNS 智能合约知道你希望它执行什么操作。如果你的钱包是通过 `new-wallet.fif` 创建的，你可以在进行转账时简单地使用 `-B` 命令行参数来调用 `wallet.fif`：


================================================

$ fift -s wallet.fif my_new_wallet EQAZC9dW9sDnlI3CbLR5aDIxd_sgNE-PmlCRjK-H7LNLeUXN 1 1.7 -B dns-msg-body.boc
Source wallet address = 0:01cec94030a395c244a49167de952d696120ce4b24dc827d3263f96842fca8fd 
kQABzslAMKOVwkSkkWfelS1pYSDOSyTcgn0yY_loQvyo_ZgI
Loading private key from file my_new_wallet.pk
Transferring GR$1.7 to account kQAZC9dW9sDnlI3CbLR5aDIxd_sgNE-PmlCRjK-H7LNLef5H = 0:190bd756f6c0e7948dc26cb47968323177fb20344f8f9a50918caf87ecb34b79 seqno=0x1 bounce=-1 
Body of transfer message is x{726567645E5D2E700481CE3F0EDAF2E6D2E8CA00BCCFB9A1_}
 x{2_}
  x{BC000C_}
   x{AD0145061C1D4EC44A937D0318589E13C73D151D1CEF5D3C0E53AFBCF56A6C2FE2BD00}
  x{BFFFF4_}
   x{9FD3800039D928061472B84894922CFBD2A5AD2C2419C9649B904FA64C7F2D085F951FA01_}

signing message: x{0000000103}
 x{62000C85EBAB7B6073CA46E1365A3CB41918BBFD901A27C7CD2848C657C3F659A5BCA32A9F880000000000000000000000000000726567645E5D2E700481CE3F0EDAF2E6D2E8CA00BCCFB9A1_}
  x{2_}
   x{BC000C_}
    x{AD0145061C1D4EC44A937D0318589E13C73D151D1CEF5D3C0E53AFBCF56A6C2FE2BD00}
   x{BFFFF4_}
    x{9FD3800039D928061472B84894922CFBD2A5AD2C2419C9649B904FA64C7F2D085F951FA01_}

resulting external message: x{8800039D928061472B84894922CFBD2A5AD2C2419C9649B904FA64C7F2D085F951FA050E3817FC01F564AECE810B8077D72E3EE15C81392E8B4AE9CDD0D6575821481C996AE8FFBABA0513F131E10E27C006C6544E99D71E0A6AACF7D02C677342B040000000081C_}
 x{62000C85EBAB7B6073CA46E1365A3CB41918BBFD901A27C7CD2848C657C3F659A5BCA32A9F880000000000000000000000000000726567645E5D2E700481CE3F0EDAF2E6D2E8CA00BCCFB9A1_}
  x{2_}
   x{BC000C_}
    x{AD0145061C1D4EC44A937D0318589E13C73D151D1CEF5D3C0E53AFBCF56A6C2FE2BD00}
   x{BFFFF4_}
    x{9FD3800039D928061472B84894922CFBD2A5AD2C2419C9649B904FA64C7F2D085F951FA01_}

B5EE9C72410207010001170001CF8800039D928061472B84894922CFBD2A5AD2C2419C9649B904FA64C7F2D085F951FA050E3817FC01F564AECE810B8077D72E3EE15C81392E8B4AE9CDD0D6575821481C996AE8FFBABA0513F131E10E27C006C6544E99D71E0A6AACF7D02C677342B040000000081C01019762000C85EBAB7B6073CA46E1365A3CB41918BBFD901A27C7CD2848C657C3F659A5BCA32A9F880000000000000000000000000000726567645E5D2E700481CE3F0EDAF2E6D2E8CA00BCCFB9A10202012003040105BC000C050105BFFFF4060046AD0145061C1D4EC44A937D0318589E13C73D151D1CEF5D3C0E53AFBCF56A6C2FE2BD0000499FD3800039D928061472B84894922CFBD2A5AD2C2419C9649B904FA64C7F2D085F951FA01031E3A74C
(Saved to file wallet-query.boc)

=====================================================

（你需要将 1 替换为你钱包的正确序列号。）一旦你获得了一个签名的外部消息文件 `wallet-query.boc`，该消息是发送给你的钱包的，并指示其将 1.7 测试 Gram 转账到自动 DNS 智能合约，并附带你要注册的新域名的描述，你可以使用 LiteClient 上传这个消息，通过输入以下命令：

=====================================================
> sendfile wallet-query.boc 
[ 1][t 1][!testnode]	sending query from file wallet-query.boc
[ 3][t 1][!query]	external message status is 1
=====================================================

如果一切正常，你会收到来自自动 DNS 智能合约的确认消息，其中会包含一些找零（它只会收取你子域名的存储费用和运行智能合约及发送消息的处理费用，其余部分会退还给你），同时你的新域名将会被注册。

=====================================================
> last
...
> dnsresolve mysite.temp.ton 1
...
Result for domain 'mysite.temp.ton' category 1
category #1 : (dns_adnl_address adnl_addr:x45061C1D4EC44A937D0318589E13C73D151D1CEF5D3C0E53AFBCF56A6C2FE2BD flags:0)
	adnl address 45061C1D4EC44A937D0318589E13C73D151D1CEF5D3C0E53AFBCF56A6C2FE2BD = vcqmha5j3ceve35ammfrhqty46rkhi455otydstv66pk2tmf7rl25f3


=====================================================

你可以以类似的方式修改或延长此域名，首先使用 `auto-dns.fif` 创建一个请求文件 `dns-msg-body.boc`，采用如 `update` 或 `prolong` 等操作，然后将该请求嵌入到从你的钱包发送给自动 DNS 智能合约的消息中，使用 `wallet.fif` 或类似的脚本，并加上命令行参数 `-B dns-msg-body.boc`。

