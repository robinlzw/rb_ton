# 安装 TON 节点
有三种类型的节点：
- 轻节点
- 全节点
- 归档节点
任何节点至少需要 16GB 的 RAM 和 8核 的 CPU，但磁盘使用量取决于节点类型。目前只有 Linux 系统经过了良好的测试。

## 轻节点
轻节点只有中间状态，基本上只是将所有请求代理到其他节点。这有助于调节工作负载。轻节点需要 约10GB 的磁盘空间。

## 全节点
全节点拥有区块链的完整状态（但不包括交易历史），它适用于稳定访问区块链。全节点起初需要 约10GB 的磁盘空间，但会以每周 约10GB 的速度增长。

## 归档节点
归档节点是全节点，但它可以访问区块链的全部历史。当前的建议是拥有 6TB 的 SSD。

## 安装轻节点或全节点
安装节点的最简单方式是使用 mytonctrl。
在 bash 中执行此命令，将 mode 替换为 lite 或 full：
```
bash wget https://raw.githubusercontent.com/igroman787/mytonctrl/master/scripts/install.sh sudo bash install.sh -m <mode>
```
安装完成后，您可以访问 mytonctrl。在 shell 中执行：
```
bash mytonctrl
```

## 暴露端口
每个节点都暴露一个随机的 UDP 端口。要找到它，请执行：
```
bash sudo netstat -tunlp
```
然后找到带有 UDP 端口的 validator-engine 进程，并配置您的防火墙和路由器。同步只需要这个端口。

## 连接到轻服务器
轻服务器是 TON 节点的一个组件，用于服务客户端连接并执行各种查询。如果您想使用本地节点之外的轻服务器，您需要获取轻客户端配置。

启动 mytonctrl：
```
bash mytonctrl
```
在 mytonctrl 中执行：
```
bash installer
```
在安装程序中执行：
```
bash plsc
```
您将获得您的轻客户端配置，如下所示：
```
{
    "ip": "<ip>",
    "port": "<port>",
    "id": {
        "@type": "pub.ed25519",
        "key": "<key>"
    }
```
打开路由器/防火墙上的 端口，以使轻服务器可以从外部访问。

## （高级）安装归档节点
首先，您需要在服务器上拥有 6TB SSD，但当前方法依赖于 ZFS，如果您启用 ZFS 压缩，可以使用 3TB。

1.安装 zfs，创建新池（data）。
2.启用压缩：zfs set compression=lz4 data
3.创建卷：zfs create data/ton-work
4.安装 mytonctrl
5.停止验证器进程：systemctl stop validator
6.备份配置：mv /var/ton-work /var/ton-work.bak
7.应用转储（见下文）
8.挂载 zfs：zfs set mountpoint=/var/ton-work data/ton-work && zfs mount data/ton-work
9.从备份复制 config.json、keys 和 db/keyring 到 /var/ton-work
10.修复权限：chown -R validator:validator /var/ton-work
11.启动验证器：systemctl start validator

## 应用转储
在 http://anode2.ton.swisscops.com 请求凭据。
获取凭据后，执行：
```
bash sudo apt-get instapp pv plzip curl -u <username>:<password> -s http://anode2.ton.swisscops.com/dumps/<link_for_last_dump> | pv | plzip -d -n48 | zfs recv data/ton-work
```

## 测试网节点
```
bash wget https://raw.githubusercontent.com/igroman787/mytonctrl/master/scripts/install.sh sudo bash install.sh -m full -c https://newton-blockchain.github.io/testnet-global.config.json
```

---
ref: https://tonwhales.com/docs/node

