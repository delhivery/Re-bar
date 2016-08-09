// Generated by the gRPC protobuf plugin.
// If you make any local change, they will be lost.
// source: expath.proto
#ifndef GRPC_expath_2eproto__INCLUDED
#define GRPC_expath_2eproto__INCLUDED

#include "expath.pb.h"

#include <grpc++/impl/codegen/async_stream.h>
#include <grpc++/impl/codegen/async_unary_call.h>
#include <grpc++/impl/codegen/proto_utils.h>
#include <grpc++/impl/codegen/rpc_method.h>
#include <grpc++/impl/codegen/service_type.h>
#include <grpc++/impl/codegen/status.h>
#include <grpc++/impl/codegen/stub_options.h>
#include <grpc++/impl/codegen/sync_stream.h>

namespace grpc {
class CompletionQueue;
class Channel;
class RpcService;
class ServerCompletionQueue;
class ServerContext;
}  // namespace grpc

namespace expath {

class EPServices GRPC_FINAL {
 public:
  class StubInterface {
   public:
    virtual ~StubInterface() {}
    virtual ::grpc::Status AddEdge(::grpc::ClientContext* context, const ::expath::Edge& request, ::expath::Response* response) = 0;
    std::unique_ptr< ::grpc::ClientAsyncResponseReaderInterface< ::expath::Response>> AsyncAddEdge(::grpc::ClientContext* context, const ::expath::Edge& request, ::grpc::CompletionQueue* cq) {
      return std::unique_ptr< ::grpc::ClientAsyncResponseReaderInterface< ::expath::Response>>(AsyncAddEdgeRaw(context, request, cq));
    }
    virtual ::grpc::Status RemoveEdge(::grpc::ClientContext* context, const ::expath::Edge& request, ::expath::Response* response) = 0;
    std::unique_ptr< ::grpc::ClientAsyncResponseReaderInterface< ::expath::Response>> AsyncRemoveEdge(::grpc::ClientContext* context, const ::expath::Edge& request, ::grpc::CompletionQueue* cq) {
      return std::unique_ptr< ::grpc::ClientAsyncResponseReaderInterface< ::expath::Response>>(AsyncRemoveEdgeRaw(context, request, cq));
    }
    virtual ::grpc::Status SearchEdge(::grpc::ClientContext* context, const ::expath::Edge& request, ::expath::FindResponse* response) = 0;
    std::unique_ptr< ::grpc::ClientAsyncResponseReaderInterface< ::expath::FindResponse>> AsyncSearchEdge(::grpc::ClientContext* context, const ::expath::Edge& request, ::grpc::CompletionQueue* cq) {
      return std::unique_ptr< ::grpc::ClientAsyncResponseReaderInterface< ::expath::FindResponse>>(AsyncSearchEdgeRaw(context, request, cq));
    }
  private:
    virtual ::grpc::ClientAsyncResponseReaderInterface< ::expath::Response>* AsyncAddEdgeRaw(::grpc::ClientContext* context, const ::expath::Edge& request, ::grpc::CompletionQueue* cq) = 0;
    virtual ::grpc::ClientAsyncResponseReaderInterface< ::expath::Response>* AsyncRemoveEdgeRaw(::grpc::ClientContext* context, const ::expath::Edge& request, ::grpc::CompletionQueue* cq) = 0;
    virtual ::grpc::ClientAsyncResponseReaderInterface< ::expath::FindResponse>* AsyncSearchEdgeRaw(::grpc::ClientContext* context, const ::expath::Edge& request, ::grpc::CompletionQueue* cq) = 0;
  };
  class Stub GRPC_FINAL : public StubInterface {
   public:
    Stub(const std::shared_ptr< ::grpc::ChannelInterface>& channel);
    ::grpc::Status AddEdge(::grpc::ClientContext* context, const ::expath::Edge& request, ::expath::Response* response) GRPC_OVERRIDE;
    std::unique_ptr< ::grpc::ClientAsyncResponseReader< ::expath::Response>> AsyncAddEdge(::grpc::ClientContext* context, const ::expath::Edge& request, ::grpc::CompletionQueue* cq) {
      return std::unique_ptr< ::grpc::ClientAsyncResponseReader< ::expath::Response>>(AsyncAddEdgeRaw(context, request, cq));
    }
    ::grpc::Status RemoveEdge(::grpc::ClientContext* context, const ::expath::Edge& request, ::expath::Response* response) GRPC_OVERRIDE;
    std::unique_ptr< ::grpc::ClientAsyncResponseReader< ::expath::Response>> AsyncRemoveEdge(::grpc::ClientContext* context, const ::expath::Edge& request, ::grpc::CompletionQueue* cq) {
      return std::unique_ptr< ::grpc::ClientAsyncResponseReader< ::expath::Response>>(AsyncRemoveEdgeRaw(context, request, cq));
    }
    ::grpc::Status SearchEdge(::grpc::ClientContext* context, const ::expath::Edge& request, ::expath::FindResponse* response) GRPC_OVERRIDE;
    std::unique_ptr< ::grpc::ClientAsyncResponseReader< ::expath::FindResponse>> AsyncSearchEdge(::grpc::ClientContext* context, const ::expath::Edge& request, ::grpc::CompletionQueue* cq) {
      return std::unique_ptr< ::grpc::ClientAsyncResponseReader< ::expath::FindResponse>>(AsyncSearchEdgeRaw(context, request, cq));
    }

   private:
    std::shared_ptr< ::grpc::ChannelInterface> channel_;
    ::grpc::ClientAsyncResponseReader< ::expath::Response>* AsyncAddEdgeRaw(::grpc::ClientContext* context, const ::expath::Edge& request, ::grpc::CompletionQueue* cq) GRPC_OVERRIDE;
    ::grpc::ClientAsyncResponseReader< ::expath::Response>* AsyncRemoveEdgeRaw(::grpc::ClientContext* context, const ::expath::Edge& request, ::grpc::CompletionQueue* cq) GRPC_OVERRIDE;
    ::grpc::ClientAsyncResponseReader< ::expath::FindResponse>* AsyncSearchEdgeRaw(::grpc::ClientContext* context, const ::expath::Edge& request, ::grpc::CompletionQueue* cq) GRPC_OVERRIDE;
    const ::grpc::RpcMethod rpcmethod_AddEdge_;
    const ::grpc::RpcMethod rpcmethod_RemoveEdge_;
    const ::grpc::RpcMethod rpcmethod_SearchEdge_;
  };
  static std::unique_ptr<Stub> NewStub(const std::shared_ptr< ::grpc::ChannelInterface>& channel, const ::grpc::StubOptions& options = ::grpc::StubOptions());

  class Service : public ::grpc::Service {
   public:
    Service();
    virtual ~Service();
    virtual ::grpc::Status AddEdge(::grpc::ServerContext* context, const ::expath::Edge* request, ::expath::Response* response);
    virtual ::grpc::Status RemoveEdge(::grpc::ServerContext* context, const ::expath::Edge* request, ::expath::Response* response);
    virtual ::grpc::Status SearchEdge(::grpc::ServerContext* context, const ::expath::Edge* request, ::expath::FindResponse* response);
  };
  template <class BaseClass>
  class WithAsyncMethod_AddEdge : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service *service) {}
   public:
    WithAsyncMethod_AddEdge() {
      ::grpc::Service::MarkMethodAsync(0);
    }
    ~WithAsyncMethod_AddEdge() GRPC_OVERRIDE {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status AddEdge(::grpc::ServerContext* context, const ::expath::Edge* request, ::expath::Response* response) GRPC_FINAL GRPC_OVERRIDE {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    void RequestAddEdge(::grpc::ServerContext* context, ::expath::Edge* request, ::grpc::ServerAsyncResponseWriter< ::expath::Response>* response, ::grpc::CompletionQueue* new_call_cq, ::grpc::ServerCompletionQueue* notification_cq, void *tag) {
      ::grpc::Service::RequestAsyncUnary(0, context, request, response, new_call_cq, notification_cq, tag);
    }
  };
  template <class BaseClass>
  class WithAsyncMethod_RemoveEdge : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service *service) {}
   public:
    WithAsyncMethod_RemoveEdge() {
      ::grpc::Service::MarkMethodAsync(1);
    }
    ~WithAsyncMethod_RemoveEdge() GRPC_OVERRIDE {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status RemoveEdge(::grpc::ServerContext* context, const ::expath::Edge* request, ::expath::Response* response) GRPC_FINAL GRPC_OVERRIDE {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    void RequestRemoveEdge(::grpc::ServerContext* context, ::expath::Edge* request, ::grpc::ServerAsyncResponseWriter< ::expath::Response>* response, ::grpc::CompletionQueue* new_call_cq, ::grpc::ServerCompletionQueue* notification_cq, void *tag) {
      ::grpc::Service::RequestAsyncUnary(1, context, request, response, new_call_cq, notification_cq, tag);
    }
  };
  template <class BaseClass>
  class WithAsyncMethod_SearchEdge : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service *service) {}
   public:
    WithAsyncMethod_SearchEdge() {
      ::grpc::Service::MarkMethodAsync(2);
    }
    ~WithAsyncMethod_SearchEdge() GRPC_OVERRIDE {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status SearchEdge(::grpc::ServerContext* context, const ::expath::Edge* request, ::expath::FindResponse* response) GRPC_FINAL GRPC_OVERRIDE {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    void RequestSearchEdge(::grpc::ServerContext* context, ::expath::Edge* request, ::grpc::ServerAsyncResponseWriter< ::expath::FindResponse>* response, ::grpc::CompletionQueue* new_call_cq, ::grpc::ServerCompletionQueue* notification_cq, void *tag) {
      ::grpc::Service::RequestAsyncUnary(2, context, request, response, new_call_cq, notification_cq, tag);
    }
  };
  typedef WithAsyncMethod_AddEdge<WithAsyncMethod_RemoveEdge<WithAsyncMethod_SearchEdge<Service > > > AsyncService;
  template <class BaseClass>
  class WithGenericMethod_AddEdge : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service *service) {}
   public:
    WithGenericMethod_AddEdge() {
      ::grpc::Service::MarkMethodGeneric(0);
    }
    ~WithGenericMethod_AddEdge() GRPC_OVERRIDE {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status AddEdge(::grpc::ServerContext* context, const ::expath::Edge* request, ::expath::Response* response) GRPC_FINAL GRPC_OVERRIDE {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
  };
  template <class BaseClass>
  class WithGenericMethod_RemoveEdge : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service *service) {}
   public:
    WithGenericMethod_RemoveEdge() {
      ::grpc::Service::MarkMethodGeneric(1);
    }
    ~WithGenericMethod_RemoveEdge() GRPC_OVERRIDE {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status RemoveEdge(::grpc::ServerContext* context, const ::expath::Edge* request, ::expath::Response* response) GRPC_FINAL GRPC_OVERRIDE {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
  };
  template <class BaseClass>
  class WithGenericMethod_SearchEdge : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service *service) {}
   public:
    WithGenericMethod_SearchEdge() {
      ::grpc::Service::MarkMethodGeneric(2);
    }
    ~WithGenericMethod_SearchEdge() GRPC_OVERRIDE {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status SearchEdge(::grpc::ServerContext* context, const ::expath::Edge* request, ::expath::FindResponse* response) GRPC_FINAL GRPC_OVERRIDE {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
  };
};

}  // namespace expath


#endif  // GRPC_expath_2eproto__INCLUDED