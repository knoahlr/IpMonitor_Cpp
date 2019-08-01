#include "sessionsManager.h"


Session::Session()

{

	clientConfiguration();

}

void Session::clientConfiguration() 
{
	Aws::Client::ClientConfiguration clientConfig;

	clientConfig.region = Region;
	dynamoDbClient = new Aws::DynamoDB::DynamoDBClient(clientConfig);
}


void Session::addIp()
{
	pir.SetTableName(_tableName);

	//Obtain current IP and Epoch Date

	Aws::String IP, DATE;
	std::tie(DATE, IP) = getIPandDate();

	setAttValue.SetS(IP);
	pir.AddItem(_ipAttribute, setAttValue);

	setAttValue.SetS(DATE);	
	pir.AddItem(_dateAttribute, setAttValue);

	putResult = dynamoDbClient->PutItem(pir);
	
	if (!putResult.IsSuccess())
	{
		std::cout << putResult.GetError() << std::endl;
	}
	
}

void Session::getTables()
{
	do
	{
		ltr.SetLimit(50);
		lto = dynamoDbClient->ListTables(ltr);
		if (!lto.IsSuccess())
		{
			std::cout << "Error: " << lto.GetError() << std::endl;
			return;
		}
		for (const auto& s : lto.GetResult().GetTableNames())
			std::cout << s << std::endl;
		ltr.SetExclusiveStartTableName(lto.GetResult().GetLastEvaluatedTableName());
	} while (!ltr.GetExclusiveStartTableName().empty());

}

void Session::getItems()
{
	queryReq.SetTableName(_tableName);
	queryReq.SetKeyConditionExpression("partitionKeyName=:Ip");
	qoc = dynamoDbClient->Query(queryReq);

	if (qoc.IsSuccess()) 
	{
		for (const auto& item : qoc.GetResult().GetItems())
		{
			for (const auto& i : item)
			{
				std::cout << i.first << i.second.GetS() << std::endl;

			}
		}
	}

}

std::tuple <Aws::String, Aws::String> Session::getIPandDate()
{
	using namespace std::chrono;
	milliseconds ms = duration_cast<milliseconds>(system_clock::now().time_since_epoch());
	std::string timeSinceEpoch = std::to_string(ms.count());
	Aws::String publicIP = queryIP();

	return std::make_tuple(Aws::String(timeSinceEpoch), publicIP);

}

Aws::String Session::queryIP()
{
	std::string ipString;
	boost::asio::io_service io_service;
	std::string const address = "ident.me";
	boost::asio::streambuf request;
	boost::asio::streambuf response;
	std::string req = "GET / HTTP/1.1\r\nHost: " + address + "\r\n\r\n";

	try {


		// Get a list of endpoints corresponding to the server name.
		tcp::resolver resolver(io_service);
		tcp::resolver::query query(address, "80", boost::asio::ip::resolver_query_base::numeric_service);
		tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
		std::cout << "Print Query --" << std::endl;

		// Try each endpoint until we successfully establish a connection.
		tcp::socket socket(io_service);
		boost::asio::connect(socket, endpoint_iterator);
	

		// Send the request.
		boost::asio::write(socket, boost::asio::buffer(req));
		boost::asio::read_until(socket, response, "\r\n");


		// Check that response is OK.
		std::istream response_stream(&response);
		std::string http_version, status_message;
		unsigned int status_code;
		std::getline(response_stream >> http_version >> status_code, status_message);

		if (!response_stream || http_version.substr(0, 5) != "HTTP/") {
			std::cout << "Invalid response\n";
			throw;
		}
		if (status_code != 200) {
			std::cout << "Response returned with status code " << status_code << "\n";
			throw;
		}
		// Read the response headers, which are terminated by a blank line.
		boost::asio::read_until(socket, response, "\r\n\r\n");
		
		// Process the response headers.
		std::string header;
		while (std::getline(response_stream, header) && header != "\r") std::cout << header << "\n";
		std::cout << "\n";
		
		//Remaining output should be IP Address
		char delim = '\r\n';
		std::string payloadString;
		for (std::string line; std::getline(response_stream, line, delim);)
		{
			payloadString += line;
		}
		std::cout << "\n";

		//Obtaining IP from response stream using regex.
		const char* pattern = "(\\d{1,3}(\\.\\d{1,3}){3})";
		boost::regex ip_regex(pattern);
		boost::sregex_iterator end;
		boost::sregex_iterator it(payloadString.begin(), payloadString.end(), ip_regex);
		for (; it != end; it++) ipString += it->str();

	}
	catch (std::exception &e) {

		std::cout << "Exception: " << e.what() << "\n";

	}


	return Aws::String(ipString);

}