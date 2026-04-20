#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/string.hpp>
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <thread>
#include <atomic>
#include <esp32_bridge/msg/imu.hpp>
#include <cstring>

constexpr int UDP_PORT = 4210;
constexpr int BUFFER_SIZE = 1024;

class UdpBridge : public rclcpp::Node {

  private:
    rclcpp::Publisher<esp32_bridge::msg::Imu>::SharedPtr publisher_;
    int sock_;
    std::thread recv_thread_;
    std::atomic_bool running_{true};


  public:

    UdpBridge() : Node("UdpBridge") {
      RCLCPP_INFO(this->get_logger(), "Node uruchomiony");
      publisher_ = this->create_publisher<esp32_bridge::msg::Imu>("/esp32/data", 10);

      sock_ = ::socket(AF_INET, SOCK_DGRAM, 0);
      sockaddr_in addr{};
      addr.sin_family = AF_INET;
      addr.sin_port = htons(UDP_PORT);
      addr.sin_addr.s_addr = INADDR_ANY;
      int wynik = ::bind(sock_, reinterpret_cast<sockaddr*>(&addr),sizeof(addr));
      if(wynik < 0){
        throw std::runtime_error("Bind nieudany");
      } else {
        std::cout << "Bind na procie ok: " << UDP_PORT << std::endl;
      }
      struct timeval tv{};
      tv.tv_sec = 0;
      tv.tv_usec = 200000;
      ::setsockopt(sock_, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
      recv_thread_ = std::thread(&UdpBridge::ReceiveLoop, this);
    }
    ~UdpBridge() {
        running_ = false;
        ::close(sock_);
        recv_thread_.join();
    }

    void ReceiveLoop() {
      while(running_) {
        char buf[BUFFER_SIZE];
        sockaddr_in sender{};
        socklen_t sender_len = sizeof(sender);
        ssize_t n = ::recvfrom(sock_, buf, BUFFER_SIZE-1, 0,
           reinterpret_cast<sockaddr*>(&sender), &sender_len);
        if (n <= 0) continue;
        auto msg = esp32_bridge::msg::Imu{};
        memcpy(&msg.ax, buf + 0, 4);
        memcpy(&msg.ay, buf + 4, 4);
        memcpy(&msg.az, buf + 8, 4);
        memcpy(&msg.gx, buf + 12, 4);
        memcpy(&msg.gy, buf + 16, 4);
        memcpy(&msg.gz, buf + 20, 4);
        memcpy(&msg.t, buf + 24, 4);
        memcpy(&msg.ts, buf + 28, 8);
        publisher_->publish(msg);
      }
    }
};

int main(int argc, char ** argv) {
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<UdpBridge>());
  rclcpp::shutdown();
  return 0;
}