#include "app/Application.hpp"

int main() {
    const arduino3d::Application application;

    return application.run() == 0 ? 0 : 1;
}
