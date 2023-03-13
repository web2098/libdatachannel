/**
 * Copyright (c) 2020 Paul-Louis Ageneau
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#ifndef RTC_IMPL_TLS_TRANSPORT_H
#define RTC_IMPL_TLS_TRANSPORT_H

#include "certificate.hpp"
#include "common.hpp"
#include "queue.hpp"
#include "tls.hpp"
#include "transport.hpp"

#if RTC_ENABLE_WEBSOCKET

#include <atomic>
#include <thread>

namespace rtc::impl {

class TcpTransport;
class TcpProxyTransport;

class TlsTransport : public Transport, public std::enable_shared_from_this<TlsTransport> {
public:
	static void Init();
	static void Cleanup();

	TlsTransport(variant<shared_ptr<TcpTransport>, shared_ptr<TcpProxyTransport>> lower, optional<string> host, certificate_ptr certificate,
	             state_callback callback);
	virtual ~TlsTransport();

	void start() override;
	void stop() override;
	bool send(message_ptr message) override;

	bool isClient() const { return mIsClient; }

protected:
	virtual void incoming(message_ptr message) override;
	virtual bool outgoing(message_ptr message) override;
	virtual void postHandshake();

	void enqueueRecv();
	void doRecv();

	const optional<string> mHost;
	const bool mIsClient;

	Queue<message_ptr> mIncomingQueue;
	std::atomic<int> mPendingRecvCount = 0;
	std::mutex mRecvMutex;

#if USE_GNUTLS
	gnutls_session_t mSession;

	message_ptr mIncomingMessage;
	size_t mIncomingMessagePosition = 0;
	std::atomic<bool> mOutgoingResult = true;

	static ssize_t WriteCallback(gnutls_transport_ptr_t ptr, const void *data, size_t len);
	static ssize_t ReadCallback(gnutls_transport_ptr_t ptr, void *data, size_t maxlen);
	static int TimeoutCallback(gnutls_transport_ptr_t ptr, unsigned int ms);
#else
	SSL_CTX *mCtx;
	SSL *mSsl;
	BIO *mInBio, *mOutBio;

	bool flushOutput();

	static BIO_METHOD *BioMethods;
	static int TransportExIndex;
	static std::mutex GlobalMutex;

	static int CertificateCallback(int preverify_ok, X509_STORE_CTX *ctx);
	static void InfoCallback(const SSL *ssl, int where, int ret);

	static int BioMethodNew(BIO *bio);
	static int BioMethodFree(BIO *bio);
	static int BioMethodWrite(BIO *bio, const char *in, int inl);
	static long BioMethodCtrl(BIO *bio, int cmd, long num, void *ptr);
#endif
};

} // namespace rtc::impl

#endif

#endif
