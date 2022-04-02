#pragma once

#include <json.h>

namespace json {

class Builder {
    class DictItemContext;
    class KeyItemContext;
    class ArrayItemContext;

  public:
    Builder() {
        nodes_stack_.push_back(&root_);
    }

    KeyItemContext Key(const std::string &);
    Builder &Value(const Node &);
    DictItemContext StartDict();
    ArrayItemContext StartArray();
    Builder &EndDict();
    Builder &EndArray();

    Node Build();

  private:
    class BaseContext {
      public:
        BaseContext(Builder &builder) : builder_(builder) {}

        Builder &Get() {
            return builder_;
        }

      private:
        Builder &builder_;
    };

    class DictItemContext : public BaseContext {
      public:
        DictItemContext(Builder &builder) : BaseContext(builder) {}

        KeyItemContext Key(std::string &&value) {
            return Get().Key(std::move(value));
        }
        Builder &EndDict() {
            return Get().EndDict();
        }
    };

    class KeyItemContext : public BaseContext {
      public:
        KeyItemContext(Builder &builder) : BaseContext(builder) {}

        DictItemContext Value(Node &&value) {
            Get().Value(std::move(value));
            return DictItemContext{Get()};
        }
        DictItemContext StartDict() {
            return Get().StartDict();
        }
        ArrayItemContext StartArray() {
            return Get().StartArray();
        }
    };

    class ArrayItemContext : public BaseContext {
      public:
        ArrayItemContext(Builder &builder) : BaseContext(builder) {}

        ArrayItemContext Value(Node &&value) {
            Get().Value(std::move(value));
            return ArrayItemContext{Get()};
        }
        DictItemContext StartDict() {
            return Get().StartDict();
        }
        ArrayItemContext StartArray() {
            return Get().StartArray();
        }
        Builder &EndArray() {
            return Get().EndArray();
        }
    };

  private:
    template <typename Container>
    Builder &Start(Container) {
        using namespace std::literals;

        if (nodes_stack_.empty() ||
            (!nodes_stack_.back()->IsNull() && !nodes_stack_.back()->IsArray())) {
            throw std::logic_error("Array must be create before"s);
        }

        if (nodes_stack_.back()->IsArray()) {
            const_cast<Array &>(nodes_stack_.back()->AsArray()).push_back(Container());
            Node *node = &const_cast<Array &>(nodes_stack_.back()->AsArray()).back();
            nodes_stack_.push_back(node);
        } else {
            *nodes_stack_.back() = Container();
        }

        return *this;
    }

  private:
    Node root_ = nullptr;
    std::vector<Node *> nodes_stack_;
};

} // namespace json
