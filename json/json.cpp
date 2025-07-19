#include <json/json.hpp>
#include <fstream>

Token String		= Token::createToken("String");
Token Number		= Token::createToken("Number");
Token Boolean		= Token::createToken("Boolean");
Token Null			= Token::createToken("Null");
Token Object		= Token::createToken("Object");
Token Array			= Token::createToken("Array");
Token Value			= Token::createToken("Value");
Token PropertyList	= Token::createToken("PropertyList");
Token PropertyList_ = Token::createToken("PropertyList'");
Token ArrayList		= Token::createToken("ArrayList");
Token ArrayList_	= Token::createToken("ArrayList'");

TokenizedString tokenize(const std::string& str) {
	std::vector<Token> tokens;
	for (std::size_t i = 0; i < str.size(); ++i) {
		if (std::isspace(str[i])) { continue; }
		if (str[i] == '"') {
			bool escaped = false;
			auto s		 = new std::string();
			++i;
			while (i < str.size() && (str[i] != '"' || escaped)) {
				if (str[i] == '\\') {
					escaped = !escaped;
					s->push_back(str[i]);
				} else {
					if (!escaped) {
						s->push_back(str[i]);
					} else {
						switch (str[i]) {
							case 'n': s->push_back('\n'); break;
							case 't': s->push_back('\t'); break;
							case 'r': s->push_back('\r'); break;
							case 'b': s->push_back('\b'); break;
							case 'f': s->push_back('\f'); break;
							case '"': s->push_back('"'); break;
							case '\\': s->push_back('\\'); break;
							default: throw std::runtime_error("Invalid escape sequence in string");
						}
					}
					escaped = false;
				}
				++i;
			}
			tokens.push_back(String);
			tokens.back().data = (uint8_t*)s;
		} else if (std::isdigit(str[i]) || (str[i] == '-' && i + 1 < str.size() && std::isdigit(str[i + 1]))) {
			std::size_t len = 0;
			double		num = std::stod(str.substr(i), &len);
			tokens.push_back(Number);
			tokens.back().data = *reinterpret_cast<uint8_t**>(&num);
			i += len - 1;
		} else if (str.substr(i).starts_with("true")) {
			tokens.push_back(Boolean);
			tokens.back().data = (uint8_t*)1;
			i += 3;
		} else if (str.substr(i).starts_with("false")) {
			tokens.push_back(Boolean);
			tokens.back().data = (uint8_t*)0;
			i += 4;
		} else if (str.substr(i).starts_with("null")) {
			tokens.push_back(Null);
			i += 2;
		} else tokens.push_back(str[i]);
	}
	tokens.push_back(Token::eof);
	return TokenizedString(tokens);
}

CFG<Token>& getJSONGrammar() {
	static CFG<Token> g(Value, Token::eof);
	g.terminals	   = {String, Number, Boolean, Null, Token::eof, '{', '}', '[', ']', ':', ','};
	g.nonTerminals = {Object, Array, Value, PropertyList, PropertyList_, ArrayList, ArrayList_};
	g.addRule(Value, {Object});
	g.addRule(Value, {Array});
	g.addRule(Value, {String});
	g.addRule(Value, {Number});
	g.addRule(Value, {Boolean});

	g.addRule(Object, {'{', PropertyList, '}'});
	g.addRule(PropertyList, {});
	g.addRule(PropertyList, {String, ':', Value, PropertyList_});
	g.addRule(PropertyList_, {});
	g.addRule(PropertyList_, {',', String, ':', Value, PropertyList_});

	g.addRule(Array, {'[', ArrayList, ']'});
	g.addRule(ArrayList, {});
	g.addRule(ArrayList, {Value, ArrayList_});
	g.addRule(ArrayList_, {});
	g.addRule(ArrayList_, {',', Value, ArrayList_});
	return g;
};

std::ostream& operator<<(std::ostream& out, const JSONType& type) {
	switch (type) {
		case JSONType::String: out << "String"; break;
		case JSONType::Number: out << "Number"; break;
		case JSONType::Boolean: out << "Boolean"; break;
		case JSONType::Null: out << "Null"; break;
		case JSONType::Object: out << "Object"; break;
		case JSONType::Array: out << "Array"; break;
		default: out << "Unknown";
	}
	return out;
}

std::string_view toString(JSONType type) {
	switch (type) {
		case JSONType::String: return "String";
		case JSONType::Number: return "Number";
		case JSONType::Boolean: return "Boolean";
		case JSONType::Null: return "Null";
		case JSONType::Object: return "Object";
		case JSONType::Array: return "Array";
		default: return "Unknown";
	}
}

std::unique_ptr<JSON> parseTreeToJSON(const std::unique_ptr<ParseNode<Token>>& node) {
	if (node->value == String) {
		return std::make_unique<JSONString>(*reinterpret_cast<std::string*>(node->value.data));
	} else if (node->value == Number) {
		return std::make_unique<JSONNumber>(*reinterpret_cast<const double*>(&node->value.data));
	} else if (node->value == Boolean) {
		return std::make_unique<JSONBoolean>(*reinterpret_cast<const bool*>(&node->value.data));
	} else if (node->value == Null) {
		return std::make_unique<JSONNull>();
	} else if (node->value == Object) {
		auto  obj	  = std::make_unique<JSONObject>();
		auto* current = node->children[1].get();	 // PropertyList
		while (current->value == PropertyList || current->value == PropertyList_) {
			if (current->children.size() == 1 && current->children[0]->value == Token::eps) break;
			assert(current->children.size() >= 3);
			int	  i		= (current->value == PropertyList_);
			auto& key	= current->children[0 + i]->value;
			auto  value = parseTreeToJSON(current->children[2 + i]);
			obj->properties.emplace(*reinterpret_cast<std::string*>(key.data), std::move(value));
			current = current->children[3 + i].get();
		}
		return obj;
	} else if (node->value == Array) {
		auto  arr	  = std::make_unique<JSONArray>();
		auto* current = node->children[1].get();	 // ArrayList
		while (current->value == ArrayList || current->value == ArrayList_) {
			if (current->children.size() == 1 && current->children[0]->value == Token::eps) break;
			assert(current->children.size() >= 1);
			int	 i	   = (current->value == ArrayList_);
			auto value = parseTreeToJSON(current->children[0 + i]);
			arr->elements.push_back(std::move(value));
			current = current->children[1 + i].get();
		}
		return arr;
	} else if (node->value == Value) return parseTreeToJSON(node->children[0]);
	throw std::runtime_error(std::format("Unknown JSON node type: {}", node->value));
}


std::unique_ptr<JSON> parseJSON(std::istream& in) {
	auto tokens = tokenize(std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()));
	auto tree   = JSONParser::getInstance().parse(tokens.get());
	if (!tree) {
		throw std::runtime_error("Failed to parse JSON");
	}
	return parseTreeToJSON(tree);
}

std::unique_ptr<JSON> JSONFromFile(const std::string_view &filename) {
	std::ifstream file(filename.data());
	if (!file.is_open()) {
		throw std::runtime_error(std::format("Failed to open JSON file: {}", filename));
	}
	return parseJSON(file);
};

