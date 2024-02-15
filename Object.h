#ifndef OBJECT_H
#define OBJECT_H
#include <any>
#include <optional>
#include <stdexcept>

class Object {
public:
    template<typename _T>
    Object(_T data): pData(data) {
    }

    template<typename _T>
    _T Cast() const {
        if (!pData.has_value()) {
            throw std::runtime_error("Data is empty!");
        }
        if (typeid(_T) != pData.value().type()) {
            throw std::runtime_error("Type mismatch!");
        } {
            try {
                return std::any_cast<_T>(pData.value());
            }
            catch (const std::bad_any_cast) {
                throw std::runtime_error("Cannot cast data to " + std::string(typeid(_T).name()));
            }
        }
    }

private:
    std::optional<std::any> pData;
};
#endif //OBJECT_H
