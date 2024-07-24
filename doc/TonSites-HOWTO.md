# 介绍 TON Sites

本文档的目的是提供一个关于 TON Sites 的简要介绍，这些站点是通过 TON 网络访问的 (TON) 网站。TON Sites 可以作为其他 TON 服务的便捷入口。特别是，从 TON Sites 下载的 HTML 页面可能包含指向 ton://... URIs 的链接，这些链接代表用户可以通过点击链接来完成的支付，只要用户的设备上安装了 TON 钱包即可。

从技术角度来看，TON Sites 与普通网站非常相似，但它们是通过 TON 网络（这是一个在互联网内部的覆盖网络）而不是互联网进行访问的。更具体地说，它们具有 ADNL 地址（而不是更常见的 IPv4 或 IPv6 地址），并且通过 RLDP 协议（这是基于 ADNL 的高级 RPC 协议，ADNL 是 TON 网络的主要协议）而不是常规的 TCP/IP 协议接受 HTTP 查询。所有的加密都由 ADNL 处理，因此不需要使用 HTTPS（即 TLS）。

为了访问现有的和创建新的 TON Sites，需要在“普通”互联网和 TON 网络之间使用特殊的网关。实际上，TON Sites 是通过在客户端机器上运行的 HTTP->RLDP 代理进行访问的，而它们是通过在远程 Web 服务器上运行的反向 RLDP->HTTP 代理来创建的。

## 1. 编译 RLDP-HTTP 代理

RLDP-HTTP 代理是一个专门设计用于访问和创建 TON Sites 的实用程序。它的当前（alpha）版本是 TON 区块链源代码树的一部分，代码可以在 GitHub 仓库 ton-blockchain/ton 中找到。要编译 RLDP-HTTP 代理，请按照 README 和 Validator-HOWTO 中的说明进行操作。代理的二进制文件将位于构建目录的

    rldp-http-proxy/rldp-http-proxy

位置。或者，您可能希望仅构建代理，而不是构建所有 TON 区块链项目。这可以通过在构建目录中运行以下命令实现：

    cmake --build . --target rldp-http-proxy

## 2. 运行 RLDP-HTTP 代理以访问 TON Sites

要访问现有的 TON Sites，您需要在计算机上运行 RLDP-HTTP 代理实例。可以如下调用：

   rldp-http-proxy/rldp-http-proxy -p 8080 -c 3333 -C global-config.json

或者

   rldp-http-proxy/rldp-http-proxy -p 8080 -a <your_public_ip>:3333 -C global-config.json

其中 <your_public_ip> 是您的公共 IPv4 地址，前提是您在家用计算机上有一个。TON 网络全球配置文件 `global-config.json` 可以在 https://ton.org/global-config.json 下载：

   wget https://ton.org/global-config.json

在上述示例中，8080 是在本地主机上监听的 TCP 端口，用于接收 HTTP 查询，3333 是用于所有出站和入站 RLDP 和 ADNL 活动的 UDP 端口，即用于通过 TON 网络连接到 TON Sites。

如果一切正常，代理将不会终止，而是继续在终端中运行。现在可以用来访问 TON Sites。当不再需要时，您可以通过按 Ctrl-C 或简单地关闭终端窗口来终止它。

## 3. 访问 TON Sites

现在假设您在计算机上运行了一个 RLDP-HTTP 代理实例，并且正在监听 localhost:8080 的入站 TCP 连接，如第 2 节所述。

可以使用 Curl 或 WGet 等程序来进行简单的测试，确保一切正常。例如，

    curl -x 127.0.0.1:8080 http://test.ton

尝试使用位于 `127.0.0.1:8080` 的代理下载 (TON) 网站 `test.ton` 的主页。如果代理正在运行，您将看到类似以下内容：

```html
<HTML>
<H2>TON Blockchain Test Network &mdash; files and resources</H2>
<H3>News</H3>
<UL>
...
</HTML>
```

因为 TON 网站 `test.ton` 当前被设置为 `https://ton.org` 网站的镜像。

您还可以通过使用虚拟域 `<adnl-addr>.adnl` 来访问 TON Sites：

    curl -x 127.0.0.1:8080 http://untzo7eat2h77xzfugxrfgfy3zbl5txomvetzke6fwr45lehvdkxauy.adnl/

目前获取相同的 TON 网页。

另外，您可以在浏览器中将 `localhost:8080` 设置为 HTTP 代理。例如，如果您使用 Firefox，请访问 [设置] -> 常规 -> 网络设置 -> 设置 -> 配置代理访问 -> 手动代理配置，然后在“HTTP 代理”字段中输入 "127.0.0.1"，在“端口”字段中输入 "8080"。如果您还没有 Firefox，请先访问 https://www.getfirefox.com。

一旦将 `localhost:8080` 设置为浏览器中使用的 HTTP 代理，您可以简单地在浏览器的地址栏中输入所需的 URI，例如 `http://test.ton` 或 `http://untzo7eat2h77xzfugxrfgfy3zbl5txomvetzke6fwr45lehvdkxauy.adnl/`，并以与普通网站相同的方式与 TON Site 交互。

## 4. 创建 TON Sites

大多数人只需要访问现有的 TON Sites，而不是创建新的。但是，如果您想创建一个，您需要在服务器上运行 RLDP-HTTP 代理，并配合使用 Apache 或 Nginx 等常规 Web 服务器软件。

我们假设您已经知道如何设置普通网站，并且已经在服务器上配置了一个接受 TCP 端口 <your-server-ip>:80 的 HTTP 连接，并在 Web 服务器配置中将所需的 TON 网络域名，例如 `example.ton`，定义为主域名或别名。

之后，您需要首先为您的服务器生成一个持久的 ADNL 地址：

    mkdir keyring

    util/generate-random-id -m adnlid

您将看到类似以下内容：

    45061C1D4EC44A937D0318589E13C73D151D1CEF5D3C0E53AFBCF56A6C2FE2BD vcqmha5j3ceve35ammfrhqty46rkhi455otydstv66pk2tmf7rl25f3

这是您新生成的持久 ADNL 地址，以十六进制和用户友好的形式显示。相应的私钥保存在当前目录中的文件 45061...2DB 中。将其移动到 keyring 目录中：

    mv 45061C1* keyring/

之后，您可以在后台执行

    rldp-http-proxy -a <your-server-ip>:3333 -L '*' -C global-config.json -A <your-adnl-address>

（在此示例中 <your-adnl-address> 等于 'vcqm...35f3'）（您可以先在终端中尝试，但如果要让您的 TON Site 永久运行，您还需要使用 `-d` 和 `-l <log-file>` 选项）。

如果一切正常，RLDP-HTTP 代理将通过 UDP 端口 3333（当然，您可以使用任何其他 UDP 端口）接收来自 TON 网络的 HTTP 查询，并将这些 HTTP 查询转发到 127.0.0.1 上的 TCP 端口 80，即您的普通 Web 服务器。

您可以从客户端机器上的浏览器访问 TON Site `http://<your-adnl-address>.adnl`（在此示例中是 `http://vcqmha5j3ceve35ammfrhqty46rkhi455otydstv66pk2tmf7rl25f3.adnl`），并检查您的 TON Site 是否实际上对公众可用。

如果您愿意，可以注册一个 TON DNS 域名，例如 'example.ton'，并为此域名创建一个记录，指向您 TON Site 的持久 ADNL 地址。然后，运行在客户端模式的 RLDP-HTTP 代理将解析 'http://example.ton' 为您的 ADNL 地址，并访问您的 TON Site。TON DNS 域名的注册过程在另一份文档中进行了描述。
