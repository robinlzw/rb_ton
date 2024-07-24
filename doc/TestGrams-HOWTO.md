这段文字的目的是描述如何快速获得少量测试 Grams（用于测试目的），或获得大量测试 Grams（用于在测试网络中运行验证节点）。我们假设读者对《LiteClient-HOWTO》中解释的 TON 区块链 LiteClient 有一定了解，并熟悉编译 LiteClient 和其他软件所需的程序。对于获得运行验证节点所需的大量测试 Grams，我们还假设读者了解《FullNode-HOWTO》和《Validator-HOWTO》。此外，您还需要一台足够强大的专用服务器来运行 Full Node，以便获得大量测试 Grams。而获得少量测试 Grams 不需要专用服务器，并可以在家用计算机上在几分钟内完成。

1. 工作量证明 TestGiver 智能合约


为了防止少数恶意方收集所有为测试目的预留的测试 Grams，测试网络的主链上部署了一种特殊的“工作量证明 TestGiver”智能合约。这些智能合约的地址如下：

小型 TestGiver（每隔几分钟提供 10 到 100 个测试 Grams）：
```
kf-kkdY_B7p-77TLn2hUhM6QidWrrsl8FYWCIvBMpZKprBtN
kf8SYc83pm5JkGt0p3TQRkuiM58O9Cr3waUtR9OoFq716lN-
kf-FV4QTxLl-7Ct3E6MqOtMt-RGXMxi27g4I645lw6MTWraV
kf_NSzfDJI1A3rOM0GQm7xsoUXHTgmdhN5-OrGD8uwL2JMvQ
kf8gf1PQy4u2kURl-Gz4LbS29eaN4sVdrVQkPO-JL80VhOe6
kf8kO6K6Qh6YM4ddjRYYlvVAK7IgyW8Zet-4ZvNrVsmQ4EOF
kf-P_TOdwcCh0AXHhBpICDMxStxHenWdLCDLNH5QcNpwMHJ8
kf91o4NNTryJ-Cw3sDGt9OTiafmETdVFUMvylQdFPoOxIsLm
kf9iWhwk9GwAXjtwKG-vN7rmXT3hLIT23RBY6KhVaynRrIK7
kf8JfFUEJhhpRW80_jqD7zzQteH6EBHOzxiOhygRhBdt4z2N
```
大型测试人员（每天至少一次提供10000克测试）：
```
kf8guqdIbY6kpMykR8WFeVGbZcP2iuBagXfnQuq0rGrxgE04
kf9CxReRyaGj0vpSH0gRZkOAitm_yDHvgiMGtmvG-ZTirrMC
kf-WXA4CX4lqyVlN4qItlQSWPFIy00NvO2BAydgC4CTeIUme
kf8yF4oXfIj7BZgkqXM6VsmDEgCqWVSKECO1pC0LXWl399Vx
kf9nNY69S3_heBBSUtpHRhIzjjqY0ChugeqbWcQGtGj-gQxO
kf_wUXx-l1Ehw0kfQRgFtWKO07B6WhSqcUQZNyh4Jmj8R4zL
kf_6keW5RniwNQYeq3DNWGcohKOwI85p-V2MsPk4v23tyO3I
kf_NSPpF4ZQ7mrPylwk-8XQQ1qFD5evLnx5_oZVNywzOjSfh
kf-uNWj4JmTJefr7IfjBSYQhFbd3JqtQ6cxuNIsJqDQ8SiEA
kf8mO4l6ZB_eaMn1OqjLRrrkiBcSt7kYTvJC_dzJLdpEDKxn
```
前十个智能合约使测试者可以在不消耗太多计算资源的情况下获得少量测试 Grams（通常，家用计算机的几分钟工作时间就足够了）。
其余的智能合约则用于获得运行测试网络验证节点所需的大量测试 Grams；通常，一个强大的专用服务器工作一天应该足够获得所需的数量。

您应该从这两个列表中随机选择一个“工作量证明 TestGiver”智能合约（根据您的目的选择一个），并通过类似于“挖矿”的过程从这个智能合约中获得测试 Grams。
本质上，您需要向选择的“工作量证明 TestGiver”智能合约提交一个包含工作量证明和您的钱包地址的外部消息，然后必要的数量将被发送给您。

2. “挖矿”过程


为了创建一个包含“工作量证明”的外部消息，您需要运行一个特别的“挖矿”工具，该工具是从 GitHub 仓库中的 TON 源代码编译而来的。
该工具位于构建目录中的文件 `./crypto/pow-miner`，可以通过在构建目录中输入 `make pow-miner` 进行编译。

然而，在运行“pow-miner”之前，您需要知道选择的“工作量证明 TestGiver”智能合约的实际“种子”（seed）和“复杂度”（complexity）参数。
这可以通过调用该智能合约的 get-method “get_pow_params” 来完成。
例如，如果您使用的 TestGiver 智能合约是 kf-kkdY_B7p-77TLn2hUhM6QidWrrsl8FYWCIvBMpZKprBtN，您可以直接输入
```
    > runmethod kf-kkdY_B7p-77TLn2hUhM6QidWrrsl8FYWCIvBMpZKprBtN get_pow_params
```
in the LiteClient console and obtain output like
```
    ...
    arguments:  [ 101616 ] 
    result:  [ 229760179690128740373110445116482216837 53919893334301279589334030174039261347274288845081144962207220498432 100000000000 256 ] 
    remote result (not to be trusted):  [ 229760179690128740373110445116482216837 53919893334301279589334030174039261347274288845081144962207220498432 100000000000 256 ] 
```
“result:” 行中的两个大数字分别是该智能合约的“种子”（seed）和“复杂度”（complexity）。
在这个例子中，种子是 229760179690128740373110445116482216837，复杂度是 53919893334301279589334030174039261347274288845081144962207220498432。

接下来，您需要按如下方式调用 pow-miner 工具：
```
      $ crypto/pow-miner -vv -w<num-threads> -t<timeout-in-sec> <your-wallet-address> <seed> <complexity> <iterations> <pow-testgiver-address> <boc-filename>
```
Here <num-threads> is the number of CPU cores that you want to use for mining, <timeout-in-sec> is the maximal amount of seconds that the miner would run before admitting failure, <your-wallet-address> is the address of your wallet (possibly not initialized yet), either in the masterchain or in the workchain (note that you need a masterchain wallet to control a validator), <seed> and <complexity> are the most recent values obtained by running get-method 'get-pow-params', <pow-testgiver-address> is the address of the chosen proof-of-work testgiver smartcontract, and <boc-filename> is the filename of the output file where the external message with the proof of work will be saved in the case of success.

For example, if your wallet address is kQBWkNKqzCAwA9vjMwRmg7aY75Rf8lByPA9zKXoqGkHi8SM7, you might run
```
    $ crypto/pow-miner -vv -w7 -t100 kQBWkNKqzCAwA9vjMwRmg7aY75Rf8lByPA9zKXoqGkHi8SM7 229760179690128740373110445116482216837 53919893334301279589334030174039261347274288845081144962207220498432 100000000000 kf-kkdY_B7p-77TLn2hUhM6QidWrrsl8FYWCIvBMpZKprBtN mined.boc
```

该程序将运行一段时间（在此情况下最多 100 秒），并且要么成功终止（退出代码为零）并将所需的工作证明保存到文件 "mined.boc" 中，要么以非零退出代码终止，如果未找到工作证明。

如果失败，您将看到类似以下内容的输出：

```
   [ expected required hashes for success: 2147483648 ]
   [ hashes computed: 1192230912 ]
```
程序将以非零退出代码终止。然后，您需要重新获取“seed”和“complexity”（因为它们可能已经在处理来自更成功矿工的请求的过程中发生了变化），并使用新的参数重新运行“pow-miner”，重复这个过程直到成功。

如果成功，您将看到类似以下内容的输出：
```
   [ expected required hashes for success: 2147483648 ]
   4D696E65005EFE49705690D2AACC203003DBE333046683B698EF945FF250723C0F73297A2A1A41E2F1A1F533B3BC4F5664D6C743C1C5C74BB3342F3A7314364B3D0DA698E6C80C1EA4ACDA33755876665780BAE9BE8A4D6385A1F533B3BC4F5664D6C743C1C5C74BB3342F3A7314364B3D0DA698E6C80C1EA4
   Saving 176 bytes of serialized external message into file `mined.boc`
   [ hashes computed: 1122036095 ]
```

然后，您可以使用 LiteClient 从文件 "mined.boc" 发送外部消息到 proof-of-work testgiver 智能合约（并且您必须尽快执行这个操作）：

```
> sendfile mined.boc
...	external message status is 1
```

您可以等待几秒钟，然后检查您的钱包状态：

```
> last
> getaccount kQBWkNKqzCAwA9vjMwRmg7aY75Rf8lByPA9zKXoqGkHi8SM7
...
account state is (account
  addr:(addr_std
    anycast:nothing workchain_id:0 address:x5690D2AACC203003DBE333046683B698EF945FF250723C0F73297A2A1A41E2F1)
  storage_stat:(storage_info
    used:(storage_used
      cells:(var_uint len:1 value:1)
      bits:(var_uint len:1 value:111)
      public_cells:(var_uint len:0 value:0)) last_paid:1593722498
    due_payment:nothing)
  storage:(account_storage last_trans_lt:7720869000002
    balance:(currencies
      grams:(nanograms
        amount:(var_uint len:5 value:100000000000))
      other:(extra_currencies
        dict:hme_empty))
    state:account_uninit))
x{C005690D2AACC203003DBE333046683B698EF945FF250723C0F73297A2A1A41E2F12025BC2F7F2341000001C169E9DCD0945D21DBA0004_}
last transaction lt = 7720869000001 hash = 83C15CDED025970FEF7521206E82D2396B462AADB962C7E1F4283D88A0FAB7D4
account balance is 100000000000ng
```
如果在您之前没有人用这个 *seed* 和 *complexity* 发送有效的工作证明，那么工作证明测试合约会接受您的工作证明，这将反映在您钱包的余额中（发送外部消息后，可能需要等待10到20秒才会发生此变化；确保多次尝试，并在每次检查钱包余额之前键入 "last" 以刷新 LiteClient 状态）。如果成功，您将看到余额增加（如果您的钱包之前不存在，您可能会看到钱包已创建为未初始化状态）。如果失败，您需要重新获取新的 "seed" 和 "complexity"，并从头开始重复挖矿过程。

如果您运气好，钱包余额增加了，您可能需要初始化钱包（有关钱包创建的更多信息，请参见 LiteClient-HOWTO）：

```
> sendfile new-wallet-query.boc
...	external message status is 1
> last
> getaccount kQBWkNKqzCAwA9vjMwRmg7aY75Rf8lByPA9zKXoqGkHi8SM7
...
account state is (account
  addr:(addr_std
    anycast:nothing workchain_id:0 address:x5690D2AACC203003DBE333046683B698EF945FF250723C0F73297A2A1A41E2F1)
  storage_stat:(storage_info
    used:(storage_used
      cells:(var_uint len:1 value:3)
      bits:(var_uint len:2 value:1147)
      public_cells:(var_uint len:0 value:0)) last_paid:1593722691
    due_payment:nothing)
  storage:(account_storage last_trans_lt:7720945000002
    balance:(currencies
      grams:(nanograms
        amount:(var_uint len:5 value:99995640998))
      other:(extra_currencies
        dict:hme_empty))
    state:(account_active
      (
        split_depth:nothing
        special:nothing
        code:(just
          value:(raw@^Cell 
            x{}
             x{FF0020DD2082014C97BA218201339CBAB19C71B0ED44D0D31FD70BFFE304E0A4F260810200D71820D70B1FED44D0D31FD3FFD15112BAF2A122F901541044F910F2A2F80001D31F3120D74A96D307D402FB00DED1A4C8CB1FCBFFC9ED54}
            ))
        data:(just
          value:(raw@^Cell 
            x{}
             x{00000001CE6A50A6E9467C32671667F8C00C5086FC8D62E5645652BED7A80DF634487715}
            ))
        library:hme_empty))))
x{C005690D2AACC203003DBE333046683B698EF945FF250723C0F73297A2A1A41E2F1206811EC2F7F23A1800001C16B0BC790945D20D1929934_}
 x{FF0020DD2082014C97BA218201339CBAB19C71B0ED44D0D31FD70BFFE304E0A4F260810200D71820D70B1FED44D0D31FD3FFD15112BAF2A122F901541044F910F2A2F80001D31F3120D74A96D307D402FB00DED1A4C8CB1FCBFFC9ED54}
 x{00000001CE6A50A6E9467C32671667F8C00C5086FC8D62E5645652BED7A80DF634487715}
last transaction lt = 7720945000001 hash = 73353151859661AB0202EA5D92FF409747F201D10F1E52BD0CBB93E1201676BF
account balance is 99995640998ng
```

现在，您是拥有100个测试Grams的幸运拥有者，可以用于您计划中的任何测试目的。祝贺您！

3. 在失败的情况下自动化挖矿过程

如果您长时间无法获得测试Grams，这可能是因为太多其他测试者同时从同一个工作证明测试合约中“挖矿”。也许您应该选择另一份从上述列表中选择的工作证明测试合约。
或者，您可以编写一个简单的脚本，自动重复运行 `pow-miner` 并使用正确的参数，直到成功（通过检查 `pow-miner` 的退出代码来检测），
并在找到工作证明后立即使用 `lite-client` 执行带有参数 -c 'sendfile mined.boc' 的命令来发送外部消息。
