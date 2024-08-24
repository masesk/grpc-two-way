#include <memory>
#include <thread>
#include <iostream>
#include <sstream>
#include <list>
#include <memory>
#include <grpcpp/grpcpp.h>
#include <grpc/grpc.h>
#include <grpcpp/security/server_credentials.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include "two_way.grpc.pb.h"
#include "google/protobuf/any.pb.h"

using google::protobuf::Empty;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using twoway::Message;
using twoway::Subscription;
using twoway::SubscriptionResponse;
using twoway::SubscriptionStatus;
using twoway::TwoWay;

class TwoWayClient
{
public:
  TwoWayClient(const std::string &address, std::mutex &mutex, std::list<TwoWayClient *> &clients)
  : mutex_(mutex), clients_(clients)
  {
    channel_ = grpc::CreateChannel(address, grpc::InsecureChannelCredentials());
    stub_ = TwoWay::NewStub(channel_);
    std::unique_lock<std::mutex> lock(mutex_);
    clients.push_back(this);
    iterator_ = clients.end();
    iterator_--;
  }

  // Assembles the client's payload, sends it and presents the response back
  // from the server.
  bool SendMessage(const Message &message)
  {
    // Data we are sending to the server.
    Message request(message);

    // Container for the data we expect from the server.
    Empty reply;

    // Context for the client. It could be used to convey extra information to
    // the server and/or tweak certain RPC behaviors.
    grpc::ClientContext context;
    context.set_wait_for_ready(false);

    // The actual RPC.
    Status status = stub_->Chat(&context, request, &reply);

    // Act upon its status.
    if (!status.ok())
    {
      std::cout << "Deleting this with address" << std::endl;
      return false;
    }
    return true;
  }

private:
  std::mutex &mutex_;
  std::shared_ptr<grpc::Channel> channel_;
  std::unique_ptr<TwoWay::Stub> stub_;
  std::list<TwoWayClient *> &clients_;
  std::list<TwoWayClient *>::iterator iterator_;
};

// Logic and data behind the server's behavior.
class TwoWayServiceImpl final : public TwoWay::Service
{
public:
  Status Connect(ServerContext *context, const Subscription *request,
                 SubscriptionResponse *reply) override
  {
    std::stringstream address_stream;
    if (request->has_address())
    {
      address_stream << request->address();
    }
    else
    {
      address_stream << context->peer();
    }
    address_stream << ":" << request->port();
    TwoWayClient *newClient = new TwoWayClient(address_stream.str(), mutex, clients);
    return Status::OK;
  }

  Status Chat(ServerContext *context, const Message *request,
              Empty *reply) override
  {
    std::unique_lock<std::mutex> lock(mutex);
    std::list<TwoWayClient *>::iterator it = clients.begin();
    while(it != clients.end())
    {

      if((*it)->SendMessage(*request)){
        
        ++it;
      }
      else{
        delete (*it);
        it = clients.erase(it);
      }
    }
    return Status::OK;
  }

private:
  std::list<TwoWayClient *> clients;
  std::mutex mutex;
};

void RunServer(uint16_t port)
{
  std::stringstream ss;
  ss << "0.0.0.0:" << port;
  std::string server_address = ss.str();
  TwoWayServiceImpl service;

  grpc::EnableDefaultHealthCheckService(true);
  ServerBuilder builder;
  // Listen on the given address without any authentication mechanism.
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  // Register "service" as the instance through which we'll communicate with
  // clients. In this case it corresponds to an *synchronous* service.
  builder.RegisterService(&service);
  // Finally assemble the server.
  std::unique_ptr<Server> server(builder.BuildAndStart());

  // Wait for the server to shutdown. Note that some other thread must be
  // responsible for shutting down the server for this call to ever return.
  server->Wait();
}

int main(int argc, char **argv)
{
  RunServer(50051);
  return 0;
}