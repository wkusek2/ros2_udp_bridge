# Kompletna lekcja: Subscribery w ROS2

---

## 1. Czym jest Subscriber — głębiej

Subscriber to obiekt który rejestruje się w systemie ROS2 mówiąc: "chcę dostawać wiadomości z topicu X". Gdy Publisher wyśle wiadomość — ROS2 middleware (DDS) dostarcza ją do wszystkich subskrybentów tego topicu i wywołuje ich callbacki.

Ważne: **Publisher i Subscriber nigdy nie wiedzą o sobie nawzajem**. Komunikują się tylko przez nazwę topicu. Możesz mieć 0 publisherów i 5 subskrybentów — nic się nie posypie. Możesz mieć 3 publisherów i 1 subskrybenta — subskrybent dostanie wiadomości od wszystkich trzech.

---

## 2. Pełna składnia `create_subscription`

```cpp
subscription_ = this->create_subscription<TypWiadomości>(
    "/nazwa/topicu",           // string
    rozmiar_kolejki,           // int lub obiekt QoS
    callback,                  // funkcja/lambda/bind
    opcje_subskrypcji          // opcjonalne, rzadko używane
);
```

Typ zwracany to `rclcpp::Subscription<T>::SharedPtr` — smart pointer. Musisz go **trzymać jako pole klasy**. Jeśli zmienna lokalna wyjdzie z zakresu — subscription zostanie zniszczona i przestaniesz odbierać wiadomości. To częsty błąd początkujących.

---

## 3. Trzy sposoby przekazania callbacka

### std::bind (klasyczny)
```cpp
std::bind(&MojaKlasa::onData, this, std::placeholders::_1)
```
`_1` = pierwszy argument funkcji (wiadomość). Jeśli callback przyjmuje dwa argumenty — dodajesz `_2`.

### Lambda (nowoczesny, preferowany)
```cpp
[this](const esp32_bridge::msg::Imu & msg) {
    RCLCPP_INFO(get_logger(), "ax: %f", msg.ax);
}
```
Lambda to anonimowa funkcja. `[this]` = "przechwyć wskaźnik `this` żebym mógł używać pól klasy". Czytelniejsza niż `std::bind`.

### Lambda z auto (najkrótszy zapis)
```cpp
[this](const auto & msg) {
    RCLCPP_INFO(get_logger(), "ax: %f", msg.ax);
}
```
`auto` sam odgadnie typ wiadomości. Wygodne ale ukrywa typ — mniej czytelne gdy masz wiele topiców.

---

## 4. Sygnatura callbacka — warianty

### Standardowy (najczęstszy)
```cpp
void callback(const esp32_bridge::msg::Imu & msg)
```

### Z informacją o nadawcy (MessageInfo)
```cpp
void callback(
    const esp32_bridge::msg::Imu & msg,
    const rclcpp::MessageInfo & info)
```
`info` zawiera timestamp dostarczenia, QoS itp. Rzadko potrzebne.

### Z shared_ptr zamiast referencji
```cpp
void callback(esp32_bridge::msg::Imu::SharedPtr msg)
```
Używasz gdy chcesz zachować wiadomość dłużej — np. wrzucić do kolejki. Z referencją wiadomość może być zniszczona po zakończeniu callbacka.

---

## 5. QoS — Quality of Service (szczegółowo)

QoS to zestaw reguł jak wiadomości mają być dostarczane.

### Reliability (niezawodność)
```cpp
.reliable()      // gwarantowane dostarczenie, ponawia próby
.best_effort()   // szybkie, można stracić wiadomości
```
- `reliable` = jak TCP
- `best_effort` = jak UDP

Kiedy używać:
- `reliable` → komendy, stany, konfiguracja — nie możesz stracić
- `best_effort` → dane sensorów (IMU, kamera) — lepiej stracić starą klatkę niż czekać

### Durability (trwałość)
```cpp
.durability_volatile()       // nie zapamiętuj (domyślne)
.transient_local()           // zapamiętaj ostatnią wiadomość
```
`transient_local` = gdy nowy subskrybent dołączy, dostanie ostatnią wiadomość natychmiast. Przydatne dla konfiguracji i map.

### History (historia)
```cpp
.keep_last(10)   // trzymaj ostatnie N wiadomości (domyślne)
.keep_all()      // trzymaj wszystkie (uwaga na pamięć!)
```

### Deadline
```cpp
.deadline(std::chrono::milliseconds(100))
```
Jeśli wiadomość nie przyjdzie w ciągu 100ms — callback `on_deadline_missed` jest wywoływany. Monitoring czy sensor żyje.

### Pełny przykład QoS dla IMU
```cpp
auto qos = rclcpp::QoS(rclcpp::KeepLast(10))
    .best_effort()
    .durability_volatile();

subscription_ = create_subscription<esp32_bridge::msg::Imu>(
    "/esp32/data", qos, callback);
```

### Tabela kompatybilności QoS

| Publisher | Subscriber | Połączenie |
|-----------|------------|------------|
| reliable  | reliable   | ✅ |
| reliable  | best_effort | ✅ |
| best_effort | reliable | ❌ |
| best_effort | best_effort | ✅ |

Sprawdź czy QoS się zgadzają:
```bash
ros2 topic info /esp32/data --verbose
```

---

## 6. Wielowątkowość i callbacki

Domyślnie `rclcpp::spin()` używa **SingleThreadedExecutor** — callbacki są wywoływane jeden po drugim, nigdy równolegle. Bezpieczne ale wolne.

### MultiThreadedExecutor
```cpp
int main(int argc, char ** argv) {
    rclcpp::init(argc, argv);
    rclcpp::executors::MultiThreadedExecutor executor;
    executor.add_node(std::make_shared<MojNode>());
    executor.spin();
    rclcpp::shutdown();
}
```
Gdy callbacki mogą działać równolegle — zabezpiecz współdzielone dane przez `std::mutex`.

### CallbackGroup
```cpp
auto cb_group = create_callback_group(
    rclcpp::CallbackGroupType::MutuallyExclusive);
// MutuallyExclusive = nigdy równolegle
// Reentrant = mogą działać równolegle
```

---

## 7. Synchronizacja wielu topiców — message_filters

Gdy chcesz przetworzyć dane z dwóch sensorów które mają **ten sam timestamp**:

```cpp
#include <message_filters/subscriber.h>
#include <message_filters/time_synchronizer.h>

message_filters::Subscriber<sensor_msgs::msg::Image> sub_cam_;
message_filters::Subscriber<sensor_msgs::msg::PointCloud2> sub_lidar_;

using Sync = message_filters::TimeSynchronizer<
    sensor_msgs::msg::Image,
    sensor_msgs::msg::PointCloud2>;
std::shared_ptr<Sync> sync_;

// W konstruktorze:
sub_cam_.subscribe(this, "/camera");
sub_lidar_.subscribe(this, "/lidar");
sync_ = std::make_shared<Sync>(sub_cam_, sub_lidar_, 10);
sync_->registerCallback(&MojNode::callback, this);

// Callback dostaje oba jednocześnie:
void callback(
    const sensor_msgs::msg::Image & img,
    const sensor_msgs::msg::PointCloud2 & cloud) { ... }
```

`ApproximateTimeSynchronizer` — gdy timestampy nie muszą być identyczne, tylko bliskie:
```cpp
#include <message_filters/sync_policies/approximate_time.h>
```

---

## 8. Throttling — ograniczanie częstotliwości callbacka

```cpp
void callback(const esp32_bridge::msg::Imu & msg) {
    RCLCPP_INFO_THROTTLE(
        get_logger(),
        *get_clock(),
        1000,   // co ile ms (1000ms = 1 sekunda)
        "ax: %f", msg.ax);
}
```

---

## 9. Intraprocess communication

Gdy Publisher i Subscriber są w tym samym procesie — ROS2 może przekazać wskaźnik zamiast kopiować dane:

```cpp
rclcpp::NodeOptions options;
options.use_intra_process_comms(true);
```

Dla IMU przy 1kHz różnica jest odczuwalna.

---

## 10. Diagnostyka i debugowanie

```bash
# Sprawdź czy topic istnieje i kto go używa
ros2 topic info /esp32/data

# Szczegółowo z QoS
ros2 topic info /esp32/data --verbose

# Częstotliwość wiadomości
ros2 topic hz /esp32/data

# Przepustowość (bajty/s)
ros2 topic bw /esp32/data

# Opóźnienie (wymaga timestampa w wiadomości)
ros2 topic delay /esp32/data

# Podejrzyj jedną wiadomość
ros2 topic echo /esp32/data --once

# Podejrzyj N wiadomości
ros2 topic echo /esp32/data --times 5

# Lista node'ów i ich subskrypcji
ros2 node info /imu_subscriber
```

---

## 11. Typowe błędy i jak je naprawić

| Problem | Przyczyna | Rozwiązanie |
|---------|-----------|-------------|
| Callback nigdy nie wywoływany | zła nazwa topicu | `ros2 topic list` i porównaj |
| Callback nigdy nie wywoływany | niekompatybilny QoS | `ros2 topic info --verbose` |
| Callback nigdy nie wywoływany | brak `spin()` | dodaj `rclcpp::spin()` |
| Subscription przestaje działać | zmienna lokalna zniszczona | trzymaj jako pole klasy |
| Deadlock przy `spin()` | blokujące wywołanie w callbacku | przenieś do osobnego wątku |
| Stare wiadomości po restarcie | `transient_local` QoS | zmień na `volatile` |
| Subskrybent nie widzi publishera | różne namespace | sprawdź pełną nazwę topicu |

---

## 12. Wzorzec subskrybenta w seniorskim kodzie

```cpp
#include <rclcpp/rclcpp.hpp>
#include <esp32_bridge/msg/imu.hpp>
#include <mutex>

class ImuSubscriber : public rclcpp::Node
{
public:
  explicit ImuSubscriber(const rclcpp::NodeOptions & options = rclcpp::NodeOptions{})
  : Node("imu_subscriber", options)
  {
    auto qos = rclcpp::QoS(rclcpp::KeepLast(10)).best_effort();

    sub_ = create_subscription<esp32_bridge::msg::Imu>(
      "/esp32/data", qos,
      [this](esp32_bridge::msg::Imu::SharedPtr msg) {
        onImu(msg);
      });

    RCLCPP_INFO(get_logger(), "Subskrybent IMU uruchomiony");
  }

  esp32_bridge::msg::Imu getLastImu() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return last_imu_;
  }

private:
  void onImu(esp32_bridge::msg::Imu::SharedPtr msg) {
    {
      std::lock_guard<std::mutex> lock(mutex_);
      last_imu_ = *msg;
    }
    RCLCPP_DEBUG(get_logger(), "ax: %.3f ay: %.3f az: %.3f",
      msg->ax, msg->ay, msg->az);
  }

  rclcpp::Subscription<esp32_bridge::msg::Imu>::SharedPtr sub_;
  mutable std::mutex mutex_;
  esp32_bridge::msg::Imu last_imu_{};
};
```

### Co różni senior od juniora w tym kodzie:
- `explicit` przy konstruktorze — zapobiega niejawnemu tworzeniu obiektu
- `SharedPtr` zamiast referencji — możesz zachować wiadomość
- `mutex` — bezpieczny dostęp z wielu wątków
- `RCLCPP_DEBUG` zamiast `INFO` — logi tylko gdy włączony debug mode
- `NodeOptions` jako parametr — umożliwia composable nodes
