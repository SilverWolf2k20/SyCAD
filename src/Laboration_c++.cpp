#include "XmlReader.h"
#include <windows.h>
#include <iostream>

//-----------------------------------------------------------------------------
using std::cerr;
using std::cout;
using std::endl;

int main()
{
    string path{ "Data.xml" };

    usl::XmlReader xml;
    auto xdoc = xml.load(path).value();

    // получаем корневой узел
    auto people = xdoc->getRoot("people");
    // проходим по всем элементам person
    if (people != nullopt) {
        auto persons = people.value().getElements("person");
        for (const auto& peson : persons) {
            cout << peson.getAttribute("name").value().value  << '\n';
            cout << peson.getElement("company").value().value << '\n';
            cout << peson.getElement("age").value().value     << '\n';
            cout << endl;
        }
    }
    cout  << "\n\n";
}
