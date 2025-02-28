#include <boost/asio.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include "jumanpp/jumanpp.h"  // Juman++ API (adjust according to actual header location)
#include "deltalog.hh"   // Assumed external index system

using boost::asio::ip::tcp;

// Inverted index instance
Container c;

class TCPServer {
private:
    boost::asio::io_context io_context;
    tcp::acceptor acceptor;

public:
    TCPServer(short port)
        : acceptor(io_context, tcp::endpoint(tcp::v4(), port)) {
        startAccept();
    }

    void startAccept() {
        auto socket = std::make_shared<tcp::socket>(io_context);
        acceptor.async_accept(*socket,
            [this, socket](const boost::system::error_code& error) {
                if (!error) {
                    handleClient(socket);
                }
                startAccept();  // Continue accepting next connections
            });
    }

    void handleClient(std::shared_ptr<tcp::socket> socket) {
        auto buffer = std::make_shared<boost::asio::streambuf>();
        boost::asio::async_read_until(*socket, *buffer, '\n',
            [this, socket, buffer](const boost::system::error_code& error, std::size_t length) {
                if (!error) {
                    std::istream stream(buffer.get());
                    std::string command;
                    std::getline(stream, command);
                    processCommand(command);
                }
            });
    }

    void processCommand(const std::string& command) {
        std::istringstream iss(command);
        std::string action, documentID, text;

        iss >> action >> documentID;
        std::getline(iss, text);

        if (action == "INDEX") {
            processIndexing(documentID, text);
        }
    }

    void processIndexing(const std::string& documentID, const std::string& text) {
        std::vector<int> wordIDs = segmentText(text);

        for (int id : wordIDs) {
            c.add(documentID, id);
        }
    }

    std::vector<int> segmentText(const std::string& text) {
        std::vector<int> wordIDs;

        jumanpp::Jumanpp juman;
        if (!juman.initialize()) {
            std::cerr << "Error initializing Juman++" << std::endl;
            return wordIDs;
        }

        jumanpp::AnalysisResult result;
        if (!juman.analyze(text, &result)) {
            std::cerr << "Juman++ analysis failed" << std::endl;
            return wordIDs;
        }

        for (const auto& token : result.tokens()) {
            wordIDs.push_back(token.id());
        }

        return wordIDs;
    }

    void run() {
        io_context.run();
    }
};

int main() {
    try {
        TCPServer server(8080);
        server.run();
    } catch (std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    return 0;
}
