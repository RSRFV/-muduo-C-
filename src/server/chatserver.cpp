#include "chatserver.hpp"
#include "chatservice.hpp"
#include "json.hpp"

#include <functional>
#include <string>

using json = nlohmann::json;

ChatServer::ChatServer(EventLoop *loop,
               const InetAddress &listenAddr,
               const string &nameArg)
    : _server(loop, listenAddr, nameArg)
    , _loop(loop)
{
    // 注册用户连接创建和断开事件回调
    _server.setConnectionCallback(
        std::bind(&ChatServer::onConnection, this, _1)
    );
    // 注册用户的读写事件的回调
    _server.setMessageCallback(
        std::bind(&ChatServer::onMessage, this, _1, _2, _3)
    );

    // 设置线程数量
    _server.setThreadNum(4);
}

// 启动服务
void ChatServer::start()
{
    _server.start();
}

// 上报连接的回调函数
void ChatServer::onConnection(const TcpConnectionPtr &conn)
{
    // 客户断开连接
    if (!conn->connected())
    {
        ChatService::instance()->clientCloseException(conn);    // 客户端异常关闭的处理业务
        conn->shutdown();
    }
}

// 处理用户读写事件
void ChatServer::onMessage(const TcpConnectionPtr &conn, Buffer *buffer, Timestamp time)
{
    //将接收数据全部放入字符串中
    string buf = buffer->retrieveAllAsString();
    cout << "recv data:" << buf << " time:" << time.toString() << endl;
    // 数据的发序列化
    json js = json::parse(buf);

    // 完全解耦（1.通过接口 2.回调）网络模块和业务模块的代码
    // 通过拿到的msgid，获取用于处理业务的handler
    auto msgHandler = ChatService::instance()->getHandler(js["msgid"].get<int>());
    // 回调消息绑定的处理器，来执行相应的业务处理
    msgHandler(conn, js, time);

    // conn->send(buf); //收到什么数据发回去什么数据
}
