#pragma once
#include <QObject>
#include <QHttpHeaders>
#include <memory>

class FetchApiOpt;

class FetchApiService : public QObject {
	Q_OBJECT
public:
	using SuccessCallback = std::function<void(int statusCode, const QByteArray& data, const QHttpHeaders& headers)>;
	using ErrorCallback = std::function<void(int errorCode, const QString& errorString, const QHttpHeaders& headers)>;
	using ProgressCallback = std::function<void(qint64 received, qint64 total)>;

	FetchApiService(QObject* parent = nullptr);
	~FetchApiService() override;

	bool fetchRequest(
		const FetchApiOpt& request,
		SuccessCallback onSuccess = nullptr,
		ErrorCallback onError = nullptr,
		ProgressCallback onProgress = nullptr);

private:
	class Private;
	std::unique_ptr<Private> d;
};
