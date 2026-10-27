#pragma once

#include "Domain.h"
#include <string>
#include <limits>

class ConsoleUI {
private:
    DeliveryService& service_;

    int readInt(const std::string& prompt, int minVal = std::numeric_limits<int>::min(), int maxVal = std::numeric_limits<int>::max());
    double readDouble(const std::string& prompt, double minVal = 0.01);
    std::string readNonEmptyString(const std::string& prompt);

    void printOrderDetails(const Order& order) const;

    void handleCreateOrder();
    void handleListOrders() const;
    void handleSelectDeliveryMethod();
    void handleAssignCourier();
    void handleChangeStatus();

public:
    explicit ConsoleUI(DeliveryService& service);
    void run();
};