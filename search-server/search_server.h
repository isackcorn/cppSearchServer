#pragma once

#include <string>
#include "string_processing.h"
#include <algorithm>
#include "document.h"
#include <vector>
#include <map>
#include <cmath>
#include <execution>
#include <deque>
#include <list>
#include "concurrent_map.h"


using namespace std::string_literals;

const int MAX_RESULT_DOCUMENT_COUNT = 5;
constexpr float EPSILON = 1e-6;

class SearchServer {
public:
    std::map<int, std::map<std::string_view, double>> docId_to_word_freq_;
    template<typename StringContainer>
    explicit SearchServer(const StringContainer &stop_words);

    explicit SearchServer(const std::string &stop_words_text);


    void AddDocument(int document_id, std::string_view document, DocumentStatus status,
                     const std::vector<int> &ratings);

    template<typename DocumentPredicate, typename ExecutionPolicy>
    [[nodiscard]] std::vector<Document> FindTopDocuments(ExecutionPolicy &executionPolicy, std::string_view raw_query,
                                                         DocumentPredicate document_predicate) const;

    template<typename DocumentPredicate>
    [[nodiscard]] std::vector<Document> FindTopDocuments(std::string_view raw_query,
                                                         DocumentPredicate document_predicate) const;

    template<typename ExecutionPolicy>
    [[nodiscard]] std::vector<Document>
    FindTopDocuments(ExecutionPolicy &executionPolicy, std::string_view raw_query, DocumentStatus status) const;

    [[nodiscard]] std::vector<Document> FindTopDocuments(std::string_view raw_query, DocumentStatus status) const;

    template<typename ExecutionPolicy>
    [[nodiscard]] std::vector<Document>
    FindTopDocuments(ExecutionPolicy &executionPolicy, std::string_view raw_query) const;

    [[nodiscard]] std::vector<Document> FindTopDocuments(std::string_view raw_query) const;

    [[nodiscard]] int GetDocumentCount() const;

    [[nodiscard]] std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(std::string_view raw_query,
                                                                                          int document_id) const;

    [[nodiscard]] std::tuple<std::vector<std::string_view>, DocumentStatus>
    MatchDocument(const std::execution::parallel_policy &, std::string_view raw_query,
                  int document_id) const;

    [[nodiscard]] std::tuple<std::vector<std::string_view>, DocumentStatus>
    MatchDocument(const std::execution::sequenced_policy &, std::string_view raw_query,
                  int document_id) const;

    auto begin() {
        return document_ids_.begin();
    }

    auto end() {
        return document_ids_.end();
    }

    [[nodiscard]] const std::map<std::string_view, double> &GetWordFrequencies(int document_id) const;

    [[nodiscard]] std::list<std::string_view> GetJustWords(int document_id) const;

    void RemoveDocument(const std::execution::sequenced_policy &, int document_id);

    void RemoveDocument(const std::execution::parallel_policy &par, int document_id);

    void RemoveDocument(int document_id);


private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
        std::string data;
    };
    const std::set<std::string, std::less<>> stop_words_;
    std::map<std::string_view, std::map<int, double>> word_to_document_freqs_;
    std::map<int, DocumentData> documents_;
    std::set<int> document_ids_;


    [[nodiscard]] bool IsStopWord(const std::string_view word) const;

    static bool IsValidWord(std::string_view word);

    std::vector<std::string_view> SplitIntoWordsNoStop(const std::string_view text) const;

    static int ComputeAverageRating(const std::vector<int> &ratings);

    struct QueryWord {
        std::string_view data;
        bool is_minus;
        bool is_stop;
    };

    [[nodiscard]] QueryWord ParseQueryWord(std::string_view text) const;

    struct Query {
        std::vector<std::string_view> plus_words;
        std::vector<std::string_view> minus_words;
    };

    [[nodiscard]] Query ParseQuery(std::string_view text) const;

    [[nodiscard]] Query ParseQuery(const std::execution::parallel_policy &, std::string_view text) const;

    [[nodiscard]] double ComputeWordInverseDocumentFreq(std::string_view word) const;

    template<typename DocumentPredicate>
    [[nodiscard]] std::vector<Document> FindAllDocuments(const Query &query,
                                                         DocumentPredicate document_predicate) const;

    template<typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const std::execution::sequenced_policy &executionPolicy, const Query &query,
                                           DocumentPredicate document_predicate) const;

    template<typename DocumentPredicate>
    std::vector<Document>
    FindAllDocuments(const std::execution::parallel_policy &executionPolicy, const Query &query,
                     DocumentPredicate document_predicate) const;
};

template<typename DocumentPredicate>
[[nodiscard]] std::vector<Document> SearchServer::FindTopDocuments(std::string_view raw_query,
                                                                   DocumentPredicate document_predicate) const {
    return FindTopDocuments(std::execution::seq, raw_query, document_predicate);
}

template<typename DocumentPredicate, typename ExecutionPolicy>
[[nodiscard]] std::vector<Document>
SearchServer::FindTopDocuments(ExecutionPolicy &executionPolicy, std::string_view raw_query,
                               DocumentPredicate document_predicate) const {
    const auto query = ParseQuery(raw_query);

    auto matched_documents = FindAllDocuments(executionPolicy, query, document_predicate);

    sort(executionPolicy, matched_documents.begin(), matched_documents.end(),
         [](const Document &lhs, const Document &rhs) {
             if (std::abs(lhs.relevance - rhs.relevance) < EPSILON) {
                 return lhs.rating > rhs.rating;
             } else {
                 return lhs.relevance > rhs.relevance;
             }
         });
    if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
        matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    }

    return matched_documents;
}

template<typename ExecutionPolicy>
[[nodiscard]] std::vector<Document>
SearchServer::FindTopDocuments(ExecutionPolicy &executionPolicy, std::string_view raw_query,
                               DocumentStatus status) const {
    return FindTopDocuments(executionPolicy,
            raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
                return document_status == status;
            });
}

template<typename ExecutionPolicy>
[[nodiscard]] std::vector<Document>
SearchServer::FindTopDocuments(ExecutionPolicy &executionPolicy, std::string_view raw_query) const {
    return FindTopDocuments(executionPolicy, raw_query, DocumentStatus::ACTUAL);

}

template<typename DocumentPredicate>
[[nodiscard]] std::vector<Document> SearchServer::FindAllDocuments(const Query &query,
                                                                   DocumentPredicate document_predicate) const {
    std::map<int, double> document_to_relevance;
    for (const auto &word: query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
        for (const auto [document_id, term_freq]: word_to_document_freqs_.at(word)) {
            const auto &document_data = documents_.at(document_id);
            if (document_predicate(document_id, document_data.status, document_data.rating)) {
                document_to_relevance[document_id] += term_freq * inverse_document_freq;
            }
        }
    }

    for (const auto &word: query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        for (const auto [document_id, _]: word_to_document_freqs_.at(word)) {
            document_to_relevance.erase(document_id);
        }
    }

    std::vector<Document> matched_documents;
    matched_documents.reserve(document_to_relevance.size());

    for (const auto [document_id, relevance]: document_to_relevance) {
        matched_documents.emplace_back(document_id, relevance, documents_.at(document_id).rating);
    }
    return matched_documents;
}

template<typename DocumentPredicate>
std::vector<Document>
SearchServer::FindAllDocuments(const std::execution::sequenced_policy &executionPolicy, const Query &query,
                               DocumentPredicate document_predicate) const {
    return FindAllDocuments(query, document_predicate);
}

template<typename DocumentPredicate>
std::vector<Document>
SearchServer::FindAllDocuments(const std::execution::parallel_policy &executionPolicy, const Query &query,
                               DocumentPredicate document_predicate) const {
    ConcurrentMap<int, double> document_to_relevance(documents_.size() / 4);

    std::for_each(executionPolicy, query.plus_words.begin(), query.plus_words.end(),
                  [this, &document_to_relevance, &document_predicate](std::string_view word) {
                      if (word_to_document_freqs_.count(word)) {
                          const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
                          for (const auto [document_id, term_freq]: word_to_document_freqs_.at(word)) {
                              const auto &document_data = documents_.at(document_id);
                              if (document_predicate(document_id, document_data.status, document_data.rating)) {
                                  document_to_relevance[document_id].ref_to_value += term_freq * inverse_document_freq;
                              }
                          }
                      }
                  });

    std::for_each(executionPolicy, query.minus_words.begin(), query.minus_words.end(),
                  [this, &document_to_relevance](std::string_view word) {
                      if (word_to_document_freqs_.count(word)) {
                          for (const auto [document_id, _]: word_to_document_freqs_.at(word)) {
                              document_to_relevance.Erase(document_id);
                          }
                      }
                  });

    auto document_to_relevance_temp = std::move(document_to_relevance.BuildOrdinaryMap());
    std::vector<Document> to_return (document_to_relevance_temp.size());

    for (const auto [document_id, relevance]: document_to_relevance_temp) {
        to_return.emplace_back(document_id, relevance, documents_.at(document_id).rating);
    }
    return to_return;

}

template<typename StringContainer>
SearchServer::SearchServer(const StringContainer &stop_words)
        : stop_words_(MakeUniqueNonEmptyStrings(stop_words))  // Extract non-empty stop words
{
    if (!all_of(stop_words_.begin(), stop_words_.end(), IsValidWord)) {
        throw std::invalid_argument("Some of stop words are invalid"s);
    }
}