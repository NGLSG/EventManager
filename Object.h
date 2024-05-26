#ifndef OBJECT_H
#define OBJECT_H
#include <any>
#include <iostream>
#include <optional>
#include <stdexcept>

namespace Event {
    class Object {
    public:
        template<typename _T>
        Object(_T data): pData(data) {
            uid = UUID::New();
        }

        template<typename _T>
        std::optional<_T> Cast() const {
            if (!pData.has_value()) {
                throw std::runtime_error("Data is empty!");
            }
            if (typeid(_T) != pData.value().type()) {
                throw std::runtime_error("Type mismatch!");
            } {
                try {
                    return std::any_cast<_T>(pData.value());
                }
                catch (const std::bad_any_cast&) {
                    std::cerr << "Cannot cast data to " + std::string(typeid(_T).name()) << std::endl;
                }
            }
            return std::nullopt;
        }

        template<class _T>
        bool is() {
            return pData.has_value() && pData.value().type() == typeid(_T);
        }

        template<typename _T>
        Object& operator=(_T data) {
            pData = data;
            return *this;
        }

        bool operator==(const Object&obj) const {
            return uid == obj.uid;
        }

    private:
        UUID uid;
        std::optional<std::any> pData;
    };
}
#endif //OBJECT_H
