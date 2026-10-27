#include "Domain.h"
#include <stdexcept>

// --- Реализация Person, Customer, Courier ---
Person::Person(int id, std::string name, std::string phone)
    : id_(id), name_(std::move(name)), phone_(std::move(phone)) {
    if (name_.empty()) throw std::invalid_argument("Имя не может быть пустым.");
    if (phone_.empty()) throw std::invalid_argument("Телефон не может быть пустым.");
}

Customer::Customer(int id, std::string name, std::string phone)
    : Person(id, std::move(name), std::move(phone)) {}

std::string Customer::getRoleDescription() const {
    return "Клиент: " + name_ + " (Тел: " + phone_ + ")";
}

Courier::Courier(int id, std::string name, std::string phone)
    : Person(id, std::move(name), std::move(phone)), isAvailable_(true) {}

std::string Courier::getRoleDescription() const {
    return "Курьер #" + std::to_string(id_) + ": " + name_ +
           " [" + (isAvailable_ ? "Свободен" : "Занят") + "]";
}

// --- Реализация OrderItem ---
OrderItem::OrderItem(std::string name, int quantity, double unitPrice)
    : name_(std::move(name)), quantity_(quantity), unitPrice_(unitPrice) {
    if (name_.empty()) throw std::invalid_argument("Название товара не может быть пустым.");
    if (quantity_ <= 0) throw std::invalid_argument("Количество товара должно быть больше нуля.");
    if (unitPrice_ < 0.0) throw std::invalid_argument("Цена товара не может быть отрицательной.");
}

// --- Реализация способов доставки ---
std::string StandardDelivery::getName() const { return "Стандартная доставка курьером"; }
double StandardDelivery::calculateCost(double itemsTotal) const { return (itemsTotal >= 3000.0) ? 0.0 : 300.0; }
std::string StandardDelivery::getEstimatedTime() const { return "1-2 рабочих дня"; }
bool StandardDelivery::requiresCourier() const { return true; }

std::string ExpressDelivery::getName() const { return "Экспресс-доставка курьером"; }
double ExpressDelivery::calculateCost(double itemsTotal) const { return 600.0 + (itemsTotal * 0.05); }
std::string ExpressDelivery::getEstimatedTime() const { return "1-3 часа"; }
bool ExpressDelivery::requiresCourier() const { return true; }

std::string PickupDelivery::getName() const { return "Самовывоз из ПВЗ"; }
double PickupDelivery::calculateCost(double /*itemsTotal*/) const { return 0.0; }
std::string PickupDelivery::getEstimatedTime() const { return "Сегодня (готовность через 30 минут)"; }
bool PickupDelivery::requiresCourier() const { return false; }

// --- Реализация Order ---
std::string statusToString(OrderStatus status) {
    switch (status) {
        case OrderStatus::Created:    return "Создан";
        case OrderStatus::Processing: return "В обработке";
        case OrderStatus::InTransit:  return "В пути / Готов к выдаче";
        case OrderStatus::Delivered:  return "Доставлен";
        case OrderStatus::Cancelled:  return "Отменён";
    }
    return "Неизвестно";
}

Order::Order(int id, Customer customer, std::string address)
    : id_(id), customer_(std::move(customer)), address_(std::move(address)),
      deliveryMethod_(nullptr), courier_(nullptr), status_(OrderStatus::Created) {
    if (address_.empty()) throw std::invalid_argument("Адрес доставки не может быть пустым.");
}

void Order::addItem(const OrderItem& item) {
    if (status_ != OrderStatus::Created) {
        throw std::logic_error("Добавлять товары можно только в заказ со статусом 'Создан'.");
    }
    items_.push_back(item);
}

void Order::setDeliveryMethod(std::unique_ptr<DeliveryMethod> method) {
    if (status_ == OrderStatus::InTransit || status_ == OrderStatus::Delivered || status_ == OrderStatus::Cancelled) {
        throw std::logic_error("Нельзя изменить способ доставки для отправленного или завершённого заказа.");
    }
    if (!method) throw std::invalid_argument("Способ доставки не может быть пустым.");

    if (!method->requiresCourier() && courier_ != nullptr) {
        courier_->setAvailable(true);
        courier_.reset();
    }
    deliveryMethod_ = std::move(method);
}

void Order::assignCourier(std::shared_ptr<Courier> courier) {
    if (!deliveryMethod_) throw std::logic_error("Сначала необходимо выбрать способ доставки для заказа.");
    if (!deliveryMethod_->requiresCourier()) {
        throw std::logic_error("Выбранный способ доставки (" + deliveryMethod_->getName() + ") не требует курьера.");
    }
    if (status_ == OrderStatus::Delivered || status_ == OrderStatus::Cancelled) {
        throw std::logic_error("Нельзя назначить курьера на завершённый или отменённый заказ.");
    }
    if (!courier) throw std::invalid_argument("Передан пустой указатель на курьера.");
    if (!courier->isAvailable()) {
        throw std::logic_error("Курьер " + courier->getName() + " сейчас занят другим заказом.");
    }

    if (courier_) courier_->setAvailable(true);
    courier_ = std::move(courier);
    courier_->setAvailable(false);
}

double Order::calculateItemsCost() const {
    double total = 0.0;
    for (const auto& item : items_) total += item.getTotalPrice();
    return total;
}

double Order::calculateDeliveryCost() const {
    return deliveryMethod_ ? deliveryMethod_->calculateCost(calculateItemsCost()) : 0.0;
}

double Order::calculateTotalCost() const {
    return calculateItemsCost() + calculateDeliveryCost();
}

void Order::changeStatus(OrderStatus newStatus) {
    if (status_ == newStatus) {
        throw std::logic_error("Заказ уже находится в статусе '" + statusToString(newStatus) + "'.");
    }
    if (status_ == OrderStatus::Delivered || status_ == OrderStatus::Cancelled) {
        throw std::logic_error("Заказ завершён (" + statusToString(status_) + "). Изменение статуса невозможно.");
    }

    if (newStatus == OrderStatus::Cancelled) {
        status_ = OrderStatus::Cancelled;
        if (courier_) courier_->setAvailable(true);
        return;
    }

    switch (status_) {
        case OrderStatus::Created:
            if (newStatus != OrderStatus::Processing)
                throw std::logic_error("Из статуса 'Создан' заказ можно перевести только в 'В обработке' (или 'Отменён').");
            if (items_.empty())
                throw std::logic_error("Нельзя перевести в обработку пустой заказ без товаров.");
            if (!deliveryMethod_)
                throw std::logic_error("Перед переводом в обработку необходимо выбрать способ доставки.");
            break;

        case OrderStatus::Processing:
            if (newStatus != OrderStatus::InTransit)
                throw std::logic_error("Из статуса 'В обработке' заказ можно перевести только в 'В пути / Готов к выдаче' (или 'Отменён').");
            if (deliveryMethod_->requiresCourier() && !courier_)
                throw std::logic_error("Для способа '" + deliveryMethod_->getName() + "' необходимо назначить курьера перед отправкой.");
            break;

        case OrderStatus::InTransit:
            if (newStatus != OrderStatus::Delivered)
                throw std::logic_error("Из статуса 'В пути' заказ можно перевести только в 'Доставлен' (или 'Отменён').");
            if (courier_) courier_->setAvailable(true);
            break;

        default:
            break;
    }
    status_ = newStatus;
}

// --- Реализация DeliveryService ---
DeliveryService::DeliveryService() {
    couriers_.push_back(std::make_shared<Courier>(1, "Иван Быстров", "+7 (900) 111-22-33"));
    couriers_.push_back(std::make_shared<Courier>(2, "Алексей Скороходов", "+7 (900) 444-55-66"));
    couriers_.push_back(std::make_shared<Courier>(3, "Мария Ветрова", "+7 (900) 777-88-99"));
}

Order& DeliveryService::createOrder(const std::string& customerName, const std::string& customerPhone, const std::string& address) {
    Customer customer(nextCustomerId_++, customerName, customerPhone);
    orders_.emplace_back(nextOrderId_++, std::move(customer), address);
    return orders_.back();
}

Order& DeliveryService::findOrderById(int orderId) {
    for (auto& order : orders_) {
        if (order.getId() == orderId) return order;
    }
    throw std::invalid_argument("Заказ с номером #" + std::to_string(orderId) + " не найден.");
}

std::shared_ptr<Courier> DeliveryService::findCourierById(int courierId) {
    for (const auto& courier : couriers_) {
        if (courier->getId() == courierId) return courier;
    }
    throw std::invalid_argument("Курьер с ID #" + std::to_string(courierId) + " не найден.");
}