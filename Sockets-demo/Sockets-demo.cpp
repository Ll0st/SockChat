#include <iostream>
#include <string>
#include <thread>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

void HandleClient(SOCKET clientSocket);
void Server(int port);
void Send(std::string message, const std::string& serverAddress, int port);
int GetLocalIPAddressToDefinePort_CustomCode(std::string IpAdressToSend);
void MessageReceive(const std::string& text);

std::string UserName;

int main() {
	std::cout << "Enter your username: "; std::cin >> UserName;
    std::string IpAdressToSend; // Sa propre adresse. Équivalent à "localhost"
	std::cout << "IPv4 address of the receiver : "; std::cin >> IpAdressToSend;
    const int port = GetLocalIPAddressToDefinePort_CustomCode(IpAdressToSend); // Le port à utiliser pour communiquer

    system("cls"); // Clears the console

    std::thread serverThread(Server, port); // Le serveur écoute activement et est donc blocant.
                                            //On le met dans un thread pour pouvoir continuer à faire autre chose
    do
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // On laisse le temps au serveur de s'initialiser et d'écouter avant de lui envoyer un message
        std::cout << "[" << UserName << "] "; std::string _message; std::cin >> _message;
		if (_message == ".exit") return 0; // On sort de la boucle si le message est "exit"
		_message = _message + " [" + UserName + "]"; // On ajoute le nom d'utilisateur au message
        Send(_message, IpAdressToSend, port); // On envoi le message "Hello world" au serveur, qui va l'afficher dans la console à la réception
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
            closesocket(clientSocket);
            return;
        }
        MessageReceive(std::string(buffer, bytesReceived));
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



int GetLocalIPAddressToDefinePort_CustomCode(std::string IpAdressToSend) {
    if (IpAdressToSend != "127.0.0.1")
    {
        WSADATA wsaData;
        char hostname[256];

        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            std::cout << "WSAStartup failed" << std::endl;
            return 7;
        }

        if (gethostname(hostname, sizeof(hostname)) == SOCKET_ERROR) {
            WSACleanup();
            std::cout << "Error getting hostname" << std::endl;
            return 7;
        }

        struct addrinfo hints = {};
        struct addrinfo* result = nullptr;

        hints.ai_family = AF_INET; // IPv4
        hints.ai_socktype = SOCK_STREAM; // Stream socket
        hints.ai_protocol = IPPROTO_TCP; // TCP protocol

        if (getaddrinfo(hostname, nullptr, &hints, &result) != 0) {
            WSACleanup();
            std::cout << "Error getting address info" << std::endl;
            return 7;
        }

        struct sockaddr_in* addr = reinterpret_cast<struct sockaddr_in*>(result->ai_addr);
        char ip[INET_ADDRSTRLEN]; // Buffer to store the IP address as a string

        // Use inet_ntop instead of inet_ntoa
        if (inet_ntop(AF_INET, &(addr->sin_addr), ip, INET_ADDRSTRLEN) == nullptr) {
            freeaddrinfo(result);
            WSACleanup();
            std::cout << "Error converting IP address" << std::endl;
            return 7;
        }

        std::string ipStr(ip);

        freeaddrinfo(result);
        WSACleanup();

        if (ipStr > IpAdressToSend) {
            return 8081;
        }
    }
    return 8080;
}

void MessageReceive(const std::string& text) {
    // Obtenir la taille de la console
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    int consoleWidth = 80; // Valeur par défaut

    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    if (GetConsoleScreenBufferInfo(hConsole, &csbi)) {
        consoleWidth = csbi.dwSize.X;
    }

    // Calculer l'indentation pour aligner à droite
    int padding = consoleWidth - static_cast<int>(text.length());
    if (padding < 0) padding = 0;

    // Appliquer la couleur (vert clair)
    SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN | FOREGROUND_INTENSITY);

    // Afficher l'espace vide puis le texte
    std::cout << std::string(padding, ' ') << text << std::endl;

    // Réinitialiser la couleur
    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
}