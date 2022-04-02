#include "json_builder.h"

namespace json {

using namespace std::literals;

Builder::KeyItemContext Builder::Key(const std::string &key) {
    if (nodes_stack_.empty() || !nodes_stack_.back()->IsDict()) {
        throw std::logic_error("Dict must be create before"s);
    }
    nodes_stack_.emplace_back(&const_cast<Dict &>(nodes_stack_.back()->AsDict())[key]);
    return *this;
}

Builder &Builder::Value(const Node &value) {
    if (nodes_stack_.empty() ||
        (!nodes_stack_.back()->IsNull() && !nodes_stack_.back()->IsArray())) {
        throw std::logic_error("Array must be create before"s);
    }

    if (nodes_stack_.back()->IsArray()) {
        const_cast<Array &>(nodes_stack_.back()->AsArray()).push_back(value);
    } else {
        *nodes_stack_.back() = value;
        nodes_stack_.pop_back();
    }
    return *this;
}

Builder::DictItemContext Builder::StartDict() {
    return Start(Dict());
}

Builder::ArrayItemContext Builder::StartArray() {
    return Start(Array());
}

Builder &Builder::EndDict() {
    if (nodes_stack_.empty() || !nodes_stack_.back()->IsDict()) {
        throw std::logic_error("StartDict() must be run before"s);
    }
    nodes_stack_.pop_back();
    return *this;
}

Builder &Builder::EndArray() {
    if (nodes_stack_.empty() || !nodes_stack_.back()->IsArray()) {
        throw std::logic_error("StartArray() must be run before"s);
    }
    nodes_stack_.pop_back();
    return *this;
}

Node Builder::Build() {
    if (!nodes_stack_.empty()) {
        throw std::logic_error("Nothing to build"s);
    }
    return root_;
}

} // namespace json
