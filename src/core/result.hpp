#pragma once

#include <variant>
#include <utility>
#include <type_traits>

template<typename T, typename E>
class Result {
    std::variant<T, E> data_;

public:
    // Constructors                 
    Result(T value) : data_(std::move(value)) {}
    Result(E error) : data_(std::move(error)) {}
    
    // Status checks
    bool is_ok() const noexcept { 
        return std::holds_alternative<T>(data_); 
    }
    
    bool is_error() const noexcept { 
        return std::holds_alternative<E>(data_); 
    }
    
    explicit operator bool() const noexcept { 
        return is_ok(); 
    }
    
    // Value accessors
    T& value() & { 
        return std::get<T>(data_); 
    }
    
    const T& value() const& { 
        return std::get<T>(data_); 
    }
    
    T&& value() && { 
        return std::get<T>(std::move(data_)); 
    }
    
    // Error accessors
    E& error() & { 
        return std::get<E>(data_); 
    }
    
    const E& error() const& { 
        return std::get<E>(data_); 
    }
    
    E&& error() && { 
        return std::get<E>(std::move(data_)); 
    }
    
    // Monadic operations
    template<typename F>
    auto and_then(F&& f) -> decltype(f(std::declval<T>())) {
        using ReturnType = decltype(f(std::declval<T>()));
        
        if (is_ok()) {
            return f(std::move(value()));
        }
        return ReturnType(std::move(error()));
    }
    
    template<typename F>
    auto map(F&& f) -> Result<decltype(f(std::declval<T>())), E> {
        using U = decltype(f(std::declval<T>()));
        
        if (is_ok()) {
            return Result<U, E>(f(std::move(value())));
        }
        return Result<U, E>(std::move(error()));
    }
    
    template<typename F>
    auto or_else(F&& f) -> Result<T, E> {
        if (is_error()) {
            return f(std::move(error()));
        }
        return std::move(*this);
    }
    
    // Unwrap with default
    T value_or(T default_value) && {
        if (is_ok()) {
            return std::move(value());
        }
        return default_value;
    }
};

// Helper type traits for result construction
template<typename T>
struct is_result : std::false_type {};

template<typename T, typename E>
struct is_result<Result<T, E>> : std::true_type {};