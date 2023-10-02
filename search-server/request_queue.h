#pragma once
#include <string>
#include "search_server.h"
#include <vector>
#include <deque>
#include <set>




class RequestQueue {
public:

    template<typename DocumentPredicate>
    std::vector<Document> AddFindRequest(const std::string &raw_query, DocumentPredicate document_predicate);

    explicit RequestQueue(const SearchServer &search_server);

    std::vector<Document> AddFindRequest(const std::string &raw_query, DocumentStatus status);

    std::vector<Document> AddFindRequest(const std::string &raw_query);

    [[nodiscard]] int GetNoResultRequests() const;

private:
    struct QueryResult {
        std::string request;
        bool it_has_results;
    };
    std::deque<QueryResult> requests_;
    const static int min_in_day_ = 1440;
    const SearchServer &search_server_;
    int current_minutes_counter_ = 0;
};


template<typename DocumentPredicate>
std::vector<Document> RequestQueue::AddFindRequest(const std::string &raw_query, DocumentPredicate document_predicate) {
    ++current_minutes_counter_;
    if (current_minutes_counter_ > min_in_day_) {
        requests_.pop_front();
    }
    auto documents = search_server_.FindTopDocuments(raw_query, document_predicate);
    requests_.push_back({raw_query, !empty(documents)});
    return documents;
}

