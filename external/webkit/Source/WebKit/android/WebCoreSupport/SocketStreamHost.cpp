// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "SocketStreamHost.h"
//#include "base/logging.h"
#include "net/socket_stream/socket_stream.h"
//#include "chrome/common/net/socket_stream.h"
//#include "chrome/common/net/url_request_context_getter.h"
//#include "chrome/common/render_messages.h"
#include "net/socket_stream/socket_stream_job.h"
#include "WebUrlLoaderClient.h"
#include "android/autofill/android_url_request_context_getter.h"
#include "WebRequestContext.h"
#include "WebCache.h"

static const char* kSocketHostKey = "socketHost";

using namespace android;
using namespace net;

namespace WebCore { 

class SocketStreamInfo : public net::SocketStream::UserData {
 public:
  explicit SocketStreamInfo(SocketStreamHost* host) : host_(host) {}
  virtual ~SocketStreamInfo() {}
  SocketStreamHost* host() const { return host_; }

 private:
  SocketStreamHost* host_;
};

SocketStreamHost::SocketStreamHost(
     SocketStreamHandlePrivate* sockstream,
    int socket_id)
    :  m_sockstream(sockstream),
      socket_id_(socket_id) {
  //DCHECK_NE(socket_id_, chrome_common_net::kNoSocketId);
  //VLOG(1) << "SocketStreamHost: socket_id=" << socket_id_;
}

/* static */
SocketStreamHost*
SocketStreamHost::GetSocketStreamHost(net::SocketStream* socket) {
  net::SocketStream::UserData* d = socket->GetUserData(kSocketHostKey);
  if (d) {
    SocketStreamInfo* info = static_cast<SocketStreamInfo*>(d);
    return info->host();
  }
  return NULL;
}

SocketStreamHost::~SocketStreamHost() {
  //VLOG(1) << "SocketStreamHost destructed socket_id=" << socket_id_;
  //if (!receiver_->Send(new ViewMsg_SocketStream_Closed(socket_id_)))
    //LOG(ERROR) << "ViewMsg_SocketStream_Closed failed.";
  socket_->DetachDelegate();
}

void SocketStreamHost::Connect(const GURL& url) {
  //VLOG(1) << "SocketStreamHost::Connect url=" << url;
    WebRequestContext * req_context = new WebRequestContext(false);
     
    ProxyConfig* config = new ProxyConfig();
    WebCache::get(false)->proxy()->GetLatestProxyConfig(config);
    
    std::string proxy("");    
    if(!config->proxy_rules().single_proxy.host_port_pair().host().empty()) 
		proxy = config->proxy_rules().single_proxy.host_port_pair().ToString();
    
    std::string exlist("");
    ProxyConfigServiceAndroid* proxyConfigService = new ProxyConfigServiceAndroid();
    proxyConfigService->UpdateProxySettings(proxy, exlist);
    ProxyService * proxy_service = net::ProxyService::CreateWithoutProxyResolver(proxyConfigService, 0 /* net_log */);    
    //ProxyService * proxy_service = net::ProxyService::CreateWithoutProxyResolver(WebCache::get(false)->proxy(), 0 /* net_log */);	
    SSLConfigService* ssl_config_service = net::SSLConfigService::CreateSystemSSLConfigService();
	HostResolver *hostResolver = net::CreateSystemHostResolver(net::HostResolver::kDefaultParallelism, 0, 0);
	
	req_context->set_host_resolver(hostResolver);
    req_context->set_proxy_service(proxy_service);
    req_context->set_ssl_config_service(ssl_config_service);
    
    //req_context->set_http_auth_handler_factory(net::HttpAuthHandlerFactory::CreateDefault(hostResolver));
    req_context->set_cert_verifier(new CertVerifier());
    
    socket_ = net::SocketStreamJob::CreateSocketStreamJob(url, this, *req_context);
    socket_->set_context(req_context);
 
    socket_->SetUserData(kSocketHostKey, new SocketStreamInfo(this));
    socket_->Connect();
}

bool SocketStreamHost::SendData(const std::vector<char>& data) {
  //VLOG(1) << "SocketStreamHost::SendData";
  return socket_ && socket_->SendData(&data[0], data.size());
}

void SocketStreamHost::Close() {
  //VLOG(1) << "SocketStreamHost::Close";
  if (!socket_)
    return;
  socket_->Close();
}

void SocketStreamHost::OnConnected(net::SocketStream*, int max_pending_send_allowed)
{
 m_sockstream->maybeCallOnMainThread(NewRunnableMethod(
                m_sockstream.get(), &SocketStreamHandlePrivate::OnConnected, max_pending_send_allowed));
}
void SocketStreamHost::OnSentData(net::SocketStream*, int sentlen)
{
 m_sockstream->maybeCallOnMainThread(NewRunnableMethod(
                m_sockstream.get(), &SocketStreamHandlePrivate::OnSentData, sentlen));
}
void SocketStreamHost::OnClose(net::SocketStream*)
{
 m_sockstream->maybeCallOnMainThread(NewRunnableMethod(
                m_sockstream.get(), &SocketStreamHandlePrivate::OnClose));
}
void SocketStreamHost::OnReceivedData(net::SocketStream*, const char* data, int len)
{
	
 m_sockstream->maybeCallOnMainThread(NewRunnableMethod(
                m_sockstream.get(), &SocketStreamHandlePrivate::OnReceivedData, std::vector<char>(data, data + len)));
}

}//namespace WebCore


