#include "json.h"
#include <charconv>
#include <cstring>

using namespace std;

namespace json {

namespace {

Node LoadNode(istream& input);

Node LoadNull(istream& input) {
    constexpr string_view null_str = "null";
    char buf[4];
    input.read(buf, 4);
    if (input.gcount() != 4 || memcmp(buf, null_str.data(), 4)) {
        throw ParsingError("Null parsing error");
    }
    return nullptr;
}

string LoadString(istream& input) {
    string s;
    s.reserve(32); // Pre-allocate to avoid frequent reallocations
    
    char ch;
    while (input.get(ch)) {
        if (ch == '"') {
            return s;
        } else if (ch == '\\') {
            if (!input.get(ch)) {
                throw ParsingError("String parsing error");
            }
            switch (ch) {
                case 'n':  s += '\n'; break;
                case 't':  s += '\t'; break;
                case 'r':  s += '\r'; break;
                case '"':  s += '"';  break;
                case '\\': s += '\\'; break;
                default: throw ParsingError("Unrecognized escape sequence \\"s + ch);
            }
        } else if (ch == '\n' || ch == '\r') {
            throw ParsingError("Unexpected end of line"s);
        } else {
            s += ch;
        }
    }
    throw ParsingError("String parsing error");
}

Node LoadNumber(istream& input) {
    string parsed_num;
    parsed_num.reserve(16); // Reserve space for typical numbers
    
    auto read_char = [&parsed_num, &input] {
        parsed_num += static_cast<char>(input.get());
        if (!input) throw ParsingError("Failed to read number from stream"s);
    };

    if (input.peek() == '-') {
        read_char();
    }

    if (input.peek() == '0') {
        read_char();
    } else if (isdigit(input.peek())) {
        while (isdigit(input.peek())) {
            read_char();
        }
    } else {
        throw ParsingError("A digit is expected"s);
    }

    bool is_int = true;
    if (input.peek() == '.') {
        read_char();
        is_int = false;
        if (!isdigit(input.peek())) {
            throw ParsingError("A digit is expected after decimal point"s);
        }
        while (isdigit(input.peek())) {
            read_char();
        }
    }

    if (tolower(input.peek()) == 'e') {
        read_char();
        is_int = false;
        if (input.peek() == '+' || input.peek() == '-') {
            read_char();
        }
        if (!isdigit(input.peek())) {
            throw ParsingError("A digit is expected in exponent"s);
        }
        while (isdigit(input.peek())) {
            read_char();
        }
    }

    // Fast path for integers
    if (is_int) {
        int int_value;
        auto result = from_chars(parsed_num.data(), parsed_num.data() + parsed_num.size(), int_value);
        if (result.ec == errc()) {
            return int_value;
        }
    }

    // Fallback to double parsing
    double double_value;
    auto result = from_chars(parsed_num.data(), parsed_num.data() + parsed_num.size(), double_value);
    if (result.ec == errc()) {
        return double_value;
    }

    throw ParsingError("Failed to convert "s + parsed_num + " to number"s);
}

Node LoadBool(istream& input) {
    constexpr string_view true_str = "true";
    constexpr string_view false_str = "false";
    
    char first_char = input.get();
    bool value = (first_char == 't');
    const auto& str = value ? true_str : false_str;
    
    char buf[4];
    const size_t len = value ? 3 : 4; // "rue" or "alse"
    input.read(buf, len);
    
    if (input.gcount() != len || memcmp(buf, str.data() + 1, len)) {
        throw ParsingError("Bool parsing error");
    }
    
    return value;
}

Node LoadArray(istream& input) {
    Array result;
    result.reserve(8); // Pre-allocate typical array size
    
    ws(input); // Skip whitespace
    if (input.peek() == ']') {
        input.get();
        return result;
    }

    while (true) {
        result.push_back(LoadNode(input));
        ws(input);
        char c = input.get();
        if (c == ']') break;
        if (c != ',') {
            throw ParsingError("Array parsing error");
        }
        ws(input);
    }

    return result;
}

Node LoadDict(istream& input) {
    Dict result;
    
    ws(input);
    if (input.peek() == '}') {
        input.get();
        return result;
    }

    while (true) {
        ws(input);
        if (input.get() != '"') {
            throw ParsingError("Dict parsing error");
        }
        
        string key = LoadString(input);
        ws(input);
        if (input.get() != ':') {
            throw ParsingError("Dict parsing error");
        }
        
        result.emplace(move(key), LoadNode(input));
        ws(input);
        
        char c = input.get();
        if (c == '}') break;
        if (c != ',') {
            throw ParsingError("Dict parsing error");
        }
    }

    return result;
}

Node LoadNode(istream& input) {
    ws(input);
    char c = input.peek();
    
    switch (c) {
        case 'n': return LoadNull(input);
        case '"': input.get(); return LoadString(input);
        case 't':
        case 'f': return LoadBool(input);
        case '[': input.get(); return LoadArray(input);
        case '{': input.get(); return LoadDict(input);
        default:  return LoadNumber(input);
    }
}

} //namespace

bool Node::IsInt() const {
    return holds_alternative<int>(value_);
}

bool Node::IsDouble() const {
    return holds_alternative<double>(value_) || holds_alternative<int>(value_);
}

bool Node::IsPureDouble() const {
    return holds_alternative<double>(value_);
}

bool Node::IsBool() const {
    return holds_alternative<bool>(value_);
}

bool Node::IsString() const {
    return holds_alternative<std::string>(value_);
}

bool Node::IsNull() const {
    return holds_alternative<std::nullptr_t>(value_);
}

bool Node::IsArray() const {
    return holds_alternative<Array>(value_);
}

bool Node::IsMap() const {
    return holds_alternative<Dict>(value_);
}

int Node::AsInt() const {
    if (!IsInt()) throw ParsingError("not int");
    return std::get<int>(value_);
}

bool Node::AsBool() const {
    if (!IsBool()) throw ParsingError("not bool");
    return std::get<bool>(value_);
}

double Node::AsDouble() const {
    if (!IsDouble()) throw ParsingError("not double");
    if (IsInt()) return static_cast<double>(std::get<int>(value_));
    return std::get<double>(value_);
}

const std::string& Node::AsString() const {
    if (!IsString()) throw ParsingError("not string");
    return std::get<std::string>(value_);
}

const Array& Node::AsArray() const {
    if (!IsArray()) throw ParsingError("not array");
    return std::get<Array>(value_);
}

const Dict& Node::AsMap() const {
    if (!IsMap()) throw ParsingError("wrong map");
    return std::get<Dict>(value_);
}

const Node::Value& Node::GetValue() const {
    return value_;
}

bool Node::operator==(const Node& rhs) const {
    return value_ == rhs.value_;
}

bool Node::operator!=(const Node& rhs) const {
    return !(value_ == rhs.value_);
}

Document::Document(Node root)
    : root_(std::move(root)) {
}

const Node& Document::GetRoot() const {
    return root_;
}

bool Document::operator==(const Document& rhs) const {
    return root_ == rhs.root_;
}

bool Document::operator!=(const Document& rhs) const {
    return !(root_ == rhs.root_);
}

Document Load(istream& input) {
    return Document{ LoadNode(input) };
}

void PrintValue(std::nullptr_t, const PrintContext& ctx) {
    ctx.out << "null"sv;
}

void PrintValue(std::string value, const PrintContext& ctx) {
    ctx.out << "\""sv;
    for (const char& c : value) {
        if (c == '\n') {
            ctx.out << "\\n"sv;
            continue;
        }
        if (c == '\r') {
            ctx.out << "\\r"sv;
            continue;
        }
        if (c == '\"') ctx.out << "\\"sv;
        if (c == '\t') {
            ctx.out << "\t"sv;
            continue;
        }
        if (c == '\\') ctx.out << "\\"sv;
        ctx.out << c;
    }
    ctx.out << "\""sv;
}

void PrintValue(bool value, const PrintContext& ctx) {
    ctx.out << std::boolalpha << value;
}

void PrintValue(Array array, const PrintContext& ctx) {
    ctx.out << "[\n"sv;
    auto inner_ctx = ctx.Indented();
    bool first = true;
    for (const auto& elem : array) {
        if (first) first = false;
        else ctx.out << ",\n"s;
        inner_ctx.PrintIndent();
        PrintNode(elem, inner_ctx);
    }
    ctx.out << "\n"s;
    ctx.PrintIndent();
    ctx.out << "]"sv;
}

void PrintValue(Dict dict, const PrintContext& ctx) {
    ctx.out << "{\n"sv;
    auto inner_ctx = ctx.Indented();
    bool first = true;
    for (auto& [key, node] : dict) {
        if (first) first = false;
        else ctx.out << ",\n"s;
        inner_ctx.PrintIndent();
        PrintValue(key, ctx);
        ctx.out << ": ";
        PrintNode(node, inner_ctx);
    }
    ctx.out << "\n"s;
    ctx.PrintIndent();
    ctx.out << "}"sv;
}

void PrintNode(const Node& node, const PrintContext& ctx) {
    std::visit(
        [&ctx](const auto& value) { PrintValue(value, ctx); },
        node.GetValue());
}

void Print(const Document& doc, std::ostream& output) {
    PrintContext ctx{ output };
    PrintNode(doc.GetRoot(), ctx);
}

}  // namespace json
