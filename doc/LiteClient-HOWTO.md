### 目标

本文件旨在提供在 TON 区块链测试网络中编译和创建简单智能合约（如简单钱包）的逐步说明，使用 TON 区块链轻客户端及其相关软件。

有关下载和安装说明，请参见 README。我们在此假设 Lite 客户端已经正确下载、编译和安装。

### 1. 智能合约地址

TON 网络中的智能合约地址由两个部分组成：(a) 工作链 ID（一个签名的 32 位整数）和 (b) 工作链内部的地址（根据工作链的不同为 64-512 位）。目前，TON 区块链测试网络中运行的仅有主链（workchain_id=-1）和偶尔的基础工作链（workchain_id=0）。这两者都有 256 位的地址，因此我们假设工作链 ID 为 0 或 -1，并且工作链内部的地址恰好是 256 位。

在上述条件下，智能合约地址可以表示为以下形式：

A) "原始"：<十进制工作链 ID>:<64 个十六进制数字的地址>

B) "用户友好"：首先生成：
- 一个标记字节（对于 "可回退" 地址为 0x11，对于 "不可回退" 地址为 0x51；如果地址不应被生产网络中的软件接受，添加 +0x80）
- 一个包含签名 8 位整数的字节（基础工作链为 0x00，主链为 0xff）
- 32 个字节包含 256 位的智能合约地址（大端格式）
- 2 个字节包含前 34 个字节的 CRC16-CCITT

在 B) 的情况下，生成的 36 个字节然后使用 base64（即，使用数字、大写和小写拉丁字母、'/' 和 '+'）或 base64url（使用 '_' 和 '-' 代替 '/' 和 '+'）进行编码，得到 48 个可打印的非空格字符。

示例：

"测试给予者"（一个特殊的智能合约，位于测试网络的主链中，给任何请求者提供最多 20 个测试 Gram）具有以下地址：

-1:fcb91a3a3816d0f7b8c2c76108b8a9bc5a6b7a55bd79f8ab101c52db29232260

在 "原始" 形式中（注意大写拉丁字母 'A'..'F' 也可以使用 'a'..'f'）

以及

kf/8uRo6OBbQ97jCx2EIuKm8Wmt6Vb15-KsQHFLbKSMiYIny（base64）
kf_8uRo6OBbQ97jCx2EIuKm8Wmt6Vb15-KsQHFLbKSMiYIny（base64url）

在 "用户友好" 形式中（供用户友好的客户端显示）。注意这两种形式（base64 和 base64url）都是有效的，并且都必须被接受。

顺便提一下，TON 区块链的其他二进制数据也有类似的 "装甲" base64 表示，区别在于它们的首字节。例如，广泛使用的 256 位 Ed25519 公钥表示为首先创建一个 36 字节的序列，如下：
- 一个标记字节 0x3E，表示这是一个公钥
- 一个标记字节 0xE6，表示这是一个 Ed25519 公钥
- 32 字节包含 Ed25519 公钥的标准二进制表示
- 2 字节包含前 34 个字节的 CRC16-CCITT 的大端表示

生成的 36 字节序列转换为 48 字符的 base64 或 base64url 字符串。例如，Ed25519 公钥 E39ECDA0A7B0C60A7107EC43967829DBE8BC356A49B9DFC6186B3EAC74B5477（通常表示为 32 字节序列 0xE3, 0x9E, ..., 0x7D）具有以下 "装甲" 表示：

Pubjns2gp7DGCnEH7EOWeCnb6Lw1akm538YYaz6sdLVHfRB2

### 2. 检查智能合约的状态

使用 TON Lite 客户端检查智能合约的状态非常简单。对于上述示例智能合约，您可以运行 Lite 客户端并输入以下命令：
```
> last
...
> getaccount -1:fcb91a3a3816d0f7b8c2c76108b8a9bc5a6b7a55bd79f8ab101c52db29232260
or
> getaccount kf_8uRo6OBbQ97jCx2EIuKm8Wmt6Vb15-KsQHFLbKSMiYIny
```
You will see something like this:

------------------------------------
```
got account state for -1 : FCB91A3A3816D0F7B8C2C76108B8A9BC5A6B7A55BD79F8AB101C52DB29232260 with respect to blocks (-1,8000000000000000,2075):BFE876CE2085274FEDAF1BD80F3ACE50F42B5A027DF230AD66DCED1F09FB39A7:522C027A721FABCB32574E3A809ABFBEE6A71DE929C1FA2B1CD0DDECF3056505
account state is (account
  addr:(addr_std
    anycast:nothing workchain_id:-1 address:xFCB91A3A3816D0F7B8C2C76108B8A9BC5A6B7A55BD79F8AB101C52DB29232260)
  storage_stat:(storage_info
    used:(storage_used
      cells:(var_uint len:1 value:3)
      bits:(var_uint len:2 value:707)
      public_cells:(var_uint len:0 value:0)) last_paid:1568899526
    due_payment:nothing)
  storage:(account_storage last_trans_lt:2310000003
    balance:(currencies
      grams:(nanograms
        amount:(var_uint len:6 value:9998859500889))
      other:(extra_currencies
        dict:hme_empty))
    state:(account_active
      (
        split_depth:nothing
        special:nothing
        code:(just
          value:(raw@^Cell 
            x{}
             x{FF0020DD2082014C97BA9730ED44D0D70B1FE0A4F260D31F01ED44D0D31FD166BAF2A1F8000120D74A8E11D307D459821804A817C80073FB0201FB00DED1A4C8CB1FC9ED54}
            ))
        data:(just
          value:(raw@^Cell 
            x{}
             x{00009A15}
            ))
        library:hme_empty))))
x{CFFFCB91A3A3816D0F7B8C2C76108B8A9BC5A6B7A55BD79F8AB101C52DB2923226020680B0C2EC1C0E300000000226BF360D8246029DFF56534_}
 x{FF0020DD2082014C97BA9730ED44D0D70B1FE0A4F260D31F01ED44D0D31FD166BAF2A1F8000120D74A8E11D307D459821804A817C80073FB0201FB00DED1A4C8CB1FC9ED54}
 x{00000003}
last transaction lt = 2310000001 hash = 73F89C6F8910F598AD84504A777E5945C798AC8C847FF861C090109665EAC6BA
```
------------------------------------

### 第一部分：获取账户状态

第一行信息“获取账户状态 ... 对于 ...”显示了账户地址和与之相关的主链区块标识符，该标识符表示账户状态已被转储。请注意，即使账户状态在后续区块中发生变化，`getaccount xxx` 命令也将返回相同的结果，直到通过 `last` 命令将参考区块更新为较新的值。通过这种方式，可以研究所有账户的状态并获得一致的结果。

“账户状态是（账户 ...” 行开始了账户状态的格式化解码视图。这是 TL-B 数据类型 Account 的解码，TL-B 用于表示 TON 区块链中的账户状态，如 TON 区块链文档中所述。（您可以在源文件 crypto/block/block.tlb 中找到用于解码的 TL-B 方案；如果方案过时，解码可能会出现问题。）

最后几行以 x{CFF538...（“原始转储”）开头，包含以树状结构显示的相同信息。在这种情况下，我们有一个根单元格，包含数据位 CFF...134_（下划线表示去除最后一个二进制1及其后所有的二进制零，因此十六进制“4_”对应二进制“0”），以及两个子单元格（以一个空格的缩进显示）。

我们可以看到 x{FF0020DD20...} 是此智能合约的代码。如果我们参考 TON 虚拟机文档的附录 A，甚至可以反汇编此代码：FF00 是 SETCP 0，20 是 DUP，DD 是 IFNOTRET，20 是 DUP，依此类推。（顺便提一下，您可以在源文件 crypto/block/new-testgiver.fif 中找到此智能合约的源代码。）

我们还可以看到 x{00009A15}（实际值可能不同）是此智能合约的持久数据。它实际上是一个无符号的 32 位整数，智能合约用作已执行操作的计数器。请注意，这个值是大端格式的（即，3 编码为 x{00000003}，而不是 x{03000000}），TON 区块链中的所有整数都是如此。在这种情况下，计数器等于 0x9A15 = 39445。

智能合约的当前余额可以在格式化输出部分轻松看到。在这种情况下，我们看到 ... balance:(currencies:(grams:(nanograms:(... value:1000000000000000...))))，这是账户的余额（以测试纳诺克拉姆为单位，在这个例子中是一百万测试克拉姆；实际数字可能会更小）。如果您研究 crypto/block/scheme.tlb 中提供的 TL-B 方案，您也可以在原始转储部分找到这个数字（10^15）的大端二进制形式（它位于根单元格数据位的末尾附近）。

### 3. 编译新的智能合约

在将新的智能合约上传到 TON 区块链之前，您需要确定其代码和数据，并将其以序列化形式保存到一个文件中（称为“单元格包”或 BOC 文件，通常以 .boc 为后缀）。让我们考虑一个简单钱包智能合约的案例，该合约在其持久数据中存储一个 32 位操作计数器和一个 256 位 Ed25519 公钥。

显然，您需要一些用于开发智能合约的工具，即 TON 智能合约编译器。基本上，TON 智能合约编译器是一个程序，它读取用专门的高级编程语言编写的智能合约源代码，并从该源代码创建一个 .boc 文件。

其中一个工具是 Fift 解释器，它包含在此发行版中，可以帮助创建简单的智能合约。更大的智能合约应使用更复杂的工具进行开发（例如，FunC 编译器，它从 FunC 源文件创建 Fift 汇编文件；您可以在目录 `crypto/smartcont` 中找到一些 FunC 智能合约源文件）。然而，Fift 对于演示目的来说是足够的。

考虑文件 `new-wallet.fif`（通常位于源目录的 `crypto/smartcont/new-wallet.fif`）包含一个简单钱包智能合约的源代码：

------------------------------------
```
#!/usr/bin/env fift -s
"TonUtil.fif" include
"Asm.fif" include

{ ."usage: " @' $0 type ." <workchain-id> [<filename-base>]" cr
  ."Creates a new wallet in specified workchain, with private key saved to or loaded from <filename-base>.pk" cr
  ."('new-wallet.pk' by default)" cr 1 halt
} : usage
$# 1- -2 and ' usage if

$1 parse-workchain-id =: wc    // set workchain id from command line argument
def? $2 { @' $2 } { "new-wallet" } cond constant file-base

."Creating new wallet in workchain " wc . cr

// Create new simple wallet
<{ SETCP0 DUP IFNOTRET // return if recv_internal
   DUP 85143 INT EQUAL IFJMP:<{ // "seqno" get-method
     DROP c4 PUSHCTR CTOS 32 PLDU  // cnt
   }>
   INC 32 THROWIF  // fail unless recv_external
   512 INT LDSLICEX DUP 32 PLDU   // sign cs cnt
   c4 PUSHCTR CTOS 32 LDU 256 LDU ENDS  // sign cs cnt cnt' pubk
   s1 s2 XCPU            // sign cs cnt pubk cnt' cnt
   EQUAL 33 THROWIFNOT   // ( seqno mismatch? )
   s2 PUSH HASHSU        // sign cs cnt pubk hash
   s0 s4 s4 XC2PU        // pubk cs cnt hash sign pubk
   CHKSIGNU              // pubk cs cnt ?
   34 THROWIFNOT         // signature mismatch
   ACCEPT
   SWAP 32 LDU NIP 
   DUP SREFS IF:<{
     // 3 INT 35 LSHIFT# 3 INT RAWRESERVE    // reserve all but 103 Grams from the balance
     8 LDU LDREF         // pubk cnt mode msg cs
     s0 s2 XCHG SENDRAWMSG  // pubk cnt cs ; ( message sent )
   }>
   ENDS
   INC NEWC 32 STU 256 STU ENDC c4 POPCTR
}>c // >libref
// code
<b 0 32 u, 
   file-base +".pk" load-generate-keypair
   constant wallet_pk
   B, 
b> // data
null // no libraries
// Libs{ x{ABACABADABACABA} drop x{AAAA} s>c public_lib x{1234} x{5678} |_ s>c public_lib }Libs
<b b{0011} s, 3 roll ref, rot ref, swap dict, b>  // create StateInit
dup ."StateInit: " <s csr. cr
dup hash wc swap 2dup 2constant wallet_addr
."new wallet address = " 2dup .addr cr
2dup file-base +".addr" save-address-verbose
."Non-bounceable address (for init): " 2dup 7 .Addr cr
."Bounceable address (for later access): " 6 .Addr cr
<b 0 32 u, b>
dup ."signing message: " <s csr. cr
dup hash wallet_pk ed25519_sign_uint rot
<b b{1000100} s, wallet_addr addr, b{000010} s, swap <s s, b{0} s, swap B, swap <s s, b>
dup ."External message for initialization is " <s csr. cr
2 boc+>B dup Bx. cr
file-base +"-query.boc" tuck B>file
."(Saved wallet creating query to file " type .")" cr
```
--------------------------------------------


（您发行版中的实际源文件可能会有所不同。）本质上，这是一个完整的 Fift 脚本，用于创建一个由新生成的密钥对控制的新智能合约实例。该脚本接受命令行参数，因此您无需每次创建新钱包时都编辑源文件。

现在，假设您已经编译了 Fift 二进制文件（通常位于构建目录下的 "crypto/fift"），您可以运行以下命令：

```
$ crypto/fift -I<source-directory>/crypto/fift/lib -s <source-directory>/crypto/smartcont/new-wallet.fif 0 my_wallet_name
```

其中 `0` 是包含新钱包的工作链（0 = 基本链，-1 = 主链），`my_wallet_name` 是您希望与此钱包关联的任何标识符。新钱包的地址将保存到文件 `my_wallet_name.addr`，其新生成的私钥将保存到 `my_wallet_name.pk`（除非该文件已经存在；此时将从该文件加载密钥），外部消息将保存到 `my_wallet_name-query.boc`。如果您未指定钱包名称（上例中的 `my_wallet_name`），则使用默认名称 `new-wallet`。

您可以选择将 FIFTPATH 环境变量设置为 `<source-directory>/crypto/fift/lib:<source-directory>/crypto/smartcont`，分别包含 Fift.fif 和 Asm.fif 库文件以及示例智能合约源文件；然后您可以省略 Fift 解释器的 -I 参数。如果您将 Fift 二进制文件 `crypto/fift` 安装到包含在 PATH 中的目录（例如 `/usr/bin/fift`），您可以简单地调用：

$ fift -s new-wallet.fif 0 my_wallet_name

instead of indicating the complete search paths in the command line.

If everything worked, you'll see something like the following

--------------------------------------------
```
Creating new wallet in workchain 0 
Saved new private key to file my_wallet_name.pk
StateInit: x{34_}
 x{FF0020DD2082014C97BA9730ED44D0D70B1FE0A4F260810200D71820D70B1FED44D0D31FD3FFD15112BAF2A122F901541044F910F2A2F80001D31F3120D74A96D307D402FB00DED1A4C8CB1FCBFFC9ED54}
 x{00000000C59DC52962CC568AC5E72735EABB025C5BDF457D029AEEA6C2FFA5EB2A945446}

new wallet address = 0:2ee9b4fd4f077c9b223280c35763df9edab0b41ac20d36f4009677df95c3afe2 
(Saving address to file my_wallet_name.addr)
Non-bounceable address (for init): 0QAu6bT9Twd8myIygMNXY9-e2rC0GsINNvQAlnfflcOv4uVb
Bounceable address (for later access): kQAu6bT9Twd8myIygMNXY9-e2rC0GsINNvQAlnfflcOv4rie
signing message: x{00000000}

External message for initialization is x{88005DD369FA9E0EF93644650186AEC7BF3DB5616835841A6DE8012CEFBF2B875FC41190260D403E40B2EE8BEB2855D0F4447679D9B9519BE64BE421166ABA2C66BEAAAF4EBAF8E162886430243216DDA10FCE68C07B6D7DDAA3E372478D711E3E1041C00000001_}
 x{FF0020DD2082014C97BA9730ED44D0D70B1FE0A4F260810200D71820D70B1FED44D0D31FD3FFD15112BAF2A122F901541044F910F2A2F80001D31F3120D74A96D307D402FB00DED1A4C8CB1FCBFFC9ED54}
 x{00000000C59DC52962CC568AC5E72735EABB025C5BDF457D029AEEA6C2FFA5EB2A945446}

B5EE9C724104030100000000E50002CF88005DD369FA9E0EF93644650186AEC7BF3DB5616835841A6DE8012CEFBF2B875FC41190260D403E40B2EE8BEB2855D0F4447679D9B9519BE64BE421166ABA2C66BEAAAF4EBAF8E162886430243216DDA10FCE68C07B6D7DDAA3E372478D711E3E1041C000000010010200A2FF0020DD2082014C97BA9730ED44D0D70B1FE0A4F260810200D71820D70B1FED44D0D31FD3FFD15112BAF2A122F901541044F910F2A2F80001D31F3120D74A96D307D402FB00DED1A4C8CB1FCBFFC9ED54004800000000C59DC52962CC568AC5E72735EABB025C5BDF457D029AEEA6C2FFA5EB2A945446BCF59C17
(Saved wallet creating query to file my_wallet_name-query.boc)
```
--------------------------------------------

简而言之，Fift 汇编器（通过 "Asm.fif" 包含行加载）用于将智能合约的源代码（包含 `<{ SETCP0 ... c4 POPCTR }> 行`）编译成其内部表示。智能合约的初始数据也会创建（通过 `<b 0 32 u, ... b>` 行），其中包含一个 32 位的序列号（等于零）和一个来自新生成的 Ed25519 密钥对的 256 位公钥。相应的私钥将保存到 `my_wallet_name.pk` 文件中，除非该文件已经存在（如果您在同一目录下运行此代码两次，则会从该文件中加载私钥）。

新智能合约的代码和数据将组合成一个 StateInit 结构（在接下来的行中），计算并输出新智能合约的地址（等于此 StateInit 结构的哈希值），然后创建一个外部消息，其目标地址等于新智能合约的地址。此外部消息包含新智能合约的正确 StateInit 以及一个非平凡的有效载荷（由正确的私钥签名）。

最后，外部消息被序列化为一个细胞包（表示为 B5EE...BE63），并保存到 `my_wallet_name-query.boc` 文件中。基本上，此文件是您的编译智能合约，包含了上传到 TON 区块链所需的所有附加信息。

4. 向新智能合约转移资金


您可以尝试立即上传新智能合约，运行 Lite Client 并输入：

> sendfile new-wallet-query.boc

or

> sendfile my_wallet_name-query.boc

如果您选择将钱包命名为 `my_wallet_name`。

不幸的是，这将不起作用，因为智能合约必须有一个正余额才能支付存储和处理其数据的费用。因此，您必须先将一些资金转移到您的新智能合约地址，该地址在生成过程中显示为 -1:60c0...c0d0（原始形式）和 0f9...EKD（用户友好形式）。

在实际场景中，您可以从已有的钱包转移一些 Grams，或让朋友帮忙，或者在加密货币交易所购买一些 Grams，并将 0f9...EKD 作为转账账户。

在测试网络中，您还有另一个选项：您可以请求“测试赠送者”给您一些测试 Grams（最多 20 个）。下面解释如何操作。

5. 使用测试赠送者智能合约


您需要知道测试赠送者智能合约的地址。我们假设它是 -1:fcb91a3a3816d0f7b8c2c76108b8a9bc5a6b7a55bd79f8ab101c52db29232260，或者等效的 kf_8uRo6OBbQ97jCx2EIuKm8Wmt6Vb15-KsQHFLbKSMiYIny，正如之前的示例所示。您可以通过在 Lite Client 中输入以下命令来检查此智能合约的状态：

```
> last
> getaccount kf_8uRo6OBbQ97jCx2EIuKm8Wmt6Vb15-KsQHFLbKSMiYIny
```


如第2节所述，您只需要从输出中获取存储在智能合约数据中的32位序列号（在上述示例中是0x9A15，但通常会有所不同）。获取此序列号当前值的更简单方法是输入以下命令：
```
> last
> runmethod kf_8uRo6OBbQ97jCx2EIuKm8Wmt6Vb15-KsQHFLbKSMiYIny seqno

producing the correct value 39445 = 0x9A15:
```
--------------------------------------------
```
got account state for -1 : FCB91A3A3816D0F7B8C2C76108B8A9BC5A6B7A55BD79F8AB101C52DB29232260 with respect to blocks (-1,8000000000000000,2240):18E6DA7707191E76C71EABBC5277650666B7E2CFA2AEF2CE607EAFE8657A3820:4EFA2540C5D1E4A1BA2B529EE0B65415DF46BFFBD27A8EB74C4C0E17770D03B1
creating VM
starting VM to run method `seqno` (85143) of smart contract -1:FCB91A3A3816D0F7B8C2C76108B8A9BC5A6B7A55BD79F8AB101C52DB29232260
...
arguments:  [ 85143 ] 
result:  [ 39445 ] 
```
--------------------------------------------

接下来，您需要创建一个外部消息，要求测试给予者向您的（尚未初始化的）智能合约发送指定数量的测试 Grams。用于生成此外部消息的特殊 Fift 脚本位于 `crypto/smartcont/testgiver.fif`：

--------------------------------------------
```
#!/usr/bin/env fift -s
"TonUtil.fif" include

{ ."usage: " @' $0 type ." <dest-addr> <seqno> <amount> [<savefile>]" cr
  ."Creates a request to TestGiver and saves it into <savefile>.boc" cr
  ."('testgiver-query.boc' by default)" cr 1 halt
} : usage

$# 3 - -2 and ' usage if

// "testgiver.addr" load-address 
Masterchain 0xfcb91a3a3816d0f7b8c2c76108b8a9bc5a6b7a55bd79f8ab101c52db29232260
2constant giver_addr
 ."Test giver address = " giver_addr 2dup .addr cr 6 .Addr cr

$1 true parse-load-address =: bounce 2=: dest_addr
$2 parse-int =: seqno
$3 $>GR =: amount
def? $4 { @' $4 } { "testgiver-query" } cond constant savefile

."Requesting " amount .GR ."to account "
dest_addr 2dup bounce 7 + .Addr ." = " .addr
."seqno=0x" seqno x. ."bounce=" bounce . cr

// create a message (NB: 01b00.., b = bounce)
<b b{01} s, bounce 1 i, b{000100} s, dest_addr addr, 
   amount Gram, 0 9 64 32 + + 1+ 1+ u, "GIFT" $, b>
<b seqno 32 u, 1 8 u, swap ref, b>
dup ."enveloping message: " <s csr. cr
<b b{1000100} s, giver_addr addr, 0 Gram, b{00} s,
   swap <s s, b>
dup ."resulting external message: " <s csr. cr
2 boc+>B dup Bx. cr
savefile +".boc" tuck B>file
."(Saved to file " type .")" cr
---------------------------------------------

You can pass the required parameters as command-line arguments to this script

$ crypto/fift -I<include-path> -s <path-to-testgiver-fif> <dest-addr> <testgiver-seqno> <gram-amount> [<savefile>]

For instance,

$ crypto/fift -I<source-directory>/crypto/fift/lib:<source-directory>/crypto/smartcont -s testgiver.fif 0QAu6bT9Twd8myIygMNXY9-e2rC0GsINNvQAlnfflcOv4uVb 0x9A15 6.666 wallet-query

or simply

$ fift -s testgiver.fif 0QAu6bT9Twd8myIygMNXY9-e2rC0GsINNvQAlnfflcOv4uVb 0x9A15 6.666 wallet-query
```

如果您已将环境变量 `FIFTPATH` 设置为 `<source-directory>/crypto/fift/lib:<source-directory>/crypto/smartcont`，并将 `fift` 二进制文件安装为 `/usr/bin/fift`（或在您的 PATH 中的其他位置）。

新创建的消息必须确保其回跳位未设置，否则转账将被“回跳”回发件人。这就是为什么我们使用了“非回跳”地址 `0QAu6bT9Twd8myIygMNXY9-e2rC0GsINNvQAlnfflcOv4uVb` 作为我们新钱包智能合约的地址。

这段 Fift 代码创建了一个从测试给予者智能合约到我们新智能合约的内部消息，携带 6.666 测试 Grams（您可以在这里输入其他数量，最多约 20 Grams）。然后，这个消息被封装到一个发给测试给予者的外部消息中；这个外部消息还必须包含测试给予者的正确序列号。当测试给予者收到这样的外部消息时，它会检查序列号是否与其持久数据中存储的序列号匹配，如果匹配，它会将嵌入的内部消息连同所需的测试 Grams 发送到目标地址（在这种情况下是我们的智能合约）。

外部消息被序列化并保存到文件 `wallet-query.boc` 中。在这个过程中会生成一些输出：

---------------------------------------------
```
Test giver address = -1:fcb91a3a3816d0f7b8c2c76108b8a9bc5a6b7a55bd79f8ab101c52db29232260 
kf_8uRo6OBbQ97jCx2EIuKm8Wmt6Vb15-KsQHFLbKSMiYIny
Requesting GR$6.666 to account 0QAu6bT9Twd8myIygMNXY9-e2rC0GsINNvQAlnfflcOv4uVb = 0:2ee9b4fd4f077c9b223280c35763df9edab0b41ac20d36f4009677df95c3afe2 seqno=0x9a15 bounce=0 
enveloping message: x{00009A1501}
 x{42001774DA7EA783BE4D91194061ABB1EFCF6D585A0D61069B7A004B3BEFCAE1D7F1280C6A98B4000000000000000000000000000047494654}

resulting external message: x{89FF02ACEEB6F264BCBAC5CE85B372D8616CA2B4B9A5E3EC98BB496327807E0E1C1A000004D0A80C_}
 x{42001774DA7EA783BE4D91194061ABB1EFCF6D585A0D61069B7A004B3BEFCAE1D7F1280C6A98B4000000000000000000000000000047494654}

B5EE9C7241040201000000006600014F89FF02ACEEB6F264BCBAC5CE85B372D8616CA2B4B9A5E3EC98BB496327807E0E1C1A000004D0A80C01007242001774DA7EA783BE4D91194061ABB1EFCF6D585A0D61069B7A004B3BEFCAE1D7F1280C6A98B4000000000000000000000000000047494654AFC17FA4
(Saved to file wallet-query.boc)
```
---------------------------------------------

6. 上传外部消息到测试给予者智能合约


现在我们可以调用 Lite Client，检查测试给予者的状态（如果序列号已更改，我们的外部消息将会失败），然后输入以下命令：
```
> sendfile wallet-query.boc

We will see some output:
... external message status is 1
```

这意味着外部消息已经被送到协作者池。随后，某个协作者可能会选择将此外部消息包含在一个区块中，为测试给予者智能合约创建一个处理该外部消息的交易。我们可以检查测试给予者的状态是否已经改变：
```
> last
> getaccount kf_8uRo6OBbQ97jCx2EIuKm8Wmt6Vb15-KsQHFLbKSMiYIny
```
（如果你忘记输入 `last` 命令，你很可能会看到测试给予者智能合约的状态没有变化。）结果输出将是：

---------------------------------------------
```
got account state for -1 : FCB91A3A3816D0F7B8C2C76108B8A9BC5A6B7A55BD79F8AB101C52DB29232260 with respect to blocks (-1,8000000000000000,2240):18E6DA7707191E76C71EABBC5277650666B7E2CFA2AEF2CE607EAFE8657A3820:4EFA2540C5D1E4A1BA2B529EE0B65415DF46BFFBD27A8EB74C4C0E17770D03B1
account state is (account
  addr:(addr_std
    anycast:nothing workchain_id:-1 address:xFCB91A3A3816D0F7B8C2C76108B8A9BC5A6B7A55BD79F8AB101C52DB29232260)
  storage_stat:(storage_info
    used:(storage_used
      cells:(var_uint len:1 value:3)
      bits:(var_uint len:2 value:707)
      public_cells:(var_uint len:0 value:0)) last_paid:0
    due_payment:nothing)
  storage:(account_storage last_trans_lt:10697000003
    balance:(currencies
      grams:(nanograms
        amount:(var_uint len:7 value:999993280210000))
      other:(extra_currencies
        dict:hme_empty))
    state:(account_active
      (
        split_depth:nothing
        special:nothing
        code:(just
          value:(raw@^Cell 
            x{}
             x{FF0020DDA4F260D31F01ED44D0D31FD166BAF2A1F80001D307D4D1821804A817C80073FB0201FB00A4C8CB1FC9ED54}
            ))
        data:(just
          value:(raw@^Cell 
            x{}
             x{00009A16}
            ))
        library:hme_empty))))
x{CFF8156775B79325E5D62E742D9B96C30B6515A5CD2F1F64C5DA4B193C03F070E0D2068086C00000000000000009F65D110DC0E35F450FA914134_}
 x{FF0020DDA4F260D31F01ED44D0D31FD166BAF2A1F80001D307D4D1821804A817C80073FB0201FB00A4C8CB1FC9ED54}
 x{00000001}
```

---------------------------------------------


你可能会注意到，持久数据中存储的序列号已发生变化（在我们的例子中，变为 0x9A16 = 39446），并且 `last_trans_lt` 字段（该账户的最后一笔交易的逻辑时间）也有所增加。

现在我们可以检查我们新智能合约的状态：
```
> getaccount 0QAu6bT9Twd8myIygMNXY9-e2rC0GsINNvQAlnfflcOv4uVb
or
> getaccount 0:2ee9b4fd4f077c9b223280c35763df9edab0b41ac20d36f4009677df95c3afe2
```

Now we see

---------------------------------------------
```
got account state for 0:2EE9B4FD4F077C9B223280C35763DF9EDAB0B41AC20D36F4009677DF95C3AFE2 with respect to blocks (-1,8000000000000000,16481):890F4D549428B2929F5D5E0C5719FBCDA60B308BA4B907797C9E846E644ADF26:22387176928F7BCEF654411CA820D858D57A10BBF1A0E153E1F77DE2EFB2A3FB and (-1,8000000000000000,16481):890F4D549428B2929F5D5E0C5719FBCDA60B308BA4B907797C9E846E644ADF26:22387176928F7BCEF654411CA820D858D57A10BBF1A0E153E1F77DE2EFB2A3FB
account state is (account
  addr:(addr_std
    anycast:nothing workchain_id:0 address:x2EE9B4FD4F077C9B223280C35763DF9EDAB0B41AC20D36F4009677DF95C3AFE2)
  storage_stat:(storage_info
    used:(storage_used
      cells:(var_uint len:1 value:1)
      bits:(var_uint len:1 value:111)
      public_cells:(var_uint len:0 value:0)) last_paid:1553210152
    due_payment:nothing)
  storage:(account_storage last_trans_lt:16413000004
    balance:(currencies
      grams:(nanograms
        amount:(var_uint len:5 value:6666000000))
      other:(extra_currencies
        dict:hme_empty))
    state:account_uninit))
x{CFF60C04141C6A7B96D68615E7A91D265AD0F3A9A922E9AE9C901D4FA83F5D3C0D02025BC2E4A0D9400000000F492A0511406354C5A004_}
```
---------------------------------------------

我们的新智能合约有一定的正余额（6.666 测试 Grams），但没有代码或数据（显示为 `state:account_uninit`）。

7. 上传新智能合约的代码和数据

Now you can finally upload the external message with the StateInit of the new smart contract, containing its code and data:

---------------------------------------------
```
> sendfile my_wallet_name-query.boc
... external message status is 1
> last
...
> getaccount 0QAu6bT9Twd8myIygMNXY9-e2rC0GsINNvQAlnfflcOv4uVb
...
got account state for 0:2EE9B4FD4F077C9B223280C35763DF9EDAB0B41AC20D36F4009677DF95C3AFE2 with respect to blocks (-1,8000000000000000,16709):D223B25D8D68401B4AA19893C00221CF9AB6B4E5BFECC75FD6048C27E001E0E2:4C184191CE996CF6F91F59CAD9B99B2FD5F3AA6F55B0B6135069AB432264358E and (-1,8000000000000000,16709):D223B25D8D68401B4AA19893C00221CF9AB6B4E5BFECC75FD6048C27E001E0E2:4C184191CE996CF6F91F59CAD9B99B2FD5F3AA6F55B0B6135069AB432264358E
account state is (account
  addr:(addr_std
    anycast:nothing workchain_id:0 address:x2EE9B4FD4F077C9B223280C35763DF9EDAB0B41AC20D36F4009677DF95C3AFE2)
  storage_stat:(storage_info
    used:(storage_used
      cells:(var_uint len:1 value:3)
      bits:(var_uint len:2 value:963)
      public_cells:(var_uint len:0 value:0)) last_paid:1553210725
    due_payment:nothing)
  storage:(account_storage last_trans_lt:16625000002
    balance:(currencies
      grams:(nanograms
        amount:(var_uint len:5 value:5983177000))
      other:(extra_currencies
        dict:hme_empty))
    state:(account_active
      (
        split_depth:nothing
        special:nothing
        code:(just
          value:(raw@^Cell 
            x{}
             x{FF0020DDA4F260810200D71820D70B1FED44D0D7091FD709FFD15112BAF2A122F901541044F910F2A2F80001D7091F3120D74A97D70907D402FB00DED1A4C8CB1FCBFFC9ED54}
            ))
        data:(just
          value:(raw@^Cell 
            x{}
             x{00000001F61CF0BC8E891AD7636E0CD35229D579323AA2DA827EB85D8071407464DC2FA3}
            ))
        library:hme_empty))))
x{CFF60C04141C6A7B96D68615E7A91D265AD0F3A9A922E9AE9C901D4FA83F5D3C0D020680F0C2E4A0EB280000000F7BB57909405928024A134_}
 x{FF0020DDA4F260810200D71820D70B1FED44D0D7091FD709FFD15112BAF2A122F901541044F910F2A2F80001D7091F3120D74A97D70907D402FB00DED1A4C8CB1FCBFFC9ED54}
 x{00000001F61CF0BC8E891AD7636E0CD35229D579323AA2DA827EB85D8071407464DC2FA3}
```

---------------------------------------------
您将看到智能合约已经使用外部消息中的 StateInit 的代码和数据进行了初始化，并且其余额因处理费用而有所减少。现在它已经上线运行，您可以通过生成新的外部消息并使用 Lite Client 的 "sendfile" 命令将其上传到 TON 区块链来激活它。

8. 使用简单钱包智能合约

实际上，示例中使用的简单钱包智能合约可以用来将测试 Grams 转账到任何其他账户。它在这方面类似于上面讨论的测试给予者智能合约，不同之处在于它只处理由正确私钥（即其所有者的私钥）签名的外部消息。在我们的情况下，这个私钥在智能合约编译时被保存到文件 "my_wallet_name.pk" 中（见第 3 节）。

关于如何使用这个智能合约的示例，见样本文件 `crypto/smartcont/wallet.fif`：

--------------------------------------------------------
```
#!/usr/bin/env fift -s
"TonUtil.fif" include

{ ."usage: " @' $0 type ." <filename-base> <dest-addr> <seqno> <amount> [-B <body-boc>] [<savefile>]" cr
  ."Creates a request to simple wallet created by new-wallet.fif, with private key loaded from file <filename-base>.pk "
  ."and address from <filename-base>.addr, and saves it into <savefile>.boc ('wallet-query.boc' by default)" cr 1 halt
} : usage
$# dup 4 < swap 5 > or ' usage if
def? $6 { @' $5 "-B" $= { @' $6 =: body-boc-file [forget] $6 def? $7 { @' $7 =: $5 [forget] $7 } { [forget] $5 } cond
  @' $# 2- =: $# } if } if

true constant bounce

$1 =: file-base
$2 bounce parse-load-address =: bounce 2=: dest_addr
$3 parse-int =: seqno
$4 $>GR =: amount
def? $5 { @' $5 } { "wallet-query" } cond constant savefile

file-base +".addr" load-address
2dup 2constant wallet_addr
."Source wallet address = " 2dup .addr cr 6 .Addr cr
file-base +".pk" load-keypair nip constant wallet_pk

def? body-boc-file { @' body-boc-file file>B B>boc } { <b "TEST" $, b> } cond
constant body-cell

."Transferring " amount .GR ."to account "
dest_addr 2dup bounce 7 + .Addr ." = " .addr 
."seqno=0x" seqno x. ."bounce=" bounce . cr
."Body of transfer message is " body-cell <s csr. cr
  
// create a message
<b b{01} s, bounce 1 i, b{000100} s, dest_addr addr, amount Gram, 0 9 64 32 + + 1+ u, 
  body-cell <s 2dup s-fits? not rot over 1 i, -rot { drop body-cell ref, } { s, } cond
b>
<b seqno 32 u, 1 8 u, swap ref, b>
dup ."signing message: " <s csr. cr
dup hash wallet_pk ed25519_sign_uint
<b b{1000100} s, wallet_addr addr, 0 Gram, b{00} s,
   swap B, swap <s s, b>
dup ."resulting external message: " <s csr. cr
2 boc+>B dup Bx. cr
savefile +".boc" tuck B>file
."(Saved to file " type .")" cr
```
-------------------------------------

You can invoke this script as follows:

$ fift -I<source-directory>/crypto/fift/lib:<source-directory>/crypto/smartcont -s wallet.fif <your-wallet-id> <destination-addr> <your-wallet-seqno> <gram-amount>

or simply

$ fift -s wallet.fif <your-wallet-id> <destination-addr> <your-wallet-seqno> <gram-amount>

如果您已经正确设置了 PATH 和 FIFTPATH。

例如，

```
$ fift -s wallet.fif my_wallet_name kf8Ty2EqAKfAksff0upF1gOptUWRukyI9x5wfgCbh58Pss9j 1 .666
```

这里 `my_wallet_name` 是您之前在 `new-wallet.fif` 中使用的钱包标识符；测试钱包的地址和私钥将从当前目录中的 `my_wallet_name.addr` 和 `my_wallet_name.pk` 文件中加载。

运行这段代码（通过调用 Fift 解释器），您将创建一个外部消息，目的地是您钱包智能合约的地址，包含正确的 Ed25519 签名、一个序列号以及一个封装的内部消息，从您的钱包智能合约到指定的 `dest_addr` 智能合约，附带一个任意值和任意有效负载。当您的智能合约接收到并处理这个外部消息时，它首先检查签名和序列号。如果它们是正确的，它将接受外部消息，发送嵌入的内部消息到目标地址，并增加其持久数据中的序列号（这是一个简单的防止重放攻击的措施，以防这个示例钱包智能合约代码最终被用于真实的钱包应用）。

当然，真正的 TON 区块链钱包应用程序会隐藏上述所有中间步骤。它会首先将新智能合约的地址传达给用户，要求他们从另一个钱包或加密货币交易所将一些资金转移到该地址（以其非回退用户友好形式显示），然后提供一个简单的界面以显示当前余额，并将资金转移到用户希望的其他地址。（本文档的目的是解释如何创建新的非平凡智能合约并在 TON 区块链测试网络上进行实验，而不是解释如何使用 Lite Client 替代更用户友好的钱包应用程序。）

最后一点说明：以上示例使用了基本工作链（workchain 0）的智能合约。如果将 `new-wallet.fif` 的第一个参数设置为 -1（表示主链）而不是 0，这些智能合约在主链中也会以完全相同的方式工作。唯一的区别是基本工作链中的处理和存储费用比主链低 100-1000 倍。一些智能合约（如验证者选举智能合约）只接受来自主链智能合约的转账，因此如果您希望代表自己的验证者进行质押并参与选举，您将需要在主链上拥有一个钱包。