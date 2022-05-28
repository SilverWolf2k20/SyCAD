#ifndef XML_WRITER_HEADER
#define XML_WRITER_HEADER

#include <algorithm>
#include <fstream>
#include <memory>
#include <string>
#include <optional>
#include <forward_list>
#include <functional>

using std::istreambuf_iterator;
using std::ifstream;
using std::string;
using std::string_view;
using std::unique_ptr;
using std::forward_list;
using std::optional;
using std::function;

using std::advance;
using std::make_unique;

using std::nullopt;

namespace usl {

struct XmlAttribute
{
    string key;
    string value;
};

struct XmlNode
{
    string name;
    string value;
    // TODO: добавить сраный unique_ptr.
    forward_list<XmlNode> nodes;
    forward_list<XmlAttribute> attributes;

    optional<XmlAttribute> getAttribute(string name) const noexcept {
        for (const auto& attribute : attributes)
            if (attribute.key == name)
                return attribute;
        
        for (auto it = nodes.begin(); it != nodes.end(); ++it) {
            auto attribute = (*it).getAttribute(name);
            if (attribute != nullopt)
                return attribute;
        }
        return nullopt;
    }

    optional<XmlNode> getElement(string name) const noexcept {
        if (this->name == name)
            return *this;

        for (auto it = nodes.begin(); it != nodes.end(); ++it) {
            auto node = (*it).getElement(name);
            if (node != nullopt)
                return node;
        }
        return nullopt;
    }

    forward_list<XmlNode> getElements(string name) const noexcept {
        forward_list<XmlNode> current_nodes;
        for (const auto& node : nodes) {
            if (node.name == name)
                current_nodes.push_front(node);
        }
        return current_nodes;
    }
};

class XmlReader;

class XmlDocument
{
    friend XmlReader;
public:
    explicit XmlDocument()
    {}

    explicit XmlDocument(string version, string encoding) : 
        _version{ version }, _encoding{encoding}
    {}

    const string_view& getVersion() const noexcept {
        return _version;
    }

    const string_view& getEncoding() const noexcept {
        return _encoding;
    }

    optional<XmlAttribute> getAttribute(string name) const noexcept {
        return _root.getAttribute(name);
    }

    optional<XmlNode> getElement(string name) const noexcept {
        return _root.getElement(name);
    }

    optional<XmlNode> getRoot(string name) const noexcept {
        if(_root.name == name)
            return _root;
        return nullopt;
    }

private:
    string _version;
    string _encoding;
    XmlNode _root;
};

class XmlReader {
public:
    explicit XmlReader() :
        _buffer{ "" },
        _xmlDocument{ nullptr }
    {}

    optional<unique_ptr<XmlDocument>> load(const string_view& path) noexcept {
        // Загрузка данных в буфер.
        if(loadFileData(path) == false)
            return nullopt;
        // Извлечение данных.
        return parse();
    }

private:
    bool loadFileData(const string_view& path) noexcept {
        ifstream file(path.data());

        if (!file)
            return false;

        _buffer = string(
            istreambuf_iterator<char>(file),
            istreambuf_iterator<char>()
        );

        file.close();
        return true;
    }

    optional<unique_ptr<XmlDocument>> parse() noexcept {
        auto it = _buffer.begin();

        // Считываем заголовок.
        auto xmlDocument = readTitle(it);
        if (xmlDocument == nullopt)
            return nullopt;

        _xmlDocument = move(xmlDocument.value());

        // Считываем узлы.
        auto node = readNode(it);
        if (node == nullopt)
            return nullopt;

        _xmlDocument->_root = *node.value();
        return { move(_xmlDocument)};
    }

    optional<unique_ptr<XmlDocument>> readTitle(string::iterator& it) noexcept {
        // Чтение начала заголовка.
        string buffer = "";
        readData(it, buffer, [](const auto& it) {return (*it == ' '); });

        // Это начало заголовка?
        if (buffer.compare("<?xml") != 0)
            return nullopt;

        // Пропустить пробелы.
        skipSpaces(it);

        // Чтение атрибутов.
        forward_list<unique_ptr<XmlAttribute>> attributes;
        while (it != _buffer.end() && *it != '?') {
            auto attribute = readAttribute(it);

            if(attribute == nullopt)
                return nullopt;

            attributes.push_front(move(attribute.value()));
            advance(it, 1);
            skipSpaces(it);
        }

        XmlDocument document;
        document._encoding = attributes.front()->value;
        attributes.pop_front();
        document._version = attributes.front()->value;

        // Чтение конца заголовка.
        for (; it != _buffer.end(); ++it) {
            if (*it == '\n')
                break;
        }
        advance(it, 1);
        return { make_unique<XmlDocument>(document) };
    }

    optional<unique_ptr<XmlNode>> readNode(string::iterator& it) noexcept {
        XmlNode node;
        skipSpaces(it);
        skipComment(it);
        advance(it, 1);

        // Чтение окрывающегося тега.
        for (; it != _buffer.end() && *it != ' ' && *it != '>'; ++it) {
            node.name.push_back(*it);
        }

        skipSpaces(it);
        // Чтение атрибутов.
        forward_list<unique_ptr<XmlAttribute>> attributes;
        while (it != _buffer.end() && *it != '>') {
            auto attribute = readAttribute(it);

            if (attribute == nullopt)
                return nullopt;

            node.attributes.push_front(*attribute.value());
            advance(it, 1);
            skipSpaces(it);
        }

        advance(it, 1);
        skipSpaces(it);
        // Чтение контента в тэге.
        for (; it != _buffer.end(); ++it) {
            // Пропустить комментарий
            skipComment(it);
            // Считать вложенный тег.
            if (it != _buffer.end() && *it == '<' && isalnum(*(it + 1)) != 0) {
                auto childNode = readNode(it);
                if (childNode == nullopt)
                    return nullopt;
                node.nodes.push_front(*childNode.value());
                continue;
            }
            if (*it == '<') {
                ++it;
                break;
            }
            node.value.push_back(*it);
        }
        node.value.erase(remove(node.value.begin(), node.value.end(), '\n'), node.value.end());
        skipSpaces(it);

        // Чтение закрывающегося тега.
        string name = "";
        for (; it != _buffer.end(); ++it) {
            if (*it == '>') {
                ++it;
                break;
            }
            name.push_back(*it);
        }

        name.erase(remove(name.begin(), name.end(), '/'), name.end());
        if(node.name.compare(name) != 0)
            return nullopt;

        return { make_unique<XmlNode>(node) };
    }

    optional<unique_ptr<XmlAttribute>> readAttribute(string::iterator& it) noexcept {
        // Чтение ключа.
        XmlAttribute attribute;
        readData(it, attribute.key, [](const auto& it) {return (*it == ' ' || *it == '='); });

        // Пропустить пробелы и символы '=' и '"'.
        skipSpaces(it);
        advance(it, 1);
        skipSpaces(it);
        advance(it, 1);

        // Чтение данных.
        readData(it, attribute.value, [](const auto& it) {return (*it == '"'); });
        return { make_unique<XmlAttribute>(attribute) };
    }

    void readData(string::iterator& it, string& value, const function<bool(string::iterator&)> predicate) const noexcept {
        for (; it != _buffer.end(); ++it) {
            if (predicate(it))
                break;
            value.push_back(*it);
        }
    }

    void skipSpaces(string::iterator& it) const noexcept {
        for (; it != _buffer.end(); ++it) {
            if (isspace(static_cast<unsigned char>(*it)) == 0)
                break;
        }
    }

    void skipComment(string::iterator& it) const noexcept {
        while (it != _buffer.end()) {
            if (*it == '<' && *(it + 1) == '!') {
                string buffer = "";
                for (; it != _buffer.end(); ++it) {
                    if (*it == '>' && buffer.find("--")) {
                        break;
                    }
                }
                advance(it, 1);
                skipSpaces(it);
                continue;
            }
            break;
        }
    }

private:
    string _buffer;
    unique_ptr<XmlDocument> _xmlDocument;
};
}
#endif // !XML_WRITER_HEADER