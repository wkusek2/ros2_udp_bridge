#include <rclcpp/rclcpp.hpp>
#include <esp32_bridge/msg/imu.hpp>


class ImuSubscriber : public rclcpp::Node {
    private:
        rclcpp::Subscription<esp32_bridge::msg::Imu>::SharedPtr subscription_;
    public:

        void callback(const esp32_bridge::msg::Imu & msg){
            RCLCPP_INFO(this->get_logger(), "ax: %f ay: %f az: %f", msg.ax, msg.ay, msg.az);
        }

        ImuSubscriber() : Node("Imu_subscriber") {
            RCLCPP_INFO(this->get_logger(), "Subskrajber uruchomiony");
            subscription_ = this->create_subscription<esp32_bridge::msg::Imu>
            ("esp32/data", 10,[this](const esp32_bridge::msg::Imu & msg){
                callback(msg);
            });
        }

};

int main(int argc, char ** argv) {
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<ImuSubscriber>());
    rclcpp::shutdown();
    return 0;
}