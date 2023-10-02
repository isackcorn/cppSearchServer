#include "document.h"
#include <iostream>

using namespace std::string_literals;

Document::Document() = default;

Document::Document(int id, double relevance, int rating)
        : id(id), relevance(relevance), rating(rating) {
}

std::ostream &operator<<(std::ostream &out, const Document &doc) {
    out << "{ "s << "document_id = "s << doc.id << ", "s << "relevance = "s << doc.relevance << ", "s << "rating = "s
        << doc.rating << " }"s;
    return out;
}