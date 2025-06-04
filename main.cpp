// app/main.cpp
#include <boost/filesystem.hpp>
#include <fmt/format.h>
#include <fstream>
#include "framework/server.hpp"
#include "framework/context/http_context.hpp"
#include "framework/context/websocket_context.hpp"
#include <boost/asio/ip/tcp.hpp>
#include <thread>
#include <memory>
#include "controller/HelloWsController.hpp"
#include "controller/HelloController.hpp"

namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;
namespace beast = boost::beast;

int main(int argc, char* argv[])
{
  auto const address = net::ip::make_address("0.0.0.0");
  auto const port = static_cast<unsigned short>(8080);
  auto const num_threads = std::max<int>(1, static_cast<int>(std::thread::hardware_concurrency()));

  fmt::print("Starting khttpd server with {} worker threads...\n", num_threads);

  // 定义 Web 根目录
  std::string web_root_path = "web_root";
  // 创建一个简单的 web_root 目录和文件用于测试
  boost::filesystem::create_directories(web_root_path);
  std::ofstream("web_root/index.html") <<
    "<html><body><h1>Hello from Static Index!</h1><p>Visit <a href=\"/hello_static.html\">hello_static.html</a></p></body></html>";
  std::ofstream("web_root/hello_static.html") <<
    R"(<html><body><h2>This is a static HTML file.</h2><img src="/image.png" alt="Static Image"/></body></html>)";

  auto server = std::make_shared<khttpd::framework::Server>(
    tcp::endpoint{address, port}, web_root_path, num_threads);

  auto& http_router = server->get_http_router();
  auto& ws_router = server->get_websocket_router();

  HelloController::create()->register_routes(http_router)->register_routes(ws_router);
  HelloWsController::create()->register_routes(http_router)->register_routes(ws_router);

  http_router.get("/hello", [](khttpd::framework::HttpContext& ctx)
  {
    std::string name = ctx.get_query_param("name").value_or("Guest");
    ctx.set_status(beast::http::status::ok);
    ctx.set_content_type("text/plain");
    ctx.set_body(fmt::format("Hello, {}!", name));
  });

  http_router.post("/api/json", [](khttpd::framework::HttpContext& ctx)
  {
    if (auto json_value = ctx.get_json())
    {
      std::string response_body = "Received JSON:\n" + boost::json::serialize(*json_value);
      ctx.set_status(beast::http::status::ok);
      ctx.set_content_type("application/json");
      ctx.set_body(response_body);
    }
    else
    {
      ctx.set_status(beast::http::status::bad_request);
      ctx.set_content_type("text/plain");
      ctx.set_body("Invalid JSON or Content-Type not application/json");
    }
  });

  server->run();

  fmt::print("Application exited.\n");

  return 0;
}

