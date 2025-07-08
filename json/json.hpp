#pragma once

#include <DPDA/parser.h>
#include <DPDA/token.h>
#include <cassert>

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

class TokenizedString : std::vector<Token> {
   public:
	using std::vector<Token>::vector;
	using std::vector<Token>::operator=;
	using std::vector<Token>::size;
	using std::vector<Token>::operator[];
	using std::vector<Token>::begin;
	using std::vector<Token>::end;

	TokenizedString() = default;
	TokenizedString(std::vector<Token>&& tokens) : std::vector<Token>(std::move(tokens)) {}
	TokenizedString(const std::vector<Token>& tokens) : std::vector<Token>(tokens) {}

	~TokenizedString() {
		for (auto& token : *this) {
			if (token == String) { delete reinterpret_cast<std::string*>(token.data); }
		}
	}

	auto get() const -> const std::vector<Token>& { return *this; }
};

inline auto tokenize(const std::string& str) {
	std::vector<Token> tokens;
	for (std::size_t i = 0; i < str.size(); ++i) {
		if (std::isspace(str[i])) { continue; }
		if (str[i] == '"') {
			std::size_t start	= i + 1;
			bool		escaped = false;
			auto		s		= new std::string();
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
			i += 3;
		} else if (str.substr(i).starts_with("null")) {
			tokens.push_back(Null);
			i += 2;
		} else tokens.push_back(str[i]);
	}
	tokens.push_back(Token::eof);
	return TokenizedString(tokens);
}

inline auto& getJSONGrammar() {
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

class JSONParser : public Parser<Token> {
   public:
	JSONParser() : Parser<Token>(getJSONGrammar()) {}
};

enum class JSONType { NONE, String, Number, Boolean, Null, Object, Array };

std::ostream &operator<<(std::ostream& out, const JSONType& type) {
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

template<> struct std::formatter<JSONType> : ostream_formatter {};

class JSON {
	const JSONType type;

   public:
	JSON(JSONType t) : type(t) {}
	virtual ~JSON() = default;

	virtual void print(std::ostream& out) const = 0;
	auto		 getType() const { return type; }

	template <class T>
	inline auto& as() {
		if (type != T::type) {
			throw std::runtime_error(std::format("Cannot cast JSON of type {} to {}", static_cast<int>(type), T::type));
		}
		return static_cast<T&>(*this);
	}
};

class JSONNumber : public JSON {
   public:
	float					  value;
	static constexpr JSONType type = JSONType::Number;

	template <class T>
	JSONNumber(T&& val) : JSON(JSONType::Number), value(static_cast<float>(val)) {}

	void print(std::ostream& out) const override { out << value; }

	operator float() const { return value; }
	operator float&() { return value; }

};

class JSONString : public JSON {
   public:
	std::string				  value;
	static constexpr JSONType type = JSONType::String;

	JSONString(const std::string& val) : JSON(JSONType::String), value(val) {}

	void print(std::ostream& out) const override { out << '"' << value << '"'; }

	operator const std::string&() const { return value; }
	operator std::string&() { return value; }
	operator std::string_view() const { return std::string_view(value); }
};

class JSONBoolean : public JSON {
   public:
	bool value;

	static constexpr JSONType type = JSONType::Boolean;

	JSONBoolean(bool val) : JSON(JSONType::Boolean), value(val) {}

	void print(std::ostream& out) const override { out << (value ? "true" : "false"); }

	operator bool() const { return value; }
	operator bool&() { return value; }
	operator const bool&() const { return value; }
};

class JSONNull : public JSON {
   public:
	JSONNull() : JSON(JSONType::Null) {}
	static constexpr JSONType type = JSONType::Null;
	void					  print(std::ostream& out) const override { out << "null"; }
};

class JSONObject : public JSON {
   public:
	std::unordered_map<std::string, std::unique_ptr<JSON>> properties;
	static constexpr JSONType							   type = JSONType::Object;

	JSONObject() : JSON(JSONType::Object) {}

	void print(std::ostream& out) const override {
		out << '{';
		bool first = true;
		for (const auto& [key, value] : properties) {
			if (!first) out << ", ";
			first = false;
			out << '"' << key << "\": ";
			value->print(out);
		}
		out << '}';
	}

	template <class T>
	inline auto& operator[](T&& key) {
		auto it = properties.find(std::forward<T>(key));
		if (it == properties.end()) { throw std::runtime_error(std::format("Key '{}' not found in JSON object", key)); }
		return *it->second;
	}
};

class JSONArray : public JSON {
   public:
	std::vector<std::unique_ptr<JSON>> elements;
	static constexpr JSONType		   type = JSONType::Array;

	JSONArray() : JSON(JSONType::Array) {}

	void print(std::ostream& out) const override {
		out << '[';
		bool first = true;
		for (const auto& element : elements) {
			if (!first) out << ", ";
			first = false;
			element->print(out);
		}
		out << ']';
	}

	inline auto&	   operator[](int i) { return *elements[i].get(); }
	inline const auto& operator[](int i) const { return *elements[i].get(); }
};

std::unique_ptr<JSON> parseTreeToJSON(const std::unique_ptr<ParseNode<Token>>& node) {
	if (node->value == String) {
		return std::make_unique<JSONString>(*reinterpret_cast<std::string*>(node->value.data));
	} else if (node->value == Number) {
		return std::make_unique<JSONNumber>(*reinterpret_cast<const double*>(&node->value.data));
	} else if (node->value == Boolean) {
		return std::make_unique<JSONBoolean>(*reinterpret_cast<const bool*>(&node->children[0]->value.data));
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
