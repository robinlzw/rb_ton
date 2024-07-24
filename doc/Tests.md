# 测试执行

TON 包含多个单元测试，这些测试可以在每次提交时帮助检测区块链的错误行为。

## 构建测试

进入构建目录，如果使用 ninja，使用以下命令构建测试：

```ninja test-ed25519 test-ed25519-crypto test-bigint test-vm test-fift test-cells test-smartcont test-net test-tdactor test-tdutils test-tonlib-offline test-adnl test-dht test-rldp test-rldp2 test-catchain test-fec test-tddb test-db test-validator-session-state```

有关如何构建 TON 工件的更多细节，请参阅任何 GitHub Actions 文档。

如果使用 cmake，请使用：

```cmake --build . --target test-ed25519 test-ed25519-crypto test-bigint test-vm test-fift test-cells test-smartcont test-net test-tdactor test-tdutils test-tonlib-offline test-adnl test-dht test-rldp test-rldp2 test-catchain test-fec test-tddb test-db test-validator-session-state```

## 运行测试

进入构建目录，并使用 ninja 执行：

```ninja test```

使用 ctest 执行：

```ctest```

## 将测试集成到 CI 中

最相关的 GitHub Actions 包括步骤 ```Run tests```，该步骤会执行测试。如果任何测试失败，操作将中断，并且不会提供工件。
