#pragma once
#include <vector>
#include <iostream>


template<typename Iterator>
class Paginator {
public:
    Paginator(const Iterator &begin, const Iterator &end, size_t page_size) {
        for (auto i = begin; i != end; advance(i, page_size)) {
            if (distance(i, end) <= page_size) {
                docs_.push_back({i, end - 1});
                break;
            } else {
                docs_.push_back({i, i + 1});
            }
        }
    }

    size_t size() const {
        return docs_.size();
    }

    auto begin() const {
        return docs_.begin();
    }

    auto end() const {
        return docs_.end();
    }

private:
    std::vector<std::pair<Iterator, Iterator>> docs_;
};

template<typename Container>
auto Paginate(const Container &c, size_t page_size) {
    return Paginator(begin(c), end(c), page_size);
}

template<typename Iterator>
std::ostream &operator<<(std::ostream &out, const std::pair<Iterator, Iterator> &It) {
    if (It.first == It.second) {
        out << *It.first;
        return out;
    }
    out << *It.first << *It.second;
    return out;
}