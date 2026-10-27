#pragma once

#include <string>
#include <vector>
#include <memory>

// --- Участники ---
class Person {
protected:
    int id_;
    std::string name_;
    std::string phone_;

public:
    Person(int id, std::string name, std::string phone);
    virtual ~Person() = default;

    int getId() const { return id_; }
    const std::string& getName() const { return name_; }
    const std::string& getPhone() const { return phone_; }

    virtual std::string getRoleDescription() const = 0;
};

class Customer : public Person {
public:
    Customer(int id, std::string name, std::string phone);
    std::string getRoleDescription() const override;
};

class Courier : public Person {
private:
    bool isAvailable_;

public:
    Courier(int id, std::string name, std::string phone);
    bool isAvailable() const { return isAvailable_; }
    void setAvailable(bool available) { isAvailable_ = available; }
    std::string getRoleDescription() const override;
};

// --- Позиция заказа ---
class OrderItem {
private:
    std::string name_;
    int quantity_;
    double unitPrice_;

public:
    OrderItem(std::string name, int quantity, double unitPrice);

    const std::string& getName() const { return name_; }
    int getQuantity() const { return quantity_; }
    double getUnitPrice() const { return unitPrice_; }
    double getTotalPrice() const { return quantity_ * unitPrice_; }
};

// --- Способы доставки ---
class DeliveryMethod {
public:
    virtual ~DeliveryMethod() = default;
    virtual std::string getName() const = 0;
    virtual double calculateCost(double itemsTotal) const = 0;
    virtual std::string getEstimatedTime() const = 0;
    virtual bool requiresCourier() const = 0;
};

class StandardDelivery : public DeliveryMethod {
public:
    std::string getName() const override;
    double calculateCost(double itemsTotal) const override;
    std::string getEstimatedTime() const override;
    bool requiresCourier() const override;
};

class ExpressDelivery : public DeliveryMethod {
public:
    std::string getName() const override;
    double calculateCost(double itemsTotal) const override;
    std::string getEstimatedTime() const override;
    bool requiresCourier() const override;
};

class PickupDelivery : public DeliveryMethod {
public:
    std::string getName() const override;
    double calculateCost(double itemsTotal) const override;
    std::string getEstimatedTime() const override;
    bool requiresCourier() const override;
};

// --- Заказ и Статусы ---
enum class OrderStatus {
    Created,
    Processing,
    InTransit,
    Delivered,
    Cancelled
};

std::string statusToString(OrderStatus status);

class Order {
private:
    int id_;
    Customer customer_;
    std::string address_;
    std::vector<OrderItem> items_;
    std::unique_ptr<DeliveryMethod> deliveryMethod_;
    std::shared_ptr<Courier> courier_;
    OrderStatus status_;

public:
    Order(int id, Customer customer, std::string address);

    int getId() const { return id_; }
    const Customer& getCustomer() const { return customer_; }
    const std::string& getAddress() const { return address_; }
    const std::vector<OrderItem>& getItems() const { return items_; }
    const DeliveryMethod* getDeliveryMethod() const { return deliveryMethod_.get(); }
    std::shared_ptr<Courier> getCourier() const { return courier_; }
    OrderStatus getStatus() const { return status_; }

    void addItem(const OrderItem& item);
    void setDeliveryMethod(std::unique_ptr<DeliveryMethod> method);
    void assignCourier(std::shared_ptr<Courier> courier);

    double calculateItemsCost() const;
    double calculateDeliveryCost() const;
    double calculateTotalCost() const;

    void changeStatus(OrderStatus newStatus);
};

// --- Сервис управления заказами ---
class DeliveryService {
private:
    std::vector<Order> orders_;
    std::vector<std::shared_ptr<Courier>> couriers_;
    int nextOrderId_ = 1;
    int nextCustomerId_ = 1;

public:
    DeliveryService();

    Order& createOrder(const std::string& customerName, const std::string& customerPhone, const std::string& address);
    const std::vector<Order>& getOrders() const { return orders_; }
    Order& findOrderById(int orderId);

    const std::vector<std::shared_ptr<Courier>>& getCouriers() const { return couriers_; }
    std::shared_ptr<Courier> findCourierById(int courierId);
};