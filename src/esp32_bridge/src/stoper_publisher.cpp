#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/string.hpp>

class CzasPub : public rclcpp::Node {
    private:
        rclcpp::Publisher<std_msgs::msg::String>::SharedPtr czaspub_;
        rclcpp::TimerBase::SharedPtr timer_;

    public:
        CzasPub() : Node("CzasPub") {
            RCLCPP_INFO(this->get_logger(), "Czas publikowany");
            czaspub_ = this->create_publisher<std_msgs::msg::String>("/czas", 10);
            timer_ = create_wall_timer(std::chrono::seconds(1),[this]{publisher();});

        }

        
        void publisher(){

            auto msg =  std_msgs::msg::String{};
            msg.data = "Czas: " + std::to_string(this->now().seconds()) + " ms";
            czaspub_->publish(msg);
            
        }

};

int main(int argc, char ** argv) {
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<CzasPub>());
    rclcpp::shutdown();
    return 0;
}