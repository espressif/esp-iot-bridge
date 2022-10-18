# Mesh-Lite 功能简介

## 1.概述

Mesh-Lite 为简化版的 Wi-Fi mesh 功能，其基于 Wi-Fi 层实现 Mesh 组网，父节点对子节点数据进行透传转发，达到子节点连接互联网的目的。Wi-Fi 路由器以及无线网卡方案，均可使能 Mesh-Lite 功能。

## 2.Mesh-Lite 格式定义及含义说明

| 参数名称                          | 长度   | 含义                                                         |
| --------------------------------- | ------ | ------------------------------------------------------------ |
| Version                           | 1 byte | Mesh-Lite 版本号                                              |
| Max connect number                | 4 bit  | 本节点作为 SoftAP 允许连接的最大个数                         |
| Connected station number          | 4 bit  | 本节点作为 SoftAP 已经连接的 Station 个数                    |
| Connect router status             | 1 bit  | 本节点或者父节点是否已经连接到外部路由器                     |
| Reserved                          | 3 bit  | 暂未使用                                                     |
| Level                             | 4 bit  | 本节点所属层级                                               |
| Router SSID len                   | 1 byte | 路由器 SSID 长度，具体 SSID 信息在 Router SSID 字段          |
| Trace router number               | 4 bit  | 根节点记录的路由器个数，具体网段信息在 Router network segment list 字段 |
| Extern netif number               | 4 bit  | 连接外部网络的网络接口个数，具体网段信息在 Extern netif network segment list 字段 |
| Router SSID                       | m byte | 路由器 SSID                                                  |
| Router network segment list       | n byte | 记录的路由器 IP 网段列表                                     |
| Extern netif network segment list | k byte | 连接外部网络的网络接口 IP 网段列表                           |

## 3.流程介绍

- ESP 设备上电后会首先进行扫描，如果扫描到有对应 Mesh-Lite 节点信息，便会自动连接对应的节点；如果未扫描到 Mesh-Lite 节点信息，则直接连接路由器。
- 当根节点移除后，Level 2 的节点会选择连接到路由器，作为新的根节点
- 当父节点（非根节点）被移除后，对应的子节点会重现选择节点位置，并进行连接

## 4.示例

<img src="./_static/Mesh-Lite_Network_Architecture.png" alt="Mesh-Lite_Network_Architecture" style="zoom:50%;" />

<center>ESP-Mesh-Lite Network Architecture</center>
