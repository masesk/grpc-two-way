/*
 *
 * Copyright 2021 gRPC authors.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <iostream>
#include <memory>
#include <mutex>
#include <thread>
#include <random>

#include <grpc/grpc.h>
#include <grpcpp/security/server_credentials.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>

#include <grpcpp/grpcpp.h>
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

class TwoWayServiceImpl final : public TwoWay::Service
{
public:
  Status Chat(ServerContext *context, const Message *request,
              Empty *reply) override
  {
    std::cout << request->user() << ": " << request->message() << std::endl;
    return Status::OK;
  }
};

class TwoWayClient
{
public:
  TwoWayClient(std::shared_ptr<grpc::Channel> channel, int port)
      : stub_(TwoWay::NewStub(channel)), channel_(channel)
  {
    this->port = port;
    channel->NotifyOnStateChange(channel->GetState(true), std::chrono::system_clock::time_point::max(), &this->state_cq, nullptr);
    notify_state_thread = std::thread([this]
                                        {
      void *tag = nullptr;
      bool ok = false;

      while (this->state_cq.Next(&tag, &ok))
      
        {

          // get state
          grpc_connectivity_state state = this->channel_->GetState(true);
          this->channel_->NotifyOnStateChange(state, std::chrono::system_clock::time_point::max(), &this->state_cq, nullptr);
          // if we were ready and now we are not (failed or idle), we have a problem
          if (state == GRPC_CHANNEL_READY){
              Subscription request;
              request.set_port(std::to_string(this->port));
              SubscriptionResponse reply;
              grpc::ClientContext context;
              Status status = this->stub_->Connect(&context, request, &reply);
          }
        } });
    notify_state_thread.detach();
  }
  ~TwoWayClient() {

  }

  // Assembles the client's payload, sends it and presents the response back
  // from the server.
  void Connect()
  {
    // Data we are sending to the server.
    Subscription request;
    request.set_address("127.0.0.1");
    request.set_port(std::to_string(this->port));

    // Container for the data we expect from the server.
    SubscriptionResponse reply;

    // Context for the client. It could be used to convey extra information to
    // the server and/or tweak certain RPC behaviors.
    grpc::ClientContext context;

    server_thread = std::thread([this]
                                { this->RunServer(this->port); });
    server_thread.join();
  }

  void Chat(const std::string &user, const std::string &message)
  {
    Message request;
    request.set_user(user);
    request.set_message(message);

    Empty reply;

    grpc::ClientContext context;

    Status status = stub_->Chat(&context, request, &reply);
  }

private:
  grpc::CompletionQueue state_cq;
  std::thread notify_state_thread;
  std::thread server_thread;
  std::shared_ptr<grpc::Channel> channel_;
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
    std::cout << "Server listening on " << server_address << std::endl;

    // Wait for the server to shutdown. Note that some other thread must be
    // responsible for shutting down the server for this call to ever return.
    server->Wait();
  }
  std::unique_ptr<TwoWay::Stub> stub_;
  int port;
};

int main(int argc, char **argv)
{
  // ask for username first
  if (argc <= 1)
  {
    std::cerr << "Pass port number as argument" << std::endl;
    return 1;
  }

  std::string port(argv[1]);
  int port_n = std::stoi(port);

  std::string username;
  std::cout << "Type your username: ";
  // grab and store the username
  getline(std::cin, username);

  auto channel = grpc::CreateChannel("127.0.0.1:50051", grpc::InsecureChannelCredentials());
  TwoWayClient clientService(channel, port_n);

  // create a detached thread that continously asks the user for input
  std::thread writer([&username, &clientService]
                     {
    std::string input;
    std::cout << "Type your username: ";
    std::cout << "Chat has started!" << std::endl;
    while (input != "quit") {
      getline(std::cin, input);
      clientService.Chat(username, input);
      
    } });
  writer.detach();

  clientService.Connect();

  return 0;
}