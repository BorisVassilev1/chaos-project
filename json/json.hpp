#pragma once

#include <DPDA/parser.h>
#include <DPDA/token.h>
#include <cassert>

extern Token String;
extern Token Number;
extern Token Boolean;
extern Token Null;
extern Token Object;
extern Token Array;
extern Token Value;
extern Token PropertyList;
extern Token PropertyList_;
extern Token ArrayList;
extern Token ArrayList_;

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

TokenizedString tokenize(const std::string& str);

CFG<Token>& getJSONGrammar();

class JSONParser : public Parser<Token> {
	JSONParser() : Parser<Token>(getJSONGrammar()) {}

   public:
	static JSONParser& getInstance() {
		static JSONParser instance;
		return instance;
	}
};

enum class JSONType { NONE, String, Number, Boolean, Null, Object, Array };

std::ostream& operator<<(std::ostream& out, const JSONType& type);
std::string_view toString(JSONType type);

template <>
struct std::formatter<JSONType> : ostream_formatter {};

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
			throw std::runtime_error(std::format("Cannot cast JSON of type {} to {}", type, T::type));
		}
		return static_cast<T&>(*this);
	}
	template <class T>
	inline const auto& as() const {
		if (type != T::type) {
			throw std::runtime_error(std::format("Cannot cast JSON of type {} to {}", type, T::type));
		}
		return static_cast<const T&>(*this);
	}

	template <class T>
	inline bool is() const {
		return type == T::type;
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
	inline const auto& operator[](const std::string& key) const {
		auto it = properties.find(key);
		if (it == properties.end()) { throw std::runtime_error(std::format("Key '{}' not found in JSON object", key)); }
		return *it->second;
	}

	inline auto find(const std::string& key) const { return properties.find(key); }
	inline auto find(const std::string& key) { return properties.find(key); }
	inline auto begin() const { return properties.begin(); }
	inline auto begin() { return properties.begin(); }
	inline auto end() const { return properties.end(); }
	inline auto end() { return properties.end(); }
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

	inline auto	 size() const { return elements.size(); }
	inline auto	 begin() { return elements.begin(); }
	inline auto	 end() { return elements.end(); }
	inline auto	 begin() const { return elements.begin(); }
	inline auto	 end() const { return elements.end(); }
	inline auto& push_back(std::unique_ptr<JSON> json) {
		elements.push_back(std::move(json));
		return elements.back();
	}
	template <class... Args>
	inline auto& emplace_back(Args&&... args) {
		elements.emplace_back(std::make_unique<JSON>(std::forward<Args>(args)...));
		return elements.back();
	}
};

std::unique_ptr<JSON> parseTreeToJSON(const std::unique_ptr<ParseNode<Token>>& node);

std::unique_ptr<JSON> parseJSON(std::istream& in);

std::unique_ptr<JSON> JSONFromFile(const std::string_view& filename);
