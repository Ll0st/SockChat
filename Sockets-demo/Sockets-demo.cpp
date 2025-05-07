#include <iostream>
#include <string>
#include <thread>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

void HandleClient(SOCKET clientSocket);
void Server(int port);
void Send(std::string message, const std::string& serverAddress, int port);

int main() {
    const std::string IpAdressToSend = "127.0.0.1"; // Sa propre adresse. Équivalent à "localhost"
    const int port = 8080; // Le port à utiliser pour communiquer

    std::thread serverThread(Server, port); // Le serveur écoute activement et est donc blocant.
                                            //On le met dans un thread pour pouvoir continuer à faire autre chose
    do
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // On laisse le temps au serveur de s'initialiser et d'écouter avant de lui envoyer un message
	    std::cout << "Message : "; std::string message; std::cin >> message;
		if (message == "exit") return 0; // On sort de la boucle si le message est "exit"
        Send(message, IpAdressToSend, port); // On envoi le message "Hello world" au serveur, qui va l'afficher dans la console à la réception
    } while (true);

    serverThread.join(); // On attends que le serveur arrête avant de fermer le programme. Ici ça n'arrivera jamais vu qu'il écoute indéfiniment
    return 0;
}

void HandleClient(SOCKET clientSocket) {
    char buffer[1024];
    while (true) {
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesReceived == SOCKET_ERROR) {
            std::cerr << "Erreur lors de la reception des donnees: " << WSAGetLastError() << std::endl;
            closesocket(clientSocket);
            return;
        }
        else if (bytesReceived == 0) {
            std::cout << "Client deconnecte" << std::endl;
            closesocket(clientSocket);
            return;
        }
        std::cout << "Voici ce que nous avons recu via le reseau : " << std::string(buffer, bytesReceived) << std::endl;
    }
}

void Server(int port) {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Erreur lors de l'initialisation des sockets" << std::endl;
        return;
    }

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == INVALID_SOCKET) {
        std::cerr << "Erreur lors de la creation du socket: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(port);

    if (bind(serverSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Erreur lors du bind: " << WSAGetLastError() << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return;
    }

    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Erreur lors de l'ecoute: " << WSAGetLastError() << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return;
    }

    while (true) {
        SOCKET clientSocket = accept(serverSocket, nullptr, nullptr);
        if (clientSocket == INVALID_SOCKET) {
            std::cerr << "Erreur lors de l'acceptation de la connexion: " << WSAGetLastError() << std::endl;
            continue;
        }

        std::thread clientThread(HandleClient, clientSocket);
        clientThread.detach();
    }

    closesocket(serverSocket);
    WSACleanup();
}

void Send(std::string message, const std::string& serverAddress, int port) {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Erreur lors de l'initialisation des sockets" << std::endl;
        return;
    }

    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clientSocket == INVALID_SOCKET) {
        std::cerr << "Erreur lors de la creation du socket: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);

    if (inet_pton(AF_INET, serverAddress.c_str(), &serverAddr.sin_addr) != 1) {
        std::cerr << "Adresse invalide" << std::endl;
        closesocket(clientSocket);
        WSACleanup();
        return;
    }

    if (connect(clientSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Erreur lors de la connexion au serveur: " << WSAGetLastError() << std::endl;
        closesocket(clientSocket);
        WSACleanup();
        return;
    }

    std::cout << "Connecte au serveur" << std::endl;

    char buffer[1024];
    if (send(clientSocket, message.c_str(), message.length(), 0) == SOCKET_ERROR) {
        std::cerr << "Erreur lors de l'envoi des donnees: " << WSAGetLastError() << std::endl;
        closesocket(clientSocket);
        WSACleanup();
        return;
    }

    closesocket(clientSocket);
    WSACleanup();
}