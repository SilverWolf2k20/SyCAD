#ifndef COMMAND_LINE_READER_HEADER
#define COMMAND_LINE_READER_HEADER

#include <iostream>
#include <map>
#include <forward_list>

using std::cerr;
using std::cout;
using std::endl;
using std::map;
using std::find;
using std::string;
using std::string_view;
using std::forward_list;

namespace usl {

class Application
{
    string version_;
    string name_;
    forward_list <string_view> arguments_;
public:
    Application(int argc, char* argv[]) :
        name_{ "App" },
        version_{ "0.0.1" } {
        arguments_ = forward_list <string_view>(argv, argv + argc);
    }
    void setVersion(string version) noexcept {
        version_ = version;
    }
    void setName(string name) noexcept {
        name_ = name;
    }
    const string& getVersion() const noexcept {
        return version_;
    }
    const string& getName() const noexcept {
        return name_;
    }
    const forward_list<string_view>& getArguments() const noexcept {
        return arguments_;
    }
};

class CommandLineReader
{
    map<string, string> arguments_;
    bool helpOptionEnabled_ = false;
    bool versionOptionEnabled_ = false;
public:
    void process(const Application& application) const noexcept {
        auto arguments = application.getArguments();

        if (versionOptionEnabled_ == true && find(arguments.begin(), arguments.end(), "-v") != arguments.end()) {
            cout << application.getName() << " " << application.getVersion() << endl;
        }
        if (helpOptionEnabled_ == true && find(arguments.begin(), arguments.end(), "-h") != arguments.end()) {
            for (const auto& [key, value] : arguments_)
                cout << key << "\t" << value << endl;
        }
    }
    void addOption(string name, string description) {
        arguments_[name] = description;
    }
    void addVersionOption() {
        versionOptionEnabled_ = true;
        arguments_["-v"] = "Show version information.";
    }
    void addHelpOption() {
        helpOptionEnabled_ = true;
        arguments_["-h"] = "Show this help.";
    }
};
}

#endif // !COMMAND_LINE_READER_HEADER
