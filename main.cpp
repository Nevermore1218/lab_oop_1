#include "Domain.h"
#include "ConsoleUI.h"
#include <clocale>

int main() {
    setlocale(LC_ALL, "Russian");

    DeliveryService service;
    ConsoleUI ui(service);
    ui.run();

    return 0;
}