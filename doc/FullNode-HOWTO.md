最方便的编译和运行节点的方法是使用 MyTonCtrl: [MyTonCtrl](https://github.com/ton-blockchain/mytonctrl/)。更多信息可以在 [TON Docs](https://ton.org/docs/#/nodes/run-node) 上找到。

本文档旨在提供设置 TON 区块链全节点的逐步指南。我们假设读者对 TON 区块链 Lite 客户端有一定的了解，至少了解 Lite 客户端 HOWTO 中解释的内容。

请注意，运行 TON 区块链全节点需要一台具有公共 IP 地址和高带宽网络连接的机器。通常，您需要一台在数据中心的足够强大的服务器，拥有至少 1 Gbit/s 的连接，以可靠地处理峰值负载（平均负载预计约为 100 Mbit/s）。我们推荐使用一台双处理器的服务器，每个处理器至少有八个核心，至少 256 MiB 的 RAM，至少 8 TB 的常规 HDD 存储和至少 512 GB 的更快 SSD 存储。将全节点运行在家庭计算机上并不理想；相反，您可以在远程服务器上运行全节点，并使用 TON 区块链 Lite 客户端从家里连接。

### 0. 下载和编译

TON 区块链库和验证器软件的下载和编译方式类似于 Lite 客户端。此过程在相应的 README 文件中有详细说明。最重要的区别是，您需要从公共 GitHub 仓库 [https://github.com/ton-blockchain/ton](https://github.com/ton-blockchain/ton) 下载完整的源代码（例如，通过运行 `git clone https://github.com/ton-blockchain/ton` 然后执行 `git submodule update`），而不是下载较小的 Lite 客户端源代码包。您还应该构建 CMakeLists.txt 中定义的所有目标（例如，通过在构建目录中运行 `cmake <path-to-source-directory>` 和 `make`），而不仅仅是那些专门与 Lite 客户端相关的目标（Lite 客户端也包含在较大的发行版中；您无需单独下载和构建它）。我们强烈推荐通过在第一次运行 `cmake` 时传递 `-DCMAKE_BUILD_TYPE=Release` 或 `-DCMAKE_BUILD_TYPE=RelWithDebInfo` 作为额外参数来构建“发布”或“带有调试信息的发布”版本（如果忘记了这样做，您可以在构建目录中删除 `CMakeCache.txt` 文件，然后重新运行 `cmake` 以使用适当的选项）。

### 1. 全节点二进制文件

在成功编译源代码后，您应该在构建目录中获得可执行文件 `validator-engine/validator-engine` 和 `validator-engine-console/validator-engine-console`。这些是运行 TON 区块链全节点（甚至验证器）和控制它所需的最重要的文件。您可能希望将它们安装到 /usr/bin 或类似目录。您还可能需要在设置过程中使用 `generate-random-id` 工具。

### 2. 全节点的工作目录

全节点（也称为“验证器引擎”）将其数据存储在工作目录的子目录中，例如 `/var/ton-work/db`。它需要对该目录的写权限。如果您想使用其他目录作为全节点的工作目录，可以使用命令行选项 `--db <path-to-work-dir>`：

```bash
$ validator-engine --db ${DB_ROOT}
```

其中 ${DB_ROOT} 是 /var/ton-work/db 或任何其他 validator-engine 具有写权限的目录。

### 3. 工作目录布局

TON 区块链全节点软件的工作目录大致布局如下：

* `${DB_ROOT}/config.json` — 自动生成的配置文件。它在某些情况下由 validator-engine 自动重新生成。当 validator-engine 未运行时，您可以在文本编辑器中编辑此文件，因为它是一个 JSON 文件。
* `${DB_ROOT}/static` — 目录中包含无法从网络下载的文件，如主链和活动工作链的“零状态”（对应于其他区块链架构的创世区块）。通常，除非您想运行自己实例的 TON 区块链（例如用于测试或开发目的），否则不必初始化此目录。现有 TON 区块链实例（如“testnet”和“mainnet”）的全节点将能够从已运行的全节点中下载所有所需文件。
* `${DB_ROOT}/keyring` — 存储 validator-engine 已知的公钥和私钥。例如，如果您的全节点作为某些 TON 区块链分片链的验证器运行，则验证器区块签名密钥存储在这里。您可能希望为此目录设置更严格的权限，例如 0700（在 *nix 系统中），以便只有运行 validator-engine 的用户才能访问此目录。
* `${DB_ROOT}/error` — 目录，validator-engine 复制与严重错误条件（例如无效的区块候选）相关的文件以供进一步研究。它通常是空的，validator-engine 不会删除此目录中的文件。
* `${DB_ROOT}/archive` — 目录，存放旧的和不常用的区块，直到其存储期到期。您可以在此目录中挂载更大但较慢的磁盘分区，或将此目录设置为指向此类分区中的目录的符号链接。我们建议将 ${DB_ROOT} 的其余部分放在像 SSD 这样的快速存储设备中。
* `${DB_ROOT}/etc` — （非自动）配置文件可以放在这里，或放在任何其他 validator-engine 可读的目录中。
* `${DB_ROOT}` 的其他子目录用于存储 ADNL 缓存数据、最近的区块和状态等。这些对本文档的目的不太相关。

### 4. TON 区块链的全局配置

要设置您的全节点，您需要一个称为“全局配置文件”的特殊 JSON 文件。之所以称为全局配置文件，是因为它对于所有全节点都是相同的，即使是参与不同 TON 区块链实例的节点（例如，“testnet”与“mainnet”）也共享几乎相同的全局配置。

“mainnet”全局配置可以在 [https://ton.org/global-config.json](https://ton.org/global-config.json) 下载，如下所示：

```bash
$ wget https://ton.org/global-config.json
```

您可能希望将此文件放入 /var/ton-work/etc/global-config.json。

我们稍后会更详细地讨论此文件的结构。现在，我们需要注意的是，此文件的主体部分由已知的 TON DHT 节点列表组成，这些节点对于 TON 网络的引导是必需的。文件末尾的一个小部分描述了我们希望连接的特定 TON 区块链实例。

所有 TON 区块链实例使用相同的“全球”TON 网络（即，TON 网络不会被碎片化成多个实例）。因此，全球网络配置与选择的特定 TON 区块链实例无关，但属于不同实例的全节点稍后将连接到 TON 网络内部的不同覆盖子网。

重要的是要区分用于设置 TON 区块链全节点的“全局配置文件”和由 validator-engine 自动更新的“本地”或“自动配置文件”，后者通常存储在 `${DB_ROOT}/config.json` 中。全局配置用于生成初始的自动配置文件，之后由 validator-engine 自行更新（例如，添加新的 DHT 节点或存储更新的主链区块的哈希）。

### 5. 初始化本地配置

下载全局配置文件后，可以用来创建 `${DB_ROOT}/config.json` 中的初始本地配置。为此，执行一次 validator-engine：

```bash
$ validator-engine -C /var/ton-work/etc/global-config.json --db /var/ton-work/db/ --ip <IP>:<PORT> -l /var/ton-work/log
```

其中 `/var/ton-work/log` 是 `validator-engine` 的日志目录，它将在其中创建日志文件。`-C` 命令行选项的参数是从 ton.org 下载的全局配置文件，`/var/ton-work/db/` 是工作目录 `${DB_ROOT}`。最后，`<IP>:<PORT>` 是此全节点的全球 IP 地址（您需要在此处指明公共 IPv4 地址）和用于运行 TON 网络协议（如 ADNL 和 RLDP）的 UDP 端口。确保您的防火墙已配置为传递源或目标为 `<IP>:<PORT>` 的 UDP 数据包，至少用于 `validator-engine` 二进制文件。

当按上述方式调用 validator-engine 时，如果 `${DB_ROOT}/config.json` 不存在，它将使用来自全局配置文件和命令行选项（如 `--ip`）的信息创建新的本地配置文件 `${DB_ROOT}/config.json`，然后退出。如果 `${DB_ROOT}/config.json` 已存在，则不会被重写；相反，validator-engine 将使用本地配置和全局配置启动为守护进程。

如果之后需要更改本地配置，您需要删除此文件并从全局配置重新生成它（可能会丢失在本地配置中

积累的重要信息），或者在文本编辑器中编辑本地配置（当 validator-engine 未运行时）。

### 6. 设置远程控制 CLI

您几乎肯定希望在本地配置中启用 validator-engine-console，以便在运行时控制全节点（即，validator-engine 守护进程）。为此，您需要生成两个密钥对，一个用于服务器（validator-engine），一个用于客户端（validator-engine-console）。以下示例假设 validator-engine-console 运行在同一台机器上，并通过回环网络接口连接到 validator-engine。（这并非必须；您也可以将 validator-engine-console 用于远程控制。）

首先，使用 `generate-random-id` 可执行文件创建两个密钥对，一个用于服务器（在运行 `validator-engine` 的机器上），一个用于客户端（在运行 `validator-engine-console` 的机器上）：

```bash
$ ./generate-random-id -m keys -n server
6E9FD109F76E08B5710445C72D2C5FEDE04A96357DAA4EC0DDAEA025ED3AC3F7 bp/RCfduCLVxBEXHLSxf7eBKljV9qk7A3a6gJe06w/c=
```

该工具生成一个新的密钥对，并将私钥保存到文件 `server` 中，将公钥保存到 `server.pub` 中。公钥的十六进制（6E9F...F7）和 base64（"bp/RC...6wc/="）表示会在标准输出中显示，并且将用于标识此密钥。

我们必须将私钥 `server` 安装到全节点的密钥环中（validator-engine）：

```bash
$ mv server /var/ton-work/db/keyring/6E9FD109F76E08B5710445C72D2C5FEDE04A96357DAA4EC0DDAEA025ED3AC3F7
```

请注意，存储此私钥的文件名与密钥环中存储的公钥的十六进制标识符相同（本质上是公钥的哈希）。

接下来，生成客户端密钥对：

```bash
$ ./generate-random-id -m keys -n client
8BBA4F8FCD7CC4EF573B9FF48DC63B212A8E9292B81FC0359B5DBB8719C44656 i7pPj818xO9XO5/0jcY7ISqOkpK4H8A1m127hxnERlY=
```

我们获得一个客户端密钥对，保存到文件 `client` 和 `client.pub` 中。此第二个操作应在客户端（validator-engine-console）的工作目录中运行，可能是在另一台机器上。

现在我们必须将客户端的公钥列入服务器的本地配置文件 `${DB_ROOT}/config.json`。为此，在文本编辑器中打开本地配置文件（在 validator-engine 运行时终止后），找到空的“control”部分：

```json
"control": [
]
```

将其替换为：

```json
"control" : [
  { "id" : "bp/RCfduCLVxBEXHLSxf7eBKljV9qk7A3a6gJe06w/c=",
    "port" : <CONSOLE-PORT>,
    "allowed" : [
      { "id" : "i7pPj818xO9XO5/0jcY7ISqOkpK4H8A1m127hxnERlY=",
        "permissions" : 15
      }
    ]
  }
],
```

`control.0.id` 设置为服务器公钥的 base64 标识符，`control.0.allowed.0.id` 是客户端公钥的 base64 标识符。`<CONSOLE-PORT>` 是服务器将监听的控制台命令的 TCP 端口。

### 7. 运行全节点

要运行全节点，只需在控制台中运行 validator-engine 二进制文件：

```bash
$ validator-engine --db ${DB_ROOT} -C /var/ton-work/etc/global-config.json  -l /var/ton-work/log
```

它将从 /var/ton-work/etc/global-config.json 读取全局配置，从 `${DB_ROOT}/config.json` 读取本地配置，并继续静默运行。您应该编写合适的脚本将 validator-engine 作为守护进程启动（以便在关闭控制台时它不会终止），但为了简单起见，我们将跳过这些考虑。（在大多数 *nix 系统上，`validator-engine` 的 `-d` 命令行选项应该足够。）

如果配置无效，validator-engine 将立即终止，并且在大多数情况下不会输出任何内容。您需要检查 /var/ton-work/log 下的日志文件以找出问题所在。否则，validator-engine 将保持静默运行。再次，您可以通过检查日志文件以及检查 `${DB_ROOT}` 目录的子目录来了解发生了什么。

如果一切正常，validator-engine 将找到参与相同 TON 区块链实例的其他全节点，并下载主链和所有分片链的最新区块。（您实际上可以控制下载的最新区块数量，甚至从零状态开始下载所有区块——但这个话题超出了本文档的范围；尝试使用 `-h` 命令行选项运行 validator-engine，以找到所有可用选项的简要说明）。

### 8. 使用控制台 CLI

如果按照第 6 节的说明设置了 validator-engine-console，您可以使用它连接到运行中的 validator-engine（即，全节点）并运行简单的状态和密钥管理查询：

```bash
$ ./validator-engine-console -k client -p server.pub -a <IP>:<CLIENT-PORT>
```

连接到 [<IP>:<CLIENT-PORT>]
本地密钥: 8BBA4F8FCD7CC4EF573B9FF48DC63B212A8E9292B81FC0359B5DBB8719C44656
远程密钥: 6E9FD109F76E08B5710445C72D2C5FEDE04A96357DAA4EC0DDAEA025ED3AC3F7
连接准备好
> gettime
接收到验证器时间: time=1566568904

`gettime` 命令获取验证器的当前 Unix 时间。如果一切配置正确，您将看到类似于上面的输出。请注意，控制台工作正常需要客户端的私钥（“client”）和服务器的公钥（“server.pub”）。您可能希望将它们（特别是客户端的私钥）移动到具有适当权限的单独目录中。

validator-engine-console 中还有其他控制台命令。例如，`help` 显示所有控制台命令的列表和简短描述。特别地，它们用于将全节点设置为 TON 区块链的验证器，如单独的文档 Validator-HOWTO 中所述。

### 9. 将全节点设置为 Lite 服务器

您可以将全节点设置为 Lite 服务器，以便从同一台或远程主机使用 Lite 客户端连接到它。例如，在连接到全节点的 Lite 客户端中发送命令 `last` 将显示全节点已知的最新主链区块的标识符，
以便您检查区块下载的进度。您还可以检查所有智能合约的状态，发送外部消息（例如钱包查询）等，正如 Lite 客户端 HOWTO 中所解释的那样。

为了将全节点设置为 Lite 服务器，您需要生成另一个密钥对并将私钥安装到服务器的密钥环中，类似于我们启用远程控制所做的操作：

```bash
$ utils/generate-random-id -m keys -n liteserver
BDFEA84525ADB3B16D0192488837C04883C10FF1F4031BB6DEECDD17544F5347 vf6oRSWts7FtAZJIiDfASIPBD/H0Axu23uzdF1RPU0c=
mv liteserver ${DB_ROOT}/keyring/BDFEA84525ADB3B16D0192488837C04883C10FF1F4031BB6DEECDD17544F5347
```

之后，停止正在运行的 validator-engine 并打开本地配置文件 `${TON_DB}/config.json` 在文本编辑器中。找到空的部分：

```json
"liteservers" : [
]
```

将其替换为包含用于监听传入 Lite 客户端连接的 TCP 端口和 Lite 服务器公钥的记录：

```json
"liteservers" : [
{
"id" : "vf6oRSWts7FtAZJIiDfASIPBD/H0Axu23uzdF1RPU0c=",
"port" : <TCP-PORT>
}
],
```

现在重新启动 `validator-engine`。如果它没有立即终止，很可能您已经正确配置了它。现在，您可以使用 lite-client 二进制文件（通常位于构建目录的 "lite-client/lite-client"）连接到

作为全节点一部分运行的 Lite 服务器：

```bash
$ lite-client -a <IP>:<TCP-PORT> -p liteserver.pub
```

再次，`help` 列出 Lite 客户端中的所有命令。Lite 客户端 HOWTO 包含一些有关使用 Lite 客户端可以做什么的示例。