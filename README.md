# Tcp Agent Application With Encrypted Transimission #
# 多用户高并发的tcp加密代理应用软件 #

## 综述 ##
 本项目为传输加密的TCP代理软件，对于使用TCP协议的上层服务，提供零适配的数据加密传输认证机制，实现数据通道安全需求。

## 技术设计 ##
 * 继承并优化了多线程任务门的实现，采用更健壮的自旋锁(由于缓存同步难以解决，放弃lockless).
 * 采用poll接口实现io复用，适用于中等规模的socket连接及大数据量传输场景；针对汇聚层或核心层网络传输，比epoll更有效。
 * 源于解决对HTTP网站数据安全代理的具体需求，本项目可实现国密套件(SM2/SM3/SM4)的全流程登录认证及加密。
 * 本程序主体为三个线程：IO数据接收和发送线程、用户登录和数据处理线程、以及控制主线程。
 * 每个socket自有发送和接收两个队列，同时线程以socket为单元顺序处理消息；与epoll模型设计一致。
 * 采用自定义的数据分包及重整技术，有效解决了多通道数据复用及分离问题。

## 技术特色 ##
 * 使用场景为：|User| <-> |Agent\_Client| <-> |Agent\_Server| <-> |Destination\_Server|.
 * 支持多用户多任务高并发场景，根据客户侧代理配置的服务端口号区分不同服务，同时在服务侧对服务器还原用户请求。
 * 作为TCP协议的传输代理，可零适配对接现场环境对数据加密的安全需求。不需要任何额外配置，就可以实现TCP数据在复杂外部环境下的安全加密穿透。

## 工程说明 ##
### 编译依赖 ###
 * 本项目仅支持linux系统编译运行，没有其他需求暂不适配windows系统。
 * 本项目使用openssl API接口实现国密套件，在Centos\_6.5和Opensuse\_15.2两个版本上均采用openssl-1.1.1q生成的libcrypto.a测试正常。
 * 除使用openssl的libcrypto.a库外，本项目不使用任何其他非系统内置第三方库。
 * 项目自带的libcrypto.a库是Opensuse\_15.2上编译的，其它Linux系统可自行编译。

### 编译运行 ###
 * 在src目录下运行[make], 生成bin\_test可执行程序。
 * 在src目录下执行 ./bin\_test 1 agent\_srv.conf，开始运行代理服务端应用。
 * 在src目录下执行 ./bin\_test 1 agent\_cli.conf，开始运行代理客户端应用。
 * 示例配置下，即为代理'172.8.1.11:2222'为'172.8.1.11:22'。

## The End ##
 **It is tough going.**