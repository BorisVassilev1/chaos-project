#include <json/json.hpp>
#include <fstream>
#include "log.hpp"
#include "util/utils.hpp"

const constexpr Token String		= Token::createTokenIKWIAD(1001ull);
const constexpr Token Number		= Token::createTokenIKWIAD(1002ull);
const constexpr Token Boolean		= Token::createTokenIKWIAD(1003ull);
const constexpr Token Null			= Token::createTokenIKWIAD(1004ull);
const constexpr Token Object		= Token::createTokenIKWIAD(1005ull);
const constexpr Token Array			= Token::createTokenIKWIAD(1006ull);
const constexpr Token Value			= Token::createTokenIKWIAD(1007ull);
const constexpr Token PropertyList	= Token::createTokenIKWIAD(1008ull);
const constexpr Token PropertyList_ = Token::createTokenIKWIAD(1009ull);
const constexpr Token ArrayList		= Token::createTokenIKWIAD(1010ull);
const constexpr Token ArrayList_	= Token::createTokenIKWIAD(1011ull);

JOB(asdasd, {
	Token::createToken("String", 1001);
	Token::createToken("Number", 1002);
	Token::createToken("Boolean", 1003);
	Token::createToken("Null", 1004);
	Token::createToken("Object", 1005);
	Token::createToken("Array", 1006);
	Token::createToken("Value", 1007);
	Token::createToken("PropertyList", 1008);
	Token::createToken("PropertyList'", 1009);
	Token::createToken("ArrayList", 1010);
	Token::createToken("ArrayList'", 1011);
});

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

#include <stack>
// non-recursive
std::unique_ptr<JSON> JSONParser::makeParseTree(
	const std::vector<std::reference_wrapper<const Parser<Token>::DeltaMap::value_type>>& productions,
	const std::vector<Token>& word, int& k, int& j) {
	std::size_t prodIndex = 0;
	std::size_t wordIndex = 0;

	std::stack<std::unique_ptr<JSON>*> stack;
	std::unique_ptr<JSON>			   root;

	// TODO:
	std::unique_ptr<JSON>* current = nullptr;
	stack.push(&root);
	while (prodIndex < productions.size() && wordIndex < word.size()) {
		current					 = stack.top();
		const auto& [lhs, rhs]	 = productions[prodIndex].get();
		const auto& [_, _, FROM] = lhs;
		const auto& [_, TO]		 = rhs;
		dbLog(dbg::LOG_DEBUG, "parsing: ", FROM);

		switch (FROM.value) {	  // always a non-terminal
			case Object.value: {
				*current = std::make_unique<JSONObject>();
				break;
			}
			case Value.value: {
				++prodIndex;
				continue;
			}
			default: throw std::runtime_error(std::format("Unexpected non-terminal: {}", FROM));
		};
	}

	root->print(std::cout);
	std::cout << std::endl;
	return root;
}

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
	Timer t;
	auto  tokens = tokenize(std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()));
	dbLog(dbg::LOG_DEBUG, "done tokenizing ", t.elapsed<std::chrono::milliseconds>(), "ms");
	auto tree = JSONParser::getInstance().parse(tokens.get());
	dbLog(dbg::LOG_DEBUG, "done parsing JSON ", t.elapsed<std::chrono::milliseconds>(), "ms");
	if (!tree) { throw std::runtime_error("Failed to parse JSON"); }
	// return parseTreeToJSON(tree);
	return tree;
}

std::unique_ptr<JSON> JSONFromFile(const std::string_view& filename) {
	std::ifstream file(filename.data());
	if (!file.is_open()) { throw std::runtime_error(std::format("Failed to open JSON file: {}", filename)); }
	return parseJSON(file);
};
