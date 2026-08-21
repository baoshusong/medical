#pragma once

#include <QObject>
#include <QString>

namespace medical {

// 局域网脱敏 REST 客户端：客户端不直连 PostgreSQL，经中间服务取/存数据。
// USE_HTTPLIB 开启后用 cpp-httplib 实现真实请求；否则 Mock。
class RestClient : public QObject
{
    Q_OBJECT
public:
    explicit RestClient(QObject *parent = nullptr);

    void setBaseUrl(const QString &url);

    // 伪代码占位：拉取远程报告 / 上传脱敏影像特征。
    QString fetchReport(int reportId);
    bool    uploadFindings(const QString &json);

    QString connectionInfo() const;

private:
    QString m_baseUrl;
};

} // namespace medical
