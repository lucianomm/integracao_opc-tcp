#include "socket_client.h"
#include <thread>

int main() {
	std::thread socket_client_t(socket_client);
}