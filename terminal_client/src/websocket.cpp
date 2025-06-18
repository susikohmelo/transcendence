#include <main.hpp>

bool g_is_connected = false; // Used to tell if we are connected or not

using Client = websocketpp::client<websocketpp::config::asio_tls_client>;
using ConnectionHdl = websocketpp::connection_hdl;
using SslContext = websocketpp::lib::asio::ssl::context;
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;

void send_message(Client* client, ConnectionHdl* connection, std::string msg)
{
	try
	{ client->send(*connection, msg, websocketpp::frame::opcode::text);}
	catch(websocketpp::exception e)
	{
		std::cerr << "Error -> ";
		std::cerr << e.what() << std::endl;
	}
}

void send_api_message(nlohmann::json& j)
{
    j["user_id"]   = g_user_id;
    j["api_token"] = g_api_token;
    send_message(&g_client, &g_connection, j.dump());
}

void send_api_message(const std::string& type)
{
    json    j;
    j["type"] = type;
    send_api_message(j);
}

void close_connection(Client* client, ConnectionHdl* connection)
{
	try
	{
		client->close(*connection, websocketpp::close::status::normal, "done");
	}
	catch (...) {}
}

void on_message(Client* _, ConnectionHdl __,
		websocketpp::config::asio_client::message_type::ptr msg)
{
    (void)_; (void)__;
	std::string string_payload = msg->get_raw_payload();
	if (string_payload.empty())
		return ;

	json_handler(string_payload);
}

void on_open(Client* _, ConnectionHdl* connection, ConnectionHdl hdl)
{
    (void)_;
	*connection = hdl;
	g_is_connected = true;
	std::cout << "Websocket connected" << std::endl;
}

websocketpp::lib::shared_ptr<SslContext> on_tls_init()
{
	auto ctx = websocketpp::lib::make_shared<SslContext>(
			boost::asio::ssl::context::sslv23);
	return ctx;
}

void turn_off_logging(Client& client)
{
	client.clear_access_channels(websocketpp::log::alevel::all);
	client.clear_error_channels(websocketpp::log::elevel::all);
}

void set_message_handler(Client& client)
{
	client.set_message_handler(
		websocketpp::lib::bind(&on_message, &client, ::_1, ::_2));
}

void set_open_handler(Client& client, ConnectionHdl* connection)
{
	client.set_open_handler(
		websocketpp::lib::bind(&on_open, &client, connection, ::_1));
}

void set_tls_init_handler(Client& client)
{
	client.set_tls_init_handler(websocketpp::lib::bind(&on_tls_init));
}

void set_url(Client& client, std::string url)
{
	websocketpp::lib::error_code ec;
	auto connection = client.get_connection(url, ec);
	client.connect(connection);
}
