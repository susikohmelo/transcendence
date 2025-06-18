#pragma once

extern bool g_is_connected;

using Client = websocketpp::client<websocketpp::config::asio_tls_client>;
using ConnectionHdl = websocketpp::connection_hdl;
using SslContext = websocketpp::lib::asio::ssl::context;
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;

void json_handler(std::string &payload);	// In main.cpp

void send_message(Client* client, ConnectionHdl* connection, std::string msg);

void send_api_message(nlohmann::json&);
void send_api_message(const std::string& type);

void close_connection(Client* client, ConnectionHdl* connection);

void on_message(Client* client, ConnectionHdl hdl,
		websocketpp::config::asio_client::message_type::ptr msg);

void on_open(Client* client, ConnectionHdl* connection, ConnectionHdl hdl);

websocketpp::lib::shared_ptr<SslContext> on_tls_init();

void turn_off_logging(Client& client);

void set_message_handler(Client& client);

void set_open_handler(Client& client, ConnectionHdl* connection);

void set_tls_init_handler(Client& client);

void set_url(Client& client, std::string url);
