#include "request_queue.h"

RequestQueue::RequestQueue(const SearchServer &search_server) : search_server_(search_server) {
}
std::vector<Document> RequestQueue::AddFindRequest(const std::string &raw_query, DocumentStatus status) {
    ++current_minutes_counter_;
    if (current_minutes_counter_ > min_in_day_) {
        requests_.pop_front();
    }
    auto documents = search_server_.FindTopDocuments(raw_query, status);
    requests_.push_back({raw_query, !empty(documents)});
    return documents;
}

std::vector<Document> RequestQueue::AddFindRequest(const std::string &raw_query) {
    ++current_minutes_counter_;
    if (current_minutes_counter_ > min_in_day_) {
        requests_.pop_front();
    }
    auto documents = search_server_.FindTopDocuments(raw_query);
    requests_.push_back({raw_query, !empty(documents)});

    return documents;
}

int RequestQueue::GetNoResultRequests() const {
    int no_results = 0;
    for (const auto &request: requests_) {
        if (!request.it_has_results) {
            ++no_results;
        }
    }
    return no_results;
}