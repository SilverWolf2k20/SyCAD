#include <algorithm>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

struct XmlDocument 
{
    std::string path;
    std::string version;
    std::string encoding;
};

class XmlReader 
{

};

//-----------------------------------------------------------------------------
using std::istreambuf_iterator;
using std::ifstream;
using std::string;
using std::string_view;
using std::unique_ptr;

using std::make_unique;

//-----------------------------------------------------------------------------
class XmlWriter 
{
public:
    explicit XmlWriter() 
        : _buffer{nullptr}
    {}

    bool load(const string_view& path) noexcept 
    {
        ifstream file(path.data());

        if (!file)
            return false;

        _buffer = make_unique<string>(string((
            istreambuf_iterator<char>(file)), 
            istreambuf_iterator<char>()
        ));

        file.close();

        parse();
        return true;
    }

private:
    bool parse() 
    {

        for (const auto& symbol: *_buffer) {
            std::cout << symbol;
        }
        return true;
    }

private:
    unique_ptr<string> _buffer;
};

using std::cerr;

int main()
{
    string path = "Data.xml";

    XmlWriter xmlWriter;

    if(!xmlWriter.load(path))
        cerr << "Can't open input file.\n";
}