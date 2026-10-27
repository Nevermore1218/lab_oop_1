#include "ConsoleUI.h"
#include <iostream>
#include <iomanip>
#include <stdexcept>

ConsoleUI::ConsoleUI(DeliveryService& service) : service_(service) {}

int ConsoleUI::readInt(const std::string& prompt, int minVal, int maxVal) {
    while (true) {
        std::cout << prompt;
        std::string line;
        if (!std::getline(std::cin, line)) return 0;
        try {
            size_t pos;
            int val = std::stoi(line, &pos);
            if (pos == line.length() && val >= minVal && val <= maxVal) return val;
        } catch (...) {}
        std::cout << "  [Ошибка ввода] Введите целое число от " << minVal << " до " << maxVal << ".\n";
    }
}

double ConsoleUI::readDouble(const std::string& prompt, double minVal) {
    while (true) {
        std::cout << prompt;
        std::string line;
        if (!std::getline(std::cin, line)) return minVal;
        try {
            size_t pos;
            double val = std::stod(line, &pos);
            if (pos == line.length() && val >= minVal) return val;
        } catch (...) {}
        std::cout << "  [Ошибка ввода] Введите число (не меньше " << minVal << ").\n";
    }
}

std::string ConsoleUI::readNonEmptyString(const std::string& prompt) {
    while (true) {
        std::cout << prompt;
        std::string line;
        if (!std::getline(std::cin, line)) return "";
        size_t first = line.find_first_not_of(" \t\r\n");
        if (first != std::string::npos) {
            size_t last = line.find_last_not_of(" \t\r\n");
            return line.substr(first, (last - first + 1));
        }
        std::cout << "  [Ошибка ввода] Поле не может быть пустым.\n";
    }
}

void ConsoleUI::printOrderDetails(const Order& order) const {
    std::cout << "\n------------------------------------------------------------\n";
    std::cout << "ЗАКАЗ #" << order.getId() << " | Статус: [" << statusToString(order.getStatus()) << "]\n";
    std::cout << order.getCustomer().getRoleDescription() << "\n";
    std::cout << "Адрес доставки: " << order.getAddress() << "\n";
    std::cout << "Позиции заказа:\n";
    for (const auto& item : order.getItems()) {
        std::cout << "  * " << item.getName() << " — " << item.getQuantity() << " шт. x "
                  << std::fixed << std::setprecision(2) << item.getUnitPrice() << " руб. = "
                  << item.getTotalPrice() << " руб.\n";
    }
    std::cout << "Стоимость товаров: " << std::fixed << std::setprecision(2) << order.calculateItemsCost() << " руб.\n";

    if (const auto* method = order.getDeliveryMethod()) {
        std::cout << "Способ доставки:   " << method->getName() << "\n";
        std::cout << "Срок доставки:     " << method->getEstimatedTime() << "\n";
        std::cout << "Цена доставки:     " << order.calculateDeliveryCost() << " руб.\n";
    } else {
        std::cout << "Способ доставки:   [Не выбран]\n";
    }

    if (auto courier = order.getCourier()) {
        std::cout << "Назначен курьер:   " << courier->getName() << " (" << courier->getPhone() << ")\n";
    } else if (order.getDeliveryMethod() && order.getDeliveryMethod()->requiresCourier()) {
        std::cout << "Назначен курьер:   [Требуется назначение]\n";
    } else {
        std::cout << "Назначен курьер:   [Не требуется]\n";
    }

    std::cout << "ИТОГО К ОПЛАТЕ:    " << order.calculateTotalCost() << " руб.\n";
    std::cout << "------------------------------------------------------------\n";
}

void ConsoleUI::handleCreateOrder() {
    std::cout << "\n=== 1. Создание нового заказа ===\n";
    std::string name = readNonEmptyString("Введите имя клиента: ");
    std::string phone = readNonEmptyString("Введите телефон клиента: ");
    std::string address = readNonEmptyString("Введите адрес доставки (или адрес ПВЗ): ");

    Order& order = service_.createOrder(name, phone, address);

    int itemCount = readInt("Сколько позиций добавить в заказ? (1-20): ", 1, 20);
    for (int i = 1; i <= itemCount; ++i) {
        std::cout << "\n--- Позиция #" << i << " ---\n";
        std::string itemName = readNonEmptyString("Название товара: ");
        int qty = readInt("Количество (шт.): ", 1, 10000);
        double price = readDouble("Цена за единицу (руб.): ", 1.0);
        order.addItem(OrderItem(itemName, qty, price));
    }

    std::cout << "\n[Успешно] Заказ #" << order.getId() << " создан!\n";
    printOrderDetails(order);
}

void ConsoleUI::handleListOrders() const {
    std::cout << "\n=== 2. Список существующих заказов ===\n";
    const auto& orders = service_.getOrders();
    if (orders.empty()) {
        std::cout << "Список заказов пуст.\n";
        return;
    }
    for (const auto& order : orders) {
        printOrderDetails(order);
    }
}

void ConsoleUI::handleSelectDeliveryMethod() {
    std::cout << "\n=== 3. Выбор способа доставки ===\n";
    if (service_.getOrders().empty()) {
        std::cout << "Сначала создайте хотя бы один заказ.\n";
        return;
    }

    int orderId = readInt("Введите номер заказа: ", 1);
    Order& order = service_.findOrderById(orderId);

    std::cout << "\nДоступные способы доставки:\n";
    std::cout << "1. Стандартная доставка курьером (300 руб., бесплатно от 3000 руб. | 1-2 дня)\n";
    std::cout << "2. Экспресс-доставка курьером (600 руб. + 5% от суммы заказа | 1-3 часа)\n";
    std::cout << "3. Самовывоз из ПВЗ (0 руб., курьер не требуется | Сегодня)\n";

    int choice = readInt("Выберите способ доставки (1-3): ", 1, 3);
    std::unique_ptr<DeliveryMethod> method;
    if (choice == 1) method = std::make_unique<StandardDelivery>();
    else if (choice == 2) method = std::make_unique<ExpressDelivery>();
    else method = std::make_unique<PickupDelivery>();

    order.setDeliveryMethod(std::move(method));

    std::cout << "\n[Успешно] Способ доставки установлен!\n";
    std::cout << "  -> Выбранный способ: " << order.getDeliveryMethod()->getName() << "\n";
    std::cout << "  -> Ориентировочный срок: " << order.getDeliveryMethod()->getEstimatedTime() << "\n";
    std::cout << "  -> Стоимость доставки: " << std::fixed << std::setprecision(2) << order.calculateDeliveryCost() << " руб.\n";
    std::cout << "  -> Общая сумма заказа с доставкой: " << order.calculateTotalCost() << " руб.\n";
}

void ConsoleUI::handleAssignCourier() {
    std::cout << "\n=== 4. Назначение курьера на заказ ===\n";
    if (service_.getOrders().empty()) {
        std::cout << "Сначала создайте хотя бы один заказ.\n";
        return;
    }

    int orderId = readInt("Введите номер заказа: ", 1);
    Order& order = service_.findOrderById(orderId);

    if (!order.getDeliveryMethod()) {
        std::cout << "[Внимание] Сначала выберите способ доставки для этого заказа (пункт меню 3).\n";
        return;
    }
    if (!order.getDeliveryMethod()->requiresCourier()) {
        std::cout << "[Внимание] Способ доставки '" << order.getDeliveryMethod()->getName()
                  << "' не предусматривает назначение курьера.\n";
        return;
    }

    std::cout << "\nСписок курьеров службы:\n";
    for (const auto& courier : service_.getCouriers()) {
        std::cout << "  " << courier->getRoleDescription() << "\n";
    }

    int courierId = readInt("Введите ID курьера для назначения: ", 1);
    auto courier = service_.findCourierById(courierId);

    order.assignCourier(courier);
    std::cout << "\n[Успешно] Курьер " << courier->getName() << " назначен на заказ #" << order.getId() << "!\n";
}

void ConsoleUI::handleChangeStatus() {
    std::cout << "\n=== 5. Изменение статуса заказа ===\n";
    if (service_.getOrders().empty()) {
        std::cout << "Сначала создайте хотя бы один заказ.\n";
        return;
    }

    int orderId = readInt("Введите номер заказа: ", 1);
    Order& order = service_.findOrderById(orderId);

    std::cout << "Текущий статус заказа #" << order.getId() << ": [" << statusToString(order.getStatus()) << "]\n";
    std::cout << "Доступные целевые статусы:\n";
    std::cout << "1. В обработке (требуется выбранный способ доставки)\n";
    std::cout << "2. В пути / Готов к выдаче (для курьерской доставки требуется назначенный курьер)\n";
    std::cout << "3. Доставлен (завершает заказ и освобождает курьера)\n";
    std::cout << "4. Отменён (отменяет заказ и освобождает курьера)\n";

    int choice = readInt("Выберите новый статус (1-4): ", 1, 4);
    OrderStatus targetStatus;
    switch (choice) {
        case 1: targetStatus = OrderStatus::Processing; break;
        case 2: targetStatus = OrderStatus::InTransit;  break;
        case 3: targetStatus = OrderStatus::Delivered;  break;
        default: targetStatus = OrderStatus::Cancelled; break;
    }

    order.changeStatus(targetStatus);
    std::cout << "\n[Успешно] Статус заказа изменён!\n";
    printOrderDetails(order);
}

void ConsoleUI::run() {
    while (true) {
        std::cout << "\n============================================================\n";
        std::cout << "             СИМУЛЯТОР СЛУЖБЫ ДОСТАВКИ (МЕНЮ)               \n";
        std::cout << "============================================================\n";
        std::cout << "1. Создать новый заказ и добавить позиции\n";
        std::cout << "2. Показать список существующих заказов\n";
        std::cout << "3. Выбрать способ доставки для заказа (расчёт цены и срока)\n";
        std::cout << "4. Назначить курьера на заказ\n";
        std::cout << "5. Изменить статус заказа и показать информацию\n";
        std::cout << "0. Выход из программы\n";
        std::cout << "------------------------------------------------------------\n";

        int choice = readInt("Выберите действие (0-5): ", 0, 5);
        if (choice == 0) {
            std::cout << "Завершение работы программы. До свидания!\n";
            break;
        }

        try {
            switch (choice) {
                case 1: handleCreateOrder(); break;
                case 2: handleListOrders(); break;
                case 3: handleSelectDeliveryMethod(); break;
                case 4: handleAssignCourier(); break;
                case 5: handleChangeStatus(); break;
            }
        } catch (const std::exception& e) {
            std::cout << "\n[Бизнес-ошибка] Операция отклонена: " << e.what() << "\n";
        }
    }
}