#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <lib/doctest.h>
#include <json/json.hpp>

#include <string_view>

TEST_CASE("Sanity Check") { CHECK(1 + 1 == 2); }

TEST_CASE("JSON tokenizing") {
	std::string		json_str = R"({"key": "value", "number": 123, "array": [1, 2, 3], "object": {"nested": "value"}})";
	TokenizedString tokens	 = tokenize(json_str);
	CHECK(tokens.size() > 0);
	std::cout << "Tokens: ";
	for (const auto &token : tokens) {
		if (token == String) {
			std::cout << "String(" << *reinterpret_cast<std::string *>(token.data) << ")";
		} else if (token == Number) {
			std::cout << "Number(" << *reinterpret_cast<const double *>(&token.data) << ")";
		} else if (token == Boolean) {
			std::cout << "Boolean(" << (*reinterpret_cast<const bool *>(&token.data) ? "true" : "false") << ")";
		} else if (token == Null) {
			std::cout << "Null";
		} else if (token == Object) {
			std::cout << "Object";
		} else if (token == Array) {
			std::cout << "Array";
		} else std::cout << token;
	}
	std::cout << std::endl;
}

TEST_CASE("JSON parsing") {
	JSONParser	parser;
	std::string json_str  = R"({"key": "value", "number": 123, "array": [1, 2, 3], "object": {"nested": "value"}})";
	auto		tokenized = tokenize(json_str);
	auto		result	  = parser.parse(tokenized.get());
	CHECK(result);
	std::cout << "Parsed JSON: " << result << std::endl;

	auto json = parseTreeToJSON(result);

	CHECK(json != nullptr);
	json->print(std::cout);
	std::cout << std::endl;

	CHECK(dynamic_cast<JSONObject *>(json.get()) != nullptr);
	auto &jo = *dynamic_cast<JSONObject *>(json.get());
	CHECK(jo.properties.size() == 4);
	CHECK_THROWS(jo["adddawda"]);
	CHECK(jo["key"].getType() == JSONType::String);

	using namespace std::literals::string_view_literals;
	CHECK(jo["key"].as<JSONString>() == "value"sv);

	CHECK(jo["number"].getType() == JSONType::Number);
	CHECK(jo["number"].as<JSONNumber>() == doctest::Approx(123.0).epsilon(0.001));

	CHECK(jo["array"].getType() == JSONType::Array);
	CHECK(jo["array"].as<JSONArray>().elements.size() == doctest::Approx(3.0).epsilon(0.001));
	CHECK(jo["array"].as<JSONArray>()[0].getType() == JSONType::Number);
	CHECK(jo["array"].as<JSONArray>()[0].as<JSONNumber>() == doctest::Approx(1.0).epsilon(0.001));
	CHECK(jo["array"].as<JSONArray>()[1].getType() == JSONType::Number);
	CHECK(jo["array"].as<JSONArray>()[1].as<JSONNumber>() == doctest::Approx(2.0).epsilon(0.001));
	CHECK(jo["array"].as<JSONArray>()[2].getType() == JSONType::Number);
	CHECK(jo["array"].as<JSONArray>()[2].as<JSONNumber>() == doctest::Approx(3.0).epsilon(0.001));
}
